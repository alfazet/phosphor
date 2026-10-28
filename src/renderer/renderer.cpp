#include "host/renderer.hpp"
#include "common/constants.h"
#include "host/logger.hpp"
#include "host/opencl_ctx.hpp"
#include "host/output.hpp"

#include <format>

Renderer::Renderer(ClContext ctx, ArgsList args, std::filesystem::path output_dir, std::string image_metadata)
    : ctx(ctx), args(args), output_dir(output_dir), image_metadata(image_metadata),
      workgroup_size(args.workgroup_size) {
    this->k_emit_photons = ctx.make_kernel("emit_photons");
    this->k_trace_rays = ctx.make_kernel("trace_rays");
    this->k_gather_flux = ctx.make_kernel("gather_flux");
    this->k_hash_photons = ctx.make_kernel("hash_photons");
    this->k_scan_small_chunks = ctx.make_kernel("scan_small_chunks");
    this->k_scan_big_chunks = ctx.make_kernel("scan_big_chunks");
    this->k_scan_add_offsets = ctx.make_kernel("scan_add_offsets");
    this->k_scatter_photons = ctx.make_kernel("scatter_photons");
}

void Renderer::set_scene(SceneData &scene) {
    if (!this->args.envmap.empty())
        scene.load_envmap(this->args.envmap.c_str());

    Camera &camera = scene.get_camera();
    this->render_size = resolve_render_size(this->args, camera.aspect_ratio);
    camera.aspect_ratio = this->render_size.aspect;
    camera.focus(this->args.defocus_angle, this->args.focus_distance);

    Bvh bvh(scene.triangles);
    const BoundingBox &bbox = bvh.get_bbox();

    this->buffers.upload_scene(this->ctx, scene, bvh);
    this->buffers.alloc_image_buffers(this->ctx, camera.to_params(), this->render_size.width, this->render_size.height);

    this->photon_hash_info = build_photon_hash_info(bbox, this->args.grid_res);
    this->bucket_count = this->args.grid_res * this->args.grid_res * this->args.grid_res + 1;

    this->photon_buf_size = this->args.photons_per_round * this->args.max_photon_bounces;
    this->d_photon_pos = cl::Buffer(this->ctx.context, CL_MEM_READ_WRITE, this->photon_buf_size * sizeof(float4));
    this->d_photon_power = cl::Buffer(this->ctx.context, CL_MEM_READ_WRITE, this->photon_buf_size * sizeof(u32));
    this->d_photon_dir = cl::Buffer(this->ctx.context, CL_MEM_READ_WRITE, this->photon_buf_size * sizeof(u32));

    u32 h_batch_size = 0;
    this->d_batch_size =
        cl::Buffer(this->ctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(u32), &h_batch_size);

    u32 n_scan_chunks = (this->bucket_count + SCAN_CHUNK_SIZE - 1) / SCAN_CHUNK_SIZE;
    this->buffers.alloc_photon_buffers(this->ctx, this->photon_buf_size, this->bucket_count);
    this->d_small_chunk_sums = cl::Buffer(this->ctx.context, CL_MEM_READ_WRITE, n_scan_chunks * sizeof(u32));
    this->d_big_chunk_sums = cl::Buffer(this->ctx.context, CL_MEM_READ_WRITE, n_scan_chunks * sizeof(u32));

    this->rng = pcg_seed(this->args.seed);
    this->h_flux.resize(this->buffers.n_pixels());
    this->h_total_radiance.resize(this->buffers.n_pixels());
}

void Renderer::run() {
    u32 photons_per_round = this->args.photons_per_round;
    u32 photon_buf_size = this->photon_buf_size;
    u64 total_photons_emitted = 0;

    for (u32 round = 1; round <= this->args.sppm_rounds; round++) {
        u32 round_seed = random_u32(&this->rng);

        TimerScope timer_scope_round(std::format("SPPM round {}", round));
        // trace camera rays until the first diffuse hit
        this->trace_rays(round_seed);
        // emit photons from light sources
        this->emit_photons(round_seed);
        // hash photons based on their location and sort them by that hash
        this->hash_and_sort_photons();
        // gather photon flux to compute indirect lighting and run the SPPM update
        this->gather_flux();
        timer_scope_round.stop();

        total_photons_emitted += photons_per_round;
        if ((this->args.save_snapshots && is_pow2(round)) || round == this->args.sppm_rounds) {
            this->ctx.queue.enqueueReadBuffer(this->buffers.d_flux(), CL_TRUE, 0,
                                              this->buffers.n_pixels() * sizeof(float4), this->h_flux.data());
            this->ctx.queue.enqueueReadBuffer(this->buffers.d_total_radiance(), CL_TRUE, 0,
                                              this->buffers.n_pixels() * sizeof(float4), this->h_total_radiance.data());
            this->save_snapshot(round, total_photons_emitted);
        }
    }
    this->writer.flush();
}

void Renderer::trace_rays(u32 seed) {
    this->buffers.set_trace_rays_args(this->k_trace_rays, seed, this->args.direct_samples);
    this->ctx.queue.enqueueNDRangeKernel(this->k_trace_rays, cl::NullRange,
                                         cl::NDRange(global_size(this->buffers.n_pixels(), this->workgroup_size)),
                                         cl::NDRange(this->workgroup_size));
}

