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

#ifndef __OPENCL_C_VERSION__
#include <bit>

inline f32 as_float(u32 x) { return std::bit_cast<f32>(x); }
inline u32 as_uint(f32 x) { return std::bit_cast<u32>(x); }

#endif // __OPENCL_C_VERSION__

#ifndef __OPENCL_C_VERSION__
#include "glm_bundle.hpp"
inline void make_tbn(const vec3 &n, vec3 &t, vec3 &b) {
    if (glm::abs(n.x) > glm::abs(n.y)) {
        // n crossed with (0, 1, 0)
        t = glm::normalize(vec3(-n.z, 0.0f, n.x));
    } else {
        // n crossed with (1, 0, 0)
        t = glm::normalize(vec3(0.0f, n.z, -n.y));
    }
    b = glm::cross(n, t);
}
#endif // __OPENCL_C_VERSION__

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
