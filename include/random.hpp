#ifndef PHOSPHOR_RANDOM_HPP
#define PHOSPHOR_RANDOM_HPP

#include "common.hpp"
#include <cstdlib>

inline f32 random_float() {
    // Source - https://stackoverflow.com/a/686373
    // Posted by John Dibling, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-06-21, License - CC BY-SA 3.0
    return static_cast<f32>(rand()) / static_cast<f32>(RAND_MAX);
}

inline vec3 random_in_unit_sphere() {
    vec3 p;
    do {
        p = 2.0f * vec3(random_float(), random_float(), random_float()) - vec3(1.0f);
    } while (glm::dot(p, p) >= 1.0f);
    return p;
}

inline vec3 random_unit_vector() { return glm::normalize(random_in_unit_sphere()); }

inline vec3 random_in_hemisphere(const vec3 &normal) {
    const vec3 v = random_unit_vector();
    return (glm::dot(v, normal) > 0.0f) ? v : -v;
}

#endif // PHOSPHOR_RANDOM_HPP