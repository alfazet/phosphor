#ifndef PHOSPHOR_RANDOM_H
#define PHOSPHOR_RANDOM_H

#include "constants.h"
#include "typedefs.h"
#include "utils.h"

typedef struct GPU_ALIGN RngState {
    u32 state;
    // 1 * 4 = 4

    // total: 4
    u8 _padding[12];
} RngState;

// values from https://github.com/imneme/pcg-c/blob/master/include/pcg_variants.h
inline u32 pcg_random(RngState *rng) {
    u32 oldstate = rng->state;
    rng->state = rng->state * 747796405u + 2891336453u;

    return (((oldstate >> ((oldstate >> 28u) + 4u)) ^ oldstate) * 277803737u) >> 16u;
}

inline RngState pcg_seed(u32 seed) {
    RngState rng;
    rng.state = 0u;
    rng.state = rng.state * 747796405u + 2891336453u;
    rng.state += seed;
    rng.state = rng.state * 747796405u + 2891336453u;

    return rng;
}

inline RngState make_thread_rng(RngState base, u32 thread_index) { return pcg_seed(base.state + thread_index); }

inline f32 random_float(RngState *rng) { return (f32)pcg_random(rng) / 65536.0f; }

#ifdef __OPENCL_C_VERSION__

inline float4 random_unit_vector(RngState *rng) {
    float4 p;
    do {
        p.x = 2.0f * random_float(rng) - 1.0f;
        p.y = 2.0f * random_float(rng) - 1.0f;
        p.z = 2.0f * random_float(rng) - 1.0f;
        p.w = 0.0f;
    } while (dot(p, p) >= 1.0f);

    f32 len = sqrt(dot(p, p));
    p.x /= len;
    p.y /= len;
    p.z /= len;

    return p;
}

inline float4 random_in_unit_hemisphere(RngState *rng, float4 normal) {
    normal.w = 0.0f;
    f32 r1 = random_float(rng);
    f32 r2 = random_float(rng);
    f32 phi = 2.0f * PI * r1;
    f32 sin_theta = sqrt(r2);
    f32 cos_theta = sqrt(1.0f - r2);

    float4 hemi_local = (float4)(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta, 0.0f);

    float4 tangent, bitangent;
    make_tbn(normal, &tangent, &bitangent);

    return normalize(hemi_local.x * tangent + hemi_local.y * bitangent + hemi_local.z * normal);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_RANDOM_H
