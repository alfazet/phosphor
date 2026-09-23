#include "bvh.hpp"
#include "camera.hpp"
#include "cmd_args.hpp"
#include "constants.h"
#include "image_output.hpp"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "photon_hash.hpp"
#include "scene.hpp"
#include "scene_buffers.hpp"
#include "sppm_pixel.h"
#include "utils.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <vector>

void phosphor_main(const ArgsList &args) {
    ClContext ctx;
    LOG_INFO("OpenCL platform/device: {}/{} with max. alloc size {} bytes", ctx.platform_name(), ctx.device_name(),
             ctx.max_alloc_size());

    cl::Kernel k_emit_photons = ctx.make_kernel("emit_photons");
    cl::Kernel k_camera_pass = ctx.make_kernel("camera_pass");
    cl::Kernel k_gather_pass = ctx.make_kernel("gather_pass");

    SceneData scene = read_gltf_scene(args.model.c_str());
    if (scene.triangles.empty()) {
        LOG_ERROR("empty scene, nothing to render");
        return;
    }

    std::filesystem::path output_dir(args.output_dir);
    if (std::strcmp(args.output_dir.c_str(), DEFAULT_OUTPUT_DIR) == 0) {
        auto now = std::chrono::system_clock::now();
        std::string timestamp = std::format("{:%Y_%m_%d-%H_%M_%S}", std::chrono::floor<std::chrono::seconds>(now));
        output_dir = std::filesystem::path(args.output_dir + "_" + timestamp);
    }
    std::filesystem::create_directory(output_dir);

    Camera &camera = scene.get_camera();
    camera.focus(args.defocus_angle, args.focus_distance);

    Bvh bvh(scene.triangles);
    const BoundingBox &bbox = bvh.get_bbox();

    SceneBuffers buffers;
    buffers.copy_scene(ctx, scene, bvh);
    buffers.set_camera(camera.to_params(), args.res, args.res);
    buffers.alloc_sppm_buffers(ctx);

    u32 photons_per_round = args.photons;
    u32 photons_per_batch = std::min(photons_per_round, MAX_PHOTONS_PER_BATCH);
    u32 max_photons_in_batch = photons_per_batch * MAX_PHOTON_BOUNCES;
    cl::Buffer d_photon_pos(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(float4));
    cl::Buffer d_photon_power(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(u32));
    cl::Buffer d_photon_dir(ctx.context, CL_MEM_READ_WRITE, max_photons_in_batch * sizeof(u32));
    u32 h_batch_size = 0;
    cl::Buffer d_batch_size(ctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(u32), &h_batch_size);

    u64 total_photons_emitted = 0;
    RngState rng = pcg_seed(args.seed);
    PhotonHashInfo photon_hash_info = build_photon_hash_info(bbox, args.grid_res);

    std::vector<SppmPixel> h_sppm(buffers.n_pixels);
    std::vector<float4> h_total_irradiance(buffers.n_pixels);

    for (u32 round = 1; round <= args.sppm_rounds; round++) {
        u32 round_seed = random_u32(&rng);

        // camera pass (trace rays and record their first diffuse hit)
        buffers.set_camera_pass_args(k_camera_pass, round_seed, args.direct_samples);
        ctx.queue.enqueueNDRangeKernel(k_camera_pass, cl::NullRange, cl::NDRange(buffers.n_pixels), cl::NullRange);
        ctx.queue.finish();

        // photon emission
        std::vector<float4> h_photon_pos;
        std::vector<u32> h_photon_power, h_photon_dir;
        ProgressScope progress_scope_photons("emitting photons", photons_per_round);
        for (u32 batch_offset = 0; batch_offset < photons_per_round; batch_offset += photons_per_batch) {
            u32 to_emit = std::min(photons_per_batch, photons_per_round - batch_offset);
            progress_scope_photons.increase(to_emit);
            h_batch_size = 0;
            ctx.queue.enqueueWriteBuffer(d_batch_size, CL_TRUE, 0, sizeof(u32), &h_batch_size);

            u32 emission_seed = random_u32(&rng);
            buffers.set_emit_photons_args(k_emit_photons, batch_offset, photons_per_round, emission_seed,
                                          max_photons_in_batch, d_photon_pos, d_photon_power, d_photon_dir,
                                          d_batch_size);
            ctx.queue.enqueueNDRangeKernel(k_emit_photons, cl::NullRange, cl::NDRange(to_emit), cl::NullRange);
            ctx.queue.finish();

            u32 h_final_batch_size = 0;
            ctx.queue.enqueueReadBuffer(d_batch_size, CL_TRUE, 0, sizeof(u32), &h_final_batch_size);
            h_final_batch_size = std::min(h_final_batch_size, max_photons_in_batch);
            if (h_final_batch_size == 0)
                LOG_FATAL("no photon hit");

            u32 old_size = h_photon_pos.size();
            h_photon_pos.resize(old_size + h_final_batch_size);
            h_photon_power.resize(old_size + h_final_batch_size);
            h_photon_dir.resize(old_size + h_final_batch_size);

            ctx.queue.enqueueReadBuffer(d_photon_pos, CL_TRUE, 0, h_final_batch_size * sizeof(float4),
                                        h_photon_pos.data() + old_size);
            ctx.queue.enqueueReadBuffer(d_photon_power, CL_TRUE, 0, h_final_batch_size * sizeof(u32),
                                        h_photon_power.data() + old_size);
            ctx.queue.enqueueReadBuffer(d_photon_dir, CL_TRUE, 0, h_final_batch_size * sizeof(u32),
                                        h_photon_dir.data() + old_size);
        }
        total_photons_emitted += photons_per_round;

        TimerScope timer_scope_hash("building spatial hash");
        PhotonHash photon_hash(h_photon_pos, h_photon_power, h_photon_dir, photon_hash_info);
        buffers.copy_photons(ctx, photon_hash, h_photon_pos, h_photon_power, h_photon_dir);
        timer_scope_hash.stop();

        // gather pass (add up irradiance and do the SPPM update)
        buffers.set_gather_pass_args(k_gather_pass, photon_hash_info, args.sppm_alpha);
        ctx.queue.enqueueNDRangeKernel(k_gather_pass, cl::NullRange, cl::NDRange(buffers.n_pixels), cl::NullRange);
        ctx.queue.finish();

        // save snapshots once in a while if enabled
        if ((args.save_snapshots && is_pow2(round)) || round == args.sppm_rounds) {
            ctx.queue.enqueueReadBuffer(buffers.sppm_pixels, CL_TRUE, 0, buffers.n_pixels * sizeof(SppmPixel),
                                        h_sppm.data());
            ctx.queue.enqueueReadBuffer(buffers.total_irradiance, CL_TRUE, 0, buffers.n_pixels * sizeof(float4),
                                        h_total_irradiance.data());

            std::filesystem::path image_path = output_dir / std::format("{:0>6}.png", std::to_string(round));
            write_png(image_path, args.res, args.res, h_sppm, h_total_irradiance, total_photons_emitted,
                      args.sppm_rounds);
            LOG_INFO("rendered image after {} SPPM rounds written to {}", round, image_path.c_str());
        }
    }
}

i32 main(i32 argc, char **argv) {
    init_logger();
    ArgParser arg_parser(argc, argv, std::cout);
    try {
        auto args = arg_parser.parse_all();
        LOG_INFO("chosen parameters:");
        arg_parser.print_values(args);
        phosphor_main(args);
        // TODO: rewrite this if we even care
        // arg_parser.write_image_metadata(args);
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
