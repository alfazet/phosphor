#ifndef PHOSPHOR_RANDOM_H
#define PHOSPHOR_RANDOM_H

#include "typedefs.h"

typedef struct RngState {
    u32 state;
    // 1 * 4 = 4

    // total: 4
    u8 _padding[12];
} RngState __attribute__((aligned(16)));

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

inline f32 random_float(RngState *rng) { return (f32)pcg_random(rng) / 65535.0f; }

#ifdef __OPENCL_C_VERSION__

inline float4 random_unit_vector(RngState *rng) {
    float4 p;
    do {
        p.x = 2.0f * random_float(rng) - 1.0f;
        p.y = 2.0f * random_float(rng) - 1.0f;
        p.z = 2.0f * random_float(rng) - 1.0f;
        p.w = 0.0f;
    } while (dot(p.xyz, p.xyz) >= 1.0f);

    float len = sqrt(dot(p.xyz, p.xyz));
    p.x /= len;
    p.y /= len;
    p.z /= len;

    return p;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_RANDOM_H
