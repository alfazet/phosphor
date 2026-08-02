#ifndef PHOSPHOR_RAY_HPP
#define PHOSPHOR_RAY_HPP

#include "common.hpp"

struct Ray {
    vec3 origin;
    vec3 direction;

    // notation the same as in
    // "Tracing Ray Differentials", Igehy
    vec3 dp_dx{0.0f}, dd_dx{0.0f};
    vec3 dp_dy{0.0f}, dd_dy{0.0f};

    Ray() = default;
    Ray(vec3 origin, vec3 direction) : origin(origin), direction(direction) {}

    vec3 at(f32 t) const { return origin + t * direction; }
};

#endif // PHOSPHOR_RAY_HPP
