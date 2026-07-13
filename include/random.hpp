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

inline f32 random_float(RngState &rng) { return static_cast<f32>(pcg_random(rng)) / static_cast<f32>(UINT32_MAX); }

inline void pcg_seed(RngState &rng, u32 seed) {
    rng.state = 0u;
    rng.state = rng.state * 747796405u + 2891336453u;
    rng.state += seed;
    rng.state = rng.state * 747796405u + 2891336453u;
}

inline vec3 random_in_unit_sphere(RngState &rng) {
    vec3 p;
    do {
        p = 2.0f * vec3(random_float(rng), random_float(rng), random_float(rng)) - vec3(1.0f);
    } while (glm::dot(p, p) >= 1.0f);
    return p;
}

inline vec3 random_unit_vector(RngState &rng) { return glm::normalize(random_in_unit_sphere(rng)); }

inline vec3 random_in_hemisphere(RngState &rng, const vec3 &normal) {
    const vec3 v = random_unit_vector(rng);
    return (glm::dot(v, normal) > 0.0f) ? v : -v;
}

#endif // PHOSPHOR_RANDOM_HPP
