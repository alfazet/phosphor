#ifndef PHOSPHOR_MATERIAL_H
#define PHOSPHOR_MATERIAL_H

#include "constants.h"
#include "typedefs.h"

typedef struct UvTransform {
    float2 uv_offset;
    float2 uv_scale;
    f32 uv_rotation;
    // 2 * 2 * 4 + 4 = 20
} UvTransform;

typedef struct GPU_ALIGN Material {
    float4 base_color;
    float4 emissive;
    float4 att_color;
    // 3 * 4 * 4 = 48

    f32 metallic;
    f32 roughness;
    f32 transmission;
    f32 ior;
    f32 att_dist;
    f32 thickness;
    // 6 * 4 = 28

    u32 diff_index;
    u32 emis_index;
    u32 norm_index;
    u32 occlusion_index;
    u32 metal_rough_index;
    u32 trans_tex_index;
    // 6 * 4 = 24

    UvTransform diff_transform;
    UvTransform emis_transform;
    UvTransform norm_transform;
    UvTransform occlusion_transform;
    UvTransform metal_rough_transform;
    UvTransform trans_tex_transform;
    // 6 * 20 = 120

    // total: 220
    u8 _padding[4];
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

inline float4 naive_sample_texture(__global const TextureMeta *tex_meta, __global const u8 *tex_atlas, u32 index,
                                   float2 uv) {
    if (index == NO_TEXTURE)
        return WHITE;

    TextureMeta meta = tex_meta[index];

    u32 px = min((u32)(uv.x * (f32)meta.width), meta.width - 1);
    u32 py = min((u32)(uv.y * (f32)meta.height), meta.height - 1);

    u32 texel = meta.atlas_offset + (py * meta.width + px) * 3u;
    f32 r = (f32)tex_atlas[texel + 0] / 255.0f;
    f32 g = (f32)tex_atlas[texel + 1] / 255.0f;
    f32 b = (f32)tex_atlas[texel + 2] / 255.0f;

    return (float4)(r, g, b, 1.0f);
}

inline float4 sample_texture(__global const TextureMeta *tex_meta, __global const u8 *tex_atlas, u32 index, float2 uv) {
    if (index == NO_TEXTURE)
        return WHITE;

    TextureMeta meta = tex_meta[index];
    i32 w = meta.width;
    i32 h = meta.height;

    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0f - uv.y;

    f32 x = min(uv.x * w, (f32)(w - 1));
    f32 y = min(uv.y * h, (f32)(h - 1));
    i32 x1 = min((i32)(x), w - 1);
    i32 x2 = min((i32)(ceil(x)), w - 1);
    i32 y1 = min((i32)(y), h - 1);
    i32 y2 = min((i32)(ceil(y)), h - 1);

    i32 idx11 = meta.atlas_offset + (y1 * w + x1) * meta.channels;
    i32 idx12 = meta.atlas_offset + (y2 * w + x1) * meta.channels;
    i32 idx21 = meta.atlas_offset + (y1 * w + x2) * meta.channels;
    i32 idx22 = meta.atlas_offset + (y2 * w + x2) * meta.channels;

    float4 Q11 = (float4)((f32)tex_atlas[idx11], (f32)tex_atlas[idx11 + 1], (f32)tex_atlas[idx11 + 2], 255.0f) / 255.0f;
    float4 Q12 = (float4)((f32)tex_atlas[idx12], (f32)tex_atlas[idx12 + 1], (f32)tex_atlas[idx12 + 2], 255.0f) / 255.0f;
    float4 Q21 = (float4)((f32)tex_atlas[idx21], (f32)tex_atlas[idx21 + 1], (f32)tex_atlas[idx21 + 2], 255.0f) / 255.0f;
    float4 Q22 = (float4)((f32)tex_atlas[idx22], (f32)tex_atlas[idx22 + 1], (f32)tex_atlas[idx22 + 2], 255.0f) / 255.0f;

    f32 w11, w12, w21, w22;
    if (x2 == x1 && y2 == y1) {
        return naive_sample_texture(tex_meta, tex_atlas, index, uv);
    } else if (x2 == x1) {
        return mix(Q11, Q12, y - y1);
    } else if (y2 == y1) {
        return mix(Q11, Q21, x - x1);
    } else {
        f32 denom = (x2 - x1) * (y2 - y1);
        w11 = (x2 - x) * (y2 - y) / denom;
        w12 = (x2 - x) * (y - y1) / denom;
        w21 = (x - x1) * (y2 - y) / denom;
        w22 = (x - x1) * (y - y1) / denom;
    }

    return w11 * Q11 + w12 * Q12 + w21 * Q21 + w22 * Q22;
}

inline float4 sample_texture_uv(const Material *mat, __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                                u32 index, float2 uv, UvTransform transform) {
    float2 transformed = apply_uv_transform(uv, transform.uv_offset, transform.uv_scale, transform.uv_rotation);
    return sample_texture(tex_meta, tex_atlas, index, transformed);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_MATERIAL_H
