#ifndef PHOSPHOR_HITPOINT_H
#define PHOSPHOR_HITPOINT_H

#include "constants.h"
#include "typedefs.h"

// represents the first diffuse surface hit along a camera ray
typedef struct GPU_ALIGN HitPoint {
    float4 position;
    float4 normal; // geometric
    float4 throughput;
    float4 base_color;
    float4 direct;
    float4 emission;
    // 6 * 16 = 96

    f32 metallic;
    u32 is_valid;
    // 2 * 4 = 8

    // total: 104
    u8 _padding[8];
} HitPoint;

#endif // PHOSPHOR_HITPOINT_H
