#ifndef PHOSPHOR_TRIANGLE_H
#define PHOSPHOR_TRIANGLE_H

#include "common/constants.h"
#include "common/typedefs.h"

typedef struct GPU_ALIGN TriangleVertices {
    float4 v0, v1, v2;
    // 3 * 4 * 4 = 48
} TriangleVertices;

typedef struct GPU_ALIGN Triangle {
    float4 v0, v1, v2;
    float4 n0, n1, n2;
    float4 t0, t1, t2;
    // 9 * 4 * 4 = 144

    float2 uv0, uv1, uv2;
    // 3 * 2 * 4 = 24

    u32 mat_index;
    // 1 * 4 = 4

    // total: 172
    u8 _padding[4];
} Triangle;

#endif // PHOSPHOR_TRIANGLE_H
