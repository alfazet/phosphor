#include "bvh.hpp"
#include "camera.hpp"
#include "cmd_args.hpp"
#include "constants.h"
#include "helpers.h"
#include "image_output.hpp"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "photon.h"
#include "photon_hash.h"
#include "printers.hpp"
#include "random.h"
#include "scene.hpp"
#include "scene_buffers.hpp"

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

constexpr const char *KERNEL_COMPILATION_FLAGS = "-I./include";
constexpr const char *EMIT_PHOTONS_NAME = "emit_photons";
constexpr const char *TRACE_RAYS_NAME = "trace_rays";

void phosphor_main(const ArgsList &args) {
    ClContext ctx;
    print_opencl_data(ctx);

    SceneData scene = read_gltf_scene(args.model.c_str());
    if (scene.triangles.empty()) {
        LOG_ERROR("scene has no triangles, nothing to render");
        return;
    }
    Camera &camera = scene.cameras[*scene.chosen_camera];

    TimerScope timer_scope_bvh("building BVH");
    std::vector<BvhNode> bvh = create_tree(scene.triangles);
    timer_scope_bvh.stop();

    SceneBuffers buffers;
    buffers.upload_scene(ctx, scene, bvh);

    RngState rng = pcg_seed(args.seed);
    std::vector<float4> h_origin, h_dir;
    generate_primary_rays(camera, rng, args.width, args.height, args.iters, h_origin, h_dir);
    buffers.upload_rays(ctx, h_origin, h_dir);

    u32 photons_to_emit = pow2roundup(args.photons);
    u32 photons_per_batch = std::min(photons_to_emit, MAX_PHOTONS_PER_BATCH);
    u32 max_photons_in_batch = photons_per_batch * MAX_PHOTON_BOUNCES;

    cl::Program emit_photons =
        ctx.build_program(std::string(PROJECT_DIR) + "/kernels/" + EMIT_PHOTONS_NAME + ".cl", KERNEL_COMPILATION_FLAGS);
    cl::Kernel k_emit_photons(emit_photons, EMIT_PHOTONS_NAME);

    cl::Buffer d_photons(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(Photon));
    u32 h_photon_count = 0;
    cl::Buffer d_photon_count(ctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(u32), &h_photon_count);

    std::vector<Photon> h_photons;
    TimerScope timer_scope_photons("emitting photons");
    for (u32 batch_offset = 0; batch_offset < photons_to_emit; batch_offset += photons_per_batch) {
        u32 batch_emit = std::min(photons_per_batch, photons_to_emit - batch_offset);
        h_photon_count = 0;
        ctx.queue.enqueueWriteBuffer(d_photon_count, CL_TRUE, 0, sizeof(u32), &h_photon_count);

        buffers.set_emit_photons_args(k_emit_photons, batch_offset, photons_to_emit, args.seed, max_photons_in_batch,
                                      d_photons, d_photon_count);
        ctx.queue.enqueueNDRangeKernel(k_emit_photons, cl::NullRange, cl::NDRange(batch_emit), cl::NullRange);
        ctx.queue.finish();

        u32 batch_count = 0;
        ctx.queue.enqueueReadBuffer(d_photon_count, CL_TRUE, 0, sizeof(u32), &batch_count);
        batch_count = std::min(batch_count, max_photons_in_batch);

        // TODO: this could be done without allocating batch_photons
        std::vector<Photon> batch_photons(batch_count);
        ctx.queue.enqueueReadBuffer(d_photons, CL_TRUE, 0, batch_count * sizeof(Photon), batch_photons.data());
        h_photons.insert(h_photons.end(), batch_photons.begin(), batch_photons.end());
    }
    timer_scope_photons.stop();

    TimerScope timer_scope_hash("building hash struct for photons");
    PhotonHashInfo info = build_hash_info(bvh[1].bbox, args.grid_res);
    PhotonHash struct_hash = build_hash(h_photons, info);
    timer_scope_hash.stop();

    cl::Program trace_rays =
        ctx.build_program(std::string(PROJECT_DIR) + "/kernels/" + TRACE_RAYS_NAME + ".cl", KERNEL_COMPILATION_FLAGS);
    cl::Kernel k_trace_rays(trace_rays, TRACE_RAYS_NAME);

    buffers.upload_photons(ctx, struct_hash, h_photons);
    cl::Buffer d_out(ctx.context, CL_MEM_WRITE_ONLY, buffers.n_rays * sizeof(float4));

    f32 search_radius = std::min({info.cell_sizes.x, info.cell_sizes.y, info.cell_sizes.z}) / 2.0f;
    buffers.set_trace_rays_args(k_trace_rays, search_radius, args.samples, info, args.seed, d_out);

    TimerScope timer_scope_image("rendering image");
    ctx.queue.enqueueNDRangeKernel(k_trace_rays, cl::NullRange, cl::NDRange(buffers.n_rays), cl::NullRange);
    ctx.queue.finish();

    std::vector<float4> h_out(buffers.n_rays);
    ctx.queue.enqueueReadBuffer(d_out, CL_TRUE, 0, buffers.n_rays * sizeof(float4), h_out.data());
    timer_scope_image.stop();

    write_png(args.output_path, args.width, args.height, args.iters, h_out);
}

i32 main(i32 argc, char **argv) {
    init_logger();
    ArgParser arg_parser(argc, argv, std::cout);
    try {
        auto args = arg_parser.parse_all();
        LOG_INFO("chosen parameters:");
        arg_parser.print_values(args);
        phosphor_main(args);
    } catch (const HelpRequested &) {
        arg_parser.print_help();
        return 0;
    } catch (const ArgParseError &e) {
        LOG_ERROR("parsing arguments: {}", e.what());
        arg_parser.print_help();
        return 1;
    }

    return 0;
}
