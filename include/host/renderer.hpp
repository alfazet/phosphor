#ifndef PHOSPHOR_RENDERER_HPP
#define PHOSPHOR_RENDERER_HPP

#include "buffers/rendering.hpp"
#include "common/constants.h"
#include "common/photon_hash.h"
#include "host/cmd_args.hpp"
#include "host/opencl_ctx.hpp"
#include "host/output.hpp"
#include "host/scene_data.hpp"

#include <filesystem>
#include <string>
#include <vector>

class Renderer {
  public:
    Renderer(ClContext ctx, ArgsList args, std::filesystem::path output_dir, std::string image_metadata);

    void set_scene(SceneData &scene);

    void run();

  private:
    ClContext ctx;
    cl::Kernel k_trace_rays;
    cl::Kernel k_emit_photons;
    cl::Kernel k_hash_photons;
    cl::Kernel k_scan_small_chunks;
    cl::Kernel k_scan_big_chunks;
    cl::Kernel k_scan_add_offsets;
    cl::Kernel k_scatter_photons;
    cl::Kernel k_gather_flux;

    ArgsList args;
    std::filesystem::path output_dir;
    std::string image_metadata;
    u32 workgroup_size = 0;

    RenderSize render_size{};
    PhotonHashData photon_hash_info{};
    u32 bucket_count = 0;
    u32 photon_buf_size = 0;
    u32 scan_wg_size = SCAN_WG_SIZE;

    RenderingBuffers buffers;

    cl::Buffer d_photon_pos;
    cl::Buffer d_photon_power;
    cl::Buffer d_photon_dir;
    cl::Buffer d_batch_size;

    cl::Buffer d_small_chunk_sums;
    cl::Buffer d_big_chunk_sums;

    std::vector<float4> h_flux;
    std::vector<float4> h_total_radiance;

    ImageWriter writer;

    RngState rng{};

    void trace_rays(u32 seed);

    void emit_photons(u32 seed);

    void hash_and_sort_photons();

    void gather_flux();

    void save_snapshot(u32 round, u64 total_photons_emitted);
};

#endif // PHOSPHOR_RENDERER_HPP
