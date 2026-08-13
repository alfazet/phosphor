#ifndef PHOSPHOR_TRIANGLE_HPP
#define PHOSPHOR_TRIANGLE_HPP

#include "material.h"
#include "typedefs.h"

struct Triangle {
    float4 v0, v1, v2;
    float4 uv0, uv1, uv2;
    float4 n0, n1, n2;
    float4 t0, t1, t2;
    Material mat;
};

#endif // PHOSPHOR_TRIANGLE_HPP
