#ifndef PHOSPHOR_RNG_HPP
#define PHOSPHOR_RNG_HPP

#include "common.hpp"

#include <cstdint>

struct RngState {
    u32 state;
};

// values from https://github.com/imneme/pcg-c/blob/master/include/pcg_variants.h
inline u32 pcg_random(RngState &rng) {
    u32 oldstate = rng.state;
    rng.state = rng.state * 747796405u + 2891336453u;
    return (((oldstate >> ((oldstate >> 28u) + 4u)) ^ oldstate) * 277803737u) >> 16u;
}

inline f32 random_float(RngState &rng) {
    return static_cast<f32>(pcg_random(rng)) / static_cast<f32>(UINT32_MAX);
}

inline void pcg_oneseq_32_srandom_r(RngState &rng, u32 seed) {
    rng.state = 0u;
    rng.state = rng.state * 747796405u + 2891336453u;
    rng.state += seed;
    rng.state = rng.state * 747796405u + 2891336453u;
}

#endif // PHOSPHOR_RNG_HPP
