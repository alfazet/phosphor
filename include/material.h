#ifndef PHOSPHOR_MATERIAL_H
#define PHOSPHOR_MATERIAL_H

#include "constants.h"
#include "typedefs.h"

typedef struct GPU_ALIGN Material {
    float4 base_color;
    float4 emissive;
    float4 att_color;
    // 3 * 4 * 4 = 48

    float2 uv_offset;
    float2 uv_scale;
    // 2 * 2 * 4 = 16

    f32 metallic;
    f32 roughness;
    f32 transmission;
    f32 ior;
    f32 att_dist;
    f32 thickness;
    f32 uv_rotation;
    // 7 * 4 = 28

    u32 diff_index;
    u32 emis_index;
    u32 norm_index;
    u32 occlusion_index;
    u32 metal_rough_index;
    u32 trans_tex_index;
    // 6 * 4 = 24

    // total: 116
    u8 _padding[12];
} Material;

#ifdef __OPENCL_C_VERSION__

#include "constants.h"
#include "texture_meta.h"

inline float2 apply_uv_transform(float2 uv, float2 offset, float2 scale, f32 rotation) {
    float2 scaled = uv * scale;
    f32 cos_r = cos(rotation);
    f32 sin_r = sin(rotation);
    float2 rotated = (float2)(cos_r * scaled.x - sin_r * scaled.y, sin_r * scaled.x + cos_r * scaled.y);

    return rotated + offset;
}

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

inline float4 sample_texture_uv(const Material *mat, __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                                u32 index, float2 uv) {
    float2 transformed = apply_uv_transform(uv, mat->uv_offset, mat->uv_scale, mat->uv_rotation);
    return sample_texture(tex_meta, tex_atlas, index, transformed);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_MATERIAL_H