void Renderer::emit_photons(u32 seed) {
    u32 h_batch_size = 0;
    this->ctx.queue.enqueueWriteBuffer(this->d_batch_size, CL_FALSE, 0, sizeof(u32), &h_batch_size);

    u32 photons_per_round = this->args.photons_per_round;
    u32 photons_per_batch = std::min(photons_per_round, this->args.max_photons_per_batch);
    for (u32 batch_offset = 0; batch_offset < photons_per_round; batch_offset += photons_per_batch) {
        u32 to_emit = std::min(photons_per_batch, photons_per_round - batch_offset);
        this->buffers.set_emit_photons_args(this->k_emit_photons, batch_offset, to_emit, this->args.max_photon_bounces,
                                            seed, this->photon_buf_size, this->d_photon_pos, this->d_photon_power,
                                            this->d_photon_dir, this->d_batch_size);
        this->ctx.queue.enqueueNDRangeKernel(this->k_emit_photons, cl::NullRange,
                                             cl::NDRange(global_size(to_emit, this->workgroup_size)),
                                             cl::NDRange(this->workgroup_size));
    }

    u32 n_photons = 0;
    this->ctx.queue.enqueueReadBuffer(this->d_batch_size, CL_TRUE, 0, sizeof(u32), &n_photons);
    n_photons = std::min(n_photons, this->photon_buf_size);
    // TODO: should it crash like this?
    if (n_photons == 0)
        LOG_FATAL("no photon hit");

    this->buffers.n_photons() = n_photons;
}

void Renderer::hash_and_sort_photons() {
    auto &p = this->buffers.photon_buf;
    u32 n_photons = this->buffers.n_photons();
    u32 n_scan_chunks = (this->bucket_count + SCAN_CHUNK_SIZE - 1) / SCAN_CHUNK_SIZE;

    this->ctx.queue.enqueueFillBuffer(p.d_cell_counts, 0, 0, this->bucket_count * sizeof(u32));
    this->buffers.set_hash_photons_args(this->k_hash_photons, this->d_photon_pos, n_photons, this->photon_hash_info);
    this->ctx.queue.enqueueNDRangeKernel(this->k_hash_photons, cl::NullRange,
                                         cl::NDRange(global_size(n_photons, this->workgroup_size)),
                                         cl::NDRange(this->workgroup_size));

    set_kernel_args(this->k_scan_small_chunks, p.d_cell_counts, p.d_cell_start, this->d_small_chunk_sums,
                    this->bucket_count);
    this->ctx.queue.enqueueNDRangeKernel(this->k_scan_small_chunks, cl::NullRange,
                                         cl::NDRange(n_scan_chunks * SCAN_WG_SIZE), cl::NDRange(SCAN_WG_SIZE));

    set_kernel_args(this->k_scan_big_chunks, this->d_small_chunk_sums, this->d_big_chunk_sums, n_scan_chunks);
    this->ctx.queue.enqueueNDRangeKernel(this->k_scan_big_chunks, cl::NullRange, cl::NDRange(SCAN_WG_SIZE),
                                         cl::NDRange(SCAN_WG_SIZE));

    set_kernel_args(this->k_scan_add_offsets, p.d_cell_start, this->d_big_chunk_sums, this->bucket_count);
    this->ctx.queue.enqueueNDRangeKernel(this->k_scan_add_offsets, cl::NullRange,
                                         cl::NDRange(n_scan_chunks * SCAN_WG_SIZE), cl::NDRange(SCAN_WG_SIZE));

    this->ctx.queue.enqueueCopyBuffer(p.d_cell_start, p.d_cell_end, 0, 0, this->bucket_count * sizeof(u32));
    this->buffers.set_scatter_args(this->k_scatter_photons, this->d_photon_pos, this->d_photon_power,
                                   this->d_photon_dir, n_photons);
    this->ctx.queue.enqueueNDRangeKernel(this->k_scatter_photons, cl::NullRange,
                                         cl::NDRange(global_size(n_photons, this->workgroup_size)),
                                         cl::NDRange(this->workgroup_size));
}

void Renderer::gather_flux() {
    this->buffers.set_gather_flux_args(this->k_gather_flux, this->photon_hash_info, this->args.sppm_alpha);
    this->ctx.queue.enqueueNDRangeKernel(this->k_gather_flux, cl::NullRange,
                                         cl::NDRange(global_size(this->buffers.n_pixels(), this->workgroup_size)),
                                         cl::NDRange(this->workgroup_size));
    this->ctx.queue.finish();
}

void Renderer::save_snapshot(u32 round, u64 total_photons_emitted) {
    std::filesystem::path image_path = this->output_dir / std::format("{:06d}.png", round);
    this->writer.submit(image_path, this->render_size.width, this->render_size.height, this->args.exposure,
                        this->h_flux, this->h_total_radiance, total_photons_emitted, round, this->image_metadata);
    LOG_INFO("rendered image after {} SPPM rounds enqueued for writing: {}", round, image_path.c_str());
}
