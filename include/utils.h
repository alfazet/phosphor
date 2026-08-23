#ifndef PHOSPHOR_UTILS_H
#define PHOSPHOR_UTILS_H

#include "typedefs.h"

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline u32 round_up_to_pow2(u32 x) {
    if (x == 0)
        return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
}

// reinterprets the bits of a 32 bit unsigned int as a float
inline f32 bits_as_float(u32 x) { return *(f32 *)(&x); }

#ifdef __OPENCL_C_VERSION__

inline void make_tbn(float4 normal, float4 *tangent, float4 *bitangent) {
    normal.w = 0.0f;
    float4 tmp = (fabs(normal.x) > 0.1f) ? (float4)(0.0f, 1.0f, 0.0f, 0.0f) : (float4)(1.0f, 0.0f, 0.0f, 0.0f);

    *tangent = cross(normal, tmp);
    if (length(*tangent) < EPS) {
        tmp = (float4)(0.0f, 0.0f, 1.0f, 0.0f);
        *tangent = cross(normal, tmp);
    }

    *tangent = normalize(*tangent);
    *bitangent = cross(normal, *tangent);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_UTILS_H
