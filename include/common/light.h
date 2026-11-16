#ifndef PHOSPHOR_LIGHT_H
#define PHOSPHOR_LIGHT_H

#include "common/constants.h"
#include "common/material.h"
#include "common/random.h"
#include "common/typedefs.h"

#define LIGHT_POINT 0
#define LIGHT_SPOT 1
#define LIGHT_DIRECTIONAL 2
#define LIGHT_TEXTURED 3

typedef struct GPU_ALIGN Light {
    float4 position;
    float4 power;
    float4 direction;
    float4 tangent;
    float4 bitangent;
    float4 origin;
    float4 aux;
    // spot: .x = inner cone angle, .y = outer cone angle
    // textured: .x = (as u32) tex_index, .y = (as u32) tri_start, .z = (as u32) tri_count, .w = total_area
    // 7 * 4 * 4 = 112

    u8 kind;
    // 1 * 1 = 1

    // total: 113
    u8 _padding[15];
} Light;

#endif // PHOSPHOR_LIGHT_H
