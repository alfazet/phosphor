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

// https://www.graphics.cornell.edu/%7Ebjw/rgbe/rgbe.c
u32 encodeRGBE(float4 power) {
    f32 m = fmax(power.x, fmax(power.y, power.z));
    if (m <= 0.0f)
        return 0u;

    i32 e;
    frexp(m, &e);               // m = frac * 2^e, frac in [0.5, 1)
    f32 scale = ldexp(1.0f, e); // 2^e

    uint r = (uint)(power.x / scale * 256.0f);
    uint g = (uint)(power.y / scale * 256.0f);
    uint b = (uint)(power.z / scale * 256.0f);
    uint E = (uint)(e + 128);

    r = min(r, 255u);
    g = min(g, 255u);
    b = min(b, 255u);

    return r | (g << 8) | (b << 16) | (E << 24);
}

float4 decodeRGBE(u32 rgbe) {
    u32 E = rgbe >> 24;
    if (E == 0)
        return (float4)(0.0f);

    f32 scale = ldexp(1.0f, (i32)E - 128 - 8); // 2^(E-128) / 256

    return (float4)((rgbe & 0xFF) * scale, ((rgbe >> 8) & 0xFF) * scale, ((rgbe >> 16) & 0xFF) * scale, 0.0f);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_UTILS_H
