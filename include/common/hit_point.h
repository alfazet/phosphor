#ifndef PHOSPHOR_HIT_POINT_H
#define PHOSPHOR_HIT_POINT_H

#include "common/constants.h"
#include "common/typedefs.h"

// represents the first diffuse surface hit along a camera ray
typedef struct GPU_ALIGN HitPoint {
    float4 position;
    float4 normal; // geometric
    float4 throughput;
    float4 base_color;
    float4 direct;
    float4 emission;
    // 6 * 4 * 4 = 96
} HitPoint;

#endif // PHOSPHOR_HIT_POINT_H
