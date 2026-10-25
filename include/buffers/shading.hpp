#ifndef PHOSPHOR_SHADING_BUFFERS_HPP
#define PHOSPHOR_SHADING_BUFFERS_HPP

#include "common/bounding_box.h"
#include "common/typedefs.h"
#include "host/opencl_ctx.hpp"
#include "host/scene_data.hpp"

struct ShadingBuffers {
    cl::Buffer d_materials;
    u32 n_materials = 0;

    cl::Buffer d_lights;
    cl::Buffer d_lum_pref_sum;
    u32 n_lights = 0;
    f32 total_lum = 0.0f;

    cl::Buffer d_tex_atlas;
    cl::Buffer d_tex_meta;
    u32 n_textures = 0;

    float4 scene_center{};
    f32 scene_radius = 0.0f;

    cl::Buffer d_envmap_data;
    u32 envmap_width = 0;
    u32 envmap_height = 0;
    u32 has_envmap = 0;

    void upload(ClContext &ctx, const SceneData &scene, const BoundingBox &scene_bbox);
};

#endif // PHOSPHOR_SHADING_BUFFERS_HPP
