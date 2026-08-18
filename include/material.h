#ifndef PHOSPHOR_MATERIAL_H
#define PHOSPHOR_MATERIAL_H

#include "constants.h"
#include "texture_meta.h"
#include "typedefs.h"

typedef struct Material {
    float4 base_color;
    float4 emissive;
    // 2 * 4 * 4 = 32

    f32 metallic;
    f32 roughness;
    f32 transmission;
    f32 ior;
    // 4 * 4 = 16

    u32 diff_index;
    u32 emis_index;
    u32 norm_index;
    u32 occlusion_index;
    u32 metal_rough_index;
    // 5 * 4 = 20

    float2 uv_offset;
    float2 uv_scale;
    f32 uv_rotation;
    // 2 * 4 + 2 * 4 + 4 = 20

    // total: 88
    u8 _padding[8];
} Material __attribute__((aligned(16)));

#ifdef __OPENCL_C_VERSION__
// TODO: doesnt use transforms
inline float4 sample_texture(__global const TextureMeta *tex_meta, __global const u8 *tex_atlas, u32 index, float2 uv) {
    if (index == NO_TEXTURE)
        return (float4)(1.0f, 1.0f, 1.0f, 1.0f);

    TextureMeta meta = tex_meta[index];

    f32 u_wrapped = uv.x - floor(uv.x);
    f32 v_wrapped = uv.y - floor(uv.y);
    u32 px = min((u32)(u_wrapped * (f32)meta.width), meta.width - 1);
    u32 py = min((u32)(v_wrapped * (f32)meta.height), meta.height - 1);

    u32 texel = meta.atlas_offset + (py * meta.width + px) * 3u;
    f32 r = (f32)tex_atlas[texel + 0] / 255.0f;
    f32 g = (f32)tex_atlas[texel + 1] / 255.0f;
    f32 b = (f32)tex_atlas[texel + 2] / 255.0f;

    return (float4)(r, g, b, 1.0f);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_MATERIAL_H
