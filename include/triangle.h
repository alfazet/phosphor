#ifndef PHOSPHOR_TRIANGLE_H
#define PHOSPHOR_TRIANGLE_H

#include "typedefs.h"

typedef struct Triangle {
    float4 v0, v1, v2;
    float4 n0, n1, n2;
    float2 uv0, uv1, uv2;
    float4 t0, t1, t2;
    u32 mat_index;
} Triangle;

#endif // PHOSPHOR_TRIANGLE_H
