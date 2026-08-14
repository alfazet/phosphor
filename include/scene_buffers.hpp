#ifndef PHOSPHOR_SCENE_BUFFERS_HPP
#define PHOSPHOR_SCENE_BUFFERS_HPP

#include "typedefs.h"
#include "opencl_ctx.hpp"

#include <vector>

struct SceneData;

struct SceneBuffers {
    cl::Buffer tri_v0, tri_v1, tri_v2;
    cl::Buffer tri_uv0, tri_uv1, tri_uv2;
    cl::Buffer tri_n0, tri_n1, tri_n2;
    cl::Buffer tri_t0, tri_t1, tri_t2;
    cl::Buffer tri_mat_index;
    u32 tri_count = 0;

    cl::Buffer bvh_nodes;
    cl::Buffer bvh_tri_indices; // index into tri_* buffers
    u32 bvh_node_count = 0;

    cl::Buffer materials;
    u32 mat_count = 0;

    cl::Buffer lights;
    cl::Buffer light_area_prefix;
    u32 light_count = 0;

    cl::Buffer tex_atlas;
    cl::Buffer tex_meta;
    cl::Buffer tex_mip_offsets;
    cl::Buffer tex_mip_dims;
    u32 tex_count = 0;

    cl::Buffer photon_pos;
    cl::Buffer photon_power;
    cl::Buffer photon_dir;

    cl::Buffer camera;

    cl::Buffer framebuffer;
    cl::Buffer output_image;
    u32 image_width = 0;
    u32 image_height = 0;

    void upload_scene(const SceneData &scene, ClContext &ctx);
    void read_image(std::vector<u8> &out, ClContext &ctx);
};

#endif // PHOSPHOR_SCENE_BUFFERS_HPP
