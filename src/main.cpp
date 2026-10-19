#include "bvh.hpp"
#include "camera.hpp"
#include "cmd_args.hpp"
#include "constants.h"
#include "image_output.hpp"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "photon_hash.h"
#include "scene.hpp"
#include "scene_buffers.hpp"
#include "sppm_pixel.h"
#include "utils.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <vector>

void phosphor_main(const ArgsList &args, const std::string &image_metadata) {
    ClContext ctx;
    LOG_INFO("OpenCL platform/device: {}/{}", ctx.platform_name(), ctx.device_name());
    LOG_INFO("max. alloc size: {} bytes", ctx.max_alloc_size());
    LOG_INFO("max. workgroup size: {} threads", ctx.max_work_group_size());

    cl::Kernel k_emit_photons = ctx.make_kernel("emit_photons");
    cl::Kernel k_camera_pass = ctx.make_kernel("camera_pass");
    cl::Kernel k_gather_pass = ctx.make_kernel("gather_pass");
    cl::Kernel k_hash_photons = ctx.make_kernel("hash_photons");
    cl::Kernel k_scan_cells = ctx.make_kernel("scan_cells");
    cl::Kernel k_scatter_photons = ctx.make_kernel("scatter_photons");
    u32 local_size = args.workgroup_size;

    SceneData scene = read_gltf_scene(args.model.c_str());
    if (scene.triangles.empty()) {
        LOG_ERROR("empty scene, nothing to render");
        return;
    }
    if (!args.envmap.empty()) {
        scene.load_envmap(args.envmap.c_str());
    }

    std::filesystem::path output_dir(args.output_dir);
    if (!args.was_provided("-o")) {
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
    u32 max_photons_total = photons_per_round * args.max_photon_bounces;

    PhotonHashInfo photon_hash_info = build_photon_hash_info(bbox, args.grid_res);
    u32 bucket_count = args.grid_res * args.grid_res * args.grid_res + 1;

    cl::Buffer d_photon_pos(ctx.context, CL_MEM_READ_WRITE, max_photons_total * sizeof(float4));
    cl::Buffer d_photon_power(ctx.context, CL_MEM_READ_WRITE, max_photons_total * sizeof(u32));
    cl::Buffer d_photon_dir(ctx.context, CL_MEM_READ_WRITE, max_photons_total * sizeof(u32));

    u32 h_batch_size = 0;
    cl::Buffer d_batch_size(ctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(u32), &h_batch_size);

    u64 total_photons_emitted = 0;
    buffers.alloc_photon_hash_buffers(ctx, max_photons_total, bucket_count);
    RngState rng = pcg_seed(args.seed);

    std::vector<SppmPixel> h_sppm(buffers.n_pixels);
    std::vector<float4> h_total_irradiance(buffers.n_pixels);

    for (u32 round = 1; round <= args.sppm_rounds; round++) {
        TimerScope timer_scope_round(std::format("SPPM round {}", round));
        u32 round_seed = random_u32(&rng);

        buffers.set_camera_pass_args(k_camera_pass, round_seed, args.direct_samples);

        ctx.queue.enqueueNDRangeKernel(k_camera_pass, cl::NullRange,
                                       cl::NDRange(global_size(buffers.n_pixels, local_size)), cl::NDRange(local_size));
        ctx.queue.finish();

        h_batch_size = 0;
        ctx.queue.enqueueWriteBuffer(d_batch_size, CL_TRUE, 0, sizeof(u32), &h_batch_size);
        {
            for (u32 batch_offset = 0; batch_offset < photons_per_round; batch_offset += photons_per_batch) {
                u32 to_emit = std::min(photons_per_batch, photons_per_round - batch_offset);
                u32 emission_seed = random_u32(&rng);
                buffers.set_emit_photons_args(k_emit_photons, batch_offset, photons_per_round, args.max_photon_bounces,
                                              emission_seed, max_photons_total, d_photon_pos, d_photon_power,
                                              d_photon_dir, d_batch_size);
                ctx.queue.enqueueNDRangeKernel(k_emit_photons, cl::NullRange,
                                               cl::NDRange(global_size(to_emit, local_size)), cl::NDRange(local_size));
            }
            ctx.queue.finish();
        }

        u32 n_photons = 0;
        ctx.queue.enqueueReadBuffer(d_batch_size, CL_TRUE, 0, sizeof(u32), &n_photons);
        n_photons = std::min(n_photons, max_photons_total);
        if (n_photons == 0)
            LOG_FATAL("no photon hit");
        buffers.n_photons = n_photons;

        {
            // hash photons
            ctx.queue.enqueueFillBuffer(buffers.cell_counts, 0, 0, bucket_count * sizeof(u32));
            buffers.set_hash_photons_args(k_hash_photons, d_photon_pos, n_photons, photon_hash_info);
            ctx.queue.enqueueNDRangeKernel(k_hash_photons, cl::NullRange,
                                           cl::NDRange(global_size(n_photons, local_size)), cl::NDRange(local_size));

            // scan on grid cells
            buffers.set_scan_cells_args(k_scan_cells, bucket_count);
            ctx.queue.enqueueNDRangeKernel(k_scan_cells, cl::NullRange, cl::NDRange(1), cl::NullRange);

            // scatter photons into sorted output buffers
            ctx.queue.enqueueCopyBuffer(buffers.cell_start, buffers.cell_end, 0, 0, bucket_count * sizeof(u32));
            buffers.set_scatter_args(k_scatter_photons, d_photon_pos, d_photon_power, d_photon_dir, n_photons);
            ctx.queue.enqueueNDRangeKernel(k_scatter_photons, cl::NullRange,
                                           cl::NDRange(global_size(n_photons, local_size)), cl::NDRange(local_size));

            ctx.queue.finish();
        }

        total_photons_emitted += photons_per_round;
        buffers.set_gather_pass_args(k_gather_pass, photon_hash_info, args.sppm_alpha);
        ctx.queue.enqueueNDRangeKernel(k_gather_pass, cl::NullRange,
                                       cl::NDRange(global_size(buffers.n_pixels, local_size)), cl::NDRange(local_size));
        ctx.queue.finish();
        timer_scope_round.stop();

        if ((args.save_snapshots && is_pow2(round)) || round == args.sppm_rounds) {
            ctx.queue.enqueueReadBuffer(buffers.sppm_pixels, CL_TRUE, 0, buffers.n_pixels * sizeof(SppmPixel),
                                        h_sppm.data());
            ctx.queue.enqueueReadBuffer(buffers.total_irradiance, CL_TRUE, 0, buffers.n_pixels * sizeof(float4),
                                        h_total_irradiance.data());

            std::filesystem::path image_path = output_dir / std::format("{:0>6}.png", std::to_string(round));
            write_png(image_path, args.res, args.res, h_sppm, h_total_irradiance, total_photons_emitted,
                      args.sppm_rounds, image_metadata);
            LOG_INFO("rendered image after {} SPPM rounds written to {}", round, image_path.c_str());
        }
    }
}

i32 main(i32 argc, char **argv) {
    init_logger();
    ArgParser arg_parser(argc, argv, std::cout);
    try {
        auto args = arg_parser.parse_all();
        if (args.help) {
            arg_parser.print_help();
            return 1;
        }
        LOG_INFO("chosen parameters:");
        arg_parser.print_values(args);
        std::string metadata = arg_parser.build_image_metadata(args);
        phosphor_main(args, metadata);
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
