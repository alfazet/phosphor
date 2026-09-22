#ifndef PHOSPHOR_SCENE_BUFFERS_HPP
#define PHOSPHOR_SCENE_BUFFERS_HPP

#include "bvh.hpp"
#include "camera.h"
#include "hitpoint.h"
#include "opencl_ctx.hpp"
#include "photon_hash.hpp"
#include "scene.hpp"
#include "sppm_pixel.h"
#include "typedefs.h"

#include <vector>

struct SceneBuffers {
    cl::Buffer tri_v0, tri_v1, tri_v2;
    cl::Buffer tri_uv0, tri_uv1, tri_uv2;
    cl::Buffer tri_n0, tri_n1, tri_n2;
    cl::Buffer tri_t0, tri_t1, tri_t2;
    cl::Buffer tri_mat_index;
    u32 n_triangles = 0;

    cl::Buffer etri_v0, etri_v1, etri_v2;
    cl::Buffer etri_uv0, etri_uv1, etri_uv2;
    cl::Buffer etri_n0, etri_n1, etri_n2;
    cl::Buffer etri_t0, etri_t1, etri_t2;
    cl::Buffer etri_mat_index;
    u32 n_e_triangles = 0;

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

    CameraParams camera{};
    u32 image_width = 0;
    u32 image_height = 0;
    u32 n_pixels = 0;

    cl::Buffer hit_points;
    cl::Buffer sppm_pixels;
    cl::Buffer total_irradiance;

    cl::Buffer photon_pos;
    cl::Buffer photon_power;
    cl::Buffer photon_dir;

    cl::Buffer tree_index;
    cl::Buffer bucket_tree_offset;
    cl::Buffer bucket_tree_size;
    u32 n_photons = 0;

    float4 scene_center{};
    f32 scene_radius = 0.0f;

    void set_camera(const CameraParams &cam, u32 width, u32 height);

    void copy_scene(ClContext &ctx, const SceneData &scene, const Bvh &bvh);

    void copy_photons(ClContext &ctx, PhotonHash &hash, std::vector<float4> &photon_pos, std::vector<u32> &photon_power,
                      std::vector<u32> &photon_dir);

    void alloc_sppm_buffers(ClContext &ctx);

    void set_emit_photons_args(cl::Kernel &kernel, u32 batch_offset, u32 photons_to_emit, u32 seed,
                               u32 batch_max_photons, cl::Buffer &out_photon_pos, cl::Buffer &out_photon_power,
                               cl::Buffer &out_photon_dir, cl::Buffer &out_photon_count) const;

    void set_camera_pass_args(cl::Kernel &kernel, u32 seed, u32 direct_samples) const;

    void set_gather_pass_args(cl::Kernel &kernel, PhotonHashInfo info, f32 sppm_alpha) const;

    void print_buffer_sizes() const;
};

#endif // PHOSPHOR_SCENE_BUFFERS_HPP
