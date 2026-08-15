#ifndef PHOSPHOR_MATERIAL_H
#define PHOSPHOR_MATERIAL_H

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

#endif // PHOSPHOR_MATERIAL_H
