#include "../include/logger.hpp"
#include "bvh.hpp"
#include "camera.hpp"
#include "cmd_args.hpp"
#include "constants.h"
#include "image_output.hpp"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "photon_hash.h"
#include "random.h"
#include "scene.hpp"
#include "scene_buffers.hpp"
#include "utils.h"

#include <iostream>
#include <vector>

void phosphor_main(const ArgsList &args) {
    ClContext ctx;
    LOG_INFO("OpenCL platform/device: {}/{} with max. alloc size {} bytes", ctx.platform_name(), ctx.device_name(),
             ctx.max_alloc_size());

    cl::Kernel k_emit_photons = ctx.make_kernel("emit_photons");
    cl::Kernel k_trace_rays = ctx.make_kernel("trace_rays");

    SceneData scene = read_gltf_scene(args.model.c_str());
    if (scene.triangles.empty()) {
        LOG_ERROR("empty scene, nothing to render");
        return;
    }

    TimerScope timer_scope_bvh("building BVH");
    Bvh bvh(scene.triangles);
    timer_scope_bvh.stop();
    const BoundingBox &bbox = bvh.get_bbox();

    SceneBuffers buffers;
    buffers.upload_scene(ctx, scene, bvh);

    RngState rng = pcg_seed(args.seed);
    auto [h_origin, h_dir] = scene.get_camera().generate_rays(rng, args.res, args.res, args.image_iters);
    buffers.upload_rays(ctx, h_origin, h_dir);

    u32 photons_to_emit = round_up_to_pow2(args.photons);
    u32 photons_per_batch = std::min(photons_to_emit, MAX_PHOTONS_PER_BATCH);
    u32 max_photons_in_batch = photons_per_batch * MAX_PHOTON_BOUNCES;

    cl::Buffer d_photon_pos(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(float4));
    cl::Buffer d_photon_power(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(float4));
    cl::Buffer d_photon_dir(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(float4));
    cl::Buffer d_photon_normal(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(float4));
    u32 h_batch_size = 0;
    cl::Buffer d_batch_size(ctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(u32), &h_batch_size);
    std::vector<float4> h_photon_pos, h_photon_power, h_photon_dir, h_photon_normal;

    ProgressScope progress_scope_photons("emitting photons", photons_to_emit);
    for (u32 batch_offset = 0; batch_offset < photons_to_emit; batch_offset += photons_per_batch) {
        u32 to_emit = std::min(photons_per_batch, photons_to_emit - batch_offset);
        progress_scope_photons.increase(to_emit);
        h_batch_size = 0;
        ctx.queue.enqueueWriteBuffer(d_batch_size, CL_TRUE, 0, sizeof(u32), &h_batch_size);

        buffers.set_emit_photons_args(k_emit_photons, batch_offset, photons_to_emit, args.seed, max_photons_in_batch,
                                      d_photon_pos, d_photon_power, d_photon_dir, d_photon_normal, d_batch_size);
        ctx.queue.enqueueNDRangeKernel(k_emit_photons, cl::NullRange, cl::NDRange(to_emit), cl::NullRange);
        ctx.queue.finish();

        u32 h_final_batch_size = 0;
        ctx.queue.enqueueReadBuffer(d_batch_size, CL_TRUE, 0, sizeof(u32), &h_final_batch_size);
        h_final_batch_size = std::min(h_final_batch_size, max_photons_in_batch);
        if (h_final_batch_size <= 0)
            LOG_FATAL("no photon hit");

        const usize old_size = h_photon_pos.size();
        h_photon_pos.resize(old_size + h_final_batch_size);
        h_photon_power.resize(old_size + h_final_batch_size);
        h_photon_dir.resize(old_size + h_final_batch_size);
        h_photon_normal.resize(old_size + h_final_batch_size);

        ctx.queue.enqueueReadBuffer(d_photon_pos, CL_TRUE, 0, h_final_batch_size * sizeof(float4),
                                    h_photon_pos.data() + old_size);
        ctx.queue.enqueueReadBuffer(d_photon_power, CL_TRUE, 0, h_final_batch_size * sizeof(float4),
                                    h_photon_power.data() + old_size);
        ctx.queue.enqueueReadBuffer(d_photon_dir, CL_TRUE, 0, h_final_batch_size * sizeof(float4),
                                    h_photon_dir.data() + old_size);
        ctx.queue.enqueueReadBuffer(d_photon_normal, CL_TRUE, 0, h_final_batch_size * sizeof(float4),
                                    h_photon_normal.data() + old_size);
    }

    TimerScope timer_scope_hash("building hash struct for photons");
    PhotonHashInfo info = build_hash_info(bbox, args.grid_res);
    PhotonHash struct_hash = build_hash(h_photon_pos, h_photon_power, h_photon_dir, h_photon_normal, info);
    timer_scope_hash.stop();

    buffers.upload_photons(ctx, struct_hash, h_photon_pos, h_photon_power, h_photon_dir, h_photon_normal);

    buffers.print_buffer_sizes();

    cl::Buffer d_out(ctx.context, CL_MEM_WRITE_ONLY, buffers.n_rays * sizeof(float4));
    f32 search_radius = std::min({info.cell_sizes.x, info.cell_sizes.y, info.cell_sizes.z}) / 2.0f;
    buffers.set_trace_rays_args(k_trace_rays, search_radius, args.samples, info, args.seed, d_out);

    TimerScope timer_scope_image("rendering image");
    ctx.queue.enqueueNDRangeKernel(k_trace_rays, cl::NullRange, cl::NDRange(buffers.n_rays), cl::NullRange);
    ctx.queue.finish();

    std::vector<float4> h_out(buffers.n_rays);
    ctx.queue.enqueueReadBuffer(d_out, CL_TRUE, 0, buffers.n_rays * sizeof(float4), h_out.data());
    timer_scope_image.stop();

    write_png(args.output_path, args.res, args.res, args.image_iters, h_out);
}

i32 main(i32 argc, char **argv) {
    init_logger();
    ArgParser arg_parser(argc, argv, std::cout);
    try {
        auto args = arg_parser.parse_all();
        LOG_INFO("chosen parameters:");
        arg_parser.print_values(args);
        phosphor_main(args);
        arg_parser.write_image_metadata(args);
    } catch (const HelpRequested &) {
        arg_parser.print_help();
        return 0;
    } catch (const ArgParseError &e) {
        LOG_ERROR("parsing arguments: {}", e.what());
        arg_parser.print_help();
        return 1;
    } catch (const cl::Error &e) {
        // look up the codes here: https://gist.github.com/bmount/4a7144ce801e5569a0b6
        LOG_ERROR("OpenCL error (code {}): {}", e.err(), e.what());
        return 1;
    }

    return 0;
}
