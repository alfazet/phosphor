#ifndef PHOSPHOR_SCENE_BUFFERS_HPP
#define PHOSPHOR_SCENE_BUFFERS_HPP

#include "bvh.hpp"
#include "opencl_ctx.hpp"
#include "photon.h"
#include "photon_hash.h"
#include "scene.hpp"
#include "typedefs.h"

#include <vector>

struct SceneBuffers {
    cl::Buffer tri_v0, tri_v1, tri_v2;
    cl::Buffer tri_uv0, tri_uv1, tri_uv2;
    cl::Buffer tri_n0, tri_n1, tri_n2;
    cl::Buffer tri_t0, tri_t1, tri_t2;
    cl::Buffer tri_mat_index;
    u32 n_triangles = 0;

    cl::Buffer bvh_nodes;

    cl::Buffer materials;
    u32 n_materials = 0;

    cl::Buffer lights;
    cl::Buffer light_pref_sum;
    u32 n_lights = 0;
    f32 total_luminance = 0.0f;

    cl::Buffer tex_atlas;
    cl::Buffer tex_meta;
    u32 n_textures = 0;

    cl::Buffer ray_origin;
    cl::Buffer ray_dir;
    u32 n_rays = 0;

    cl::Buffer photons_sorted;
    cl::Buffer tree_index;
    cl::Buffer bucket_tree_offset;
    cl::Buffer bucket_tree_size;
    u32 n_photons = 0;

    float4 scene_center{};
    f32 scene_radius = 0.0f;

    void upload_scene(ClContext &ctx, const SceneData &scene, const Bvh &bvh);

    void upload_rays(ClContext &ctx, const std::vector<float4> &origins, const std::vector<float4> &dirs);

    void upload_photons(ClContext &ctx, PhotonHash &hash, std::vector<Photon> &photons);

    void set_emit_photons_args(cl::Kernel &kernel, u32 batch_offset, u32 photons_to_emit, u32 batch_max_photons,
                               u32 seed, cl::Buffer &out_photons, cl::Buffer &out_photon_count) const;

    void set_trace_rays_args(cl::Kernel &kernel, f32 search_radius, u32 samples, PhotonHashInfo info, u32 seed,
                             cl::Buffer &out_color) const;
};

#endif // PHOSPHOR_SCENE_BUFFERS_HPP
