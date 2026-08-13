#ifndef PHOSPHOR_MATERIAL_H
#define PHOSPHOR_MATERIAL_H

#include "typedefs.h"

typedef struct Material {
    float4 base_color;
    float4 emissive;
    // 2 * 4 * 4 = 32

    usize diff_index;
    usize emis_index;
    usize norm_index;
    usize occlusion_index;
    usize metal_rough_index;
    // 5 * 4 = 20

    f32 metallic;
    f32 roughness;
    f32 transmission;
    f32 ior;
    // 4 * 4 = 16

    u8 _padding[12];
} Material __attribute__((aligned(16)));

#endif // PHOSPHOR_MATERIAL_H
