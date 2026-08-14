#ifndef PHOSPHOR_LIGHT_H
#define PHOSPHOR_LIGHT_H

#include "typedefs.h"

#define LIGHT_POINT 0
#define LIGHT_SPOT 1
#define LIGHT_DIRECTIONAL 2
#define LIGHT_TEXTURED 3

typedef struct Light {
    float4 position;
    float4 power;
    float4 direction;
    float4 tangent;
    float4 bitangent;
    float4 origin;
    float4 aux;
    // spot: .x = inner cone angle, .y = outer cone angle
    // directional: .x = radius
    // textured: .x = (as u32) tex_index, .y = (as u32) tri_start, .z = (as u32) tri_count
    // 7 * 4 * 4 = 112

    u8 kind;
    // 1 * 1 = 1

    // total: 113
    u8 _padding[15];
} Light __attribute__((aligned(16)));

#endif // PHOSPHOR_LIGHT_H
