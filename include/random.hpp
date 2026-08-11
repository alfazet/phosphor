#ifndef PHOSPHOR_RANDOM_HPP
#define PHOSPHOR_RANDOM_HPP

#include "common.hpp"

struct RngState {
    u32 state;
};

// values from https://github.com/imneme/pcg-c/blob/master/include/pcg_variants.h
inline u32 pcg_random(RngState &rng) {
    u32 oldstate = rng.state;
    rng.state = rng.state * 747796405u + 2891336453u;
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

inline f32 random_float(RngState &rng) { return static_cast<f32>(pcg_random(rng)) / 65535.0f; }

#endif // PHOSPHOR_RANDOM_HPP
