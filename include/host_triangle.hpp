#ifndef PHOSPHOR_HOST_TRIANGLE_HPP
#define PHOSPHOR_HOST_TRIANGLE_HPP

#include "typedefs.h"

struct HostTriangle {
    float4 v0, v1, v2;
    float4 n0, n1, n2;
    float2 uv0, uv1, uv2;
    float4 t0, t1, t2;
    u32 mat_index;
};

#endif // PHOSPHOR_HOST_TRIANGLE_HPP
