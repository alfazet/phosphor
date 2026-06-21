#ifndef PHOSPHOR_RAY_HPP
#define PHOSPHOR_RAY_HPP

#include "common.hpp"

struct Ray {
    vec3 origin;
    vec3 direction;

    Ray() = default;
    Ray(const vec3 origin, const vec3 direction) : origin(origin), direction(direction) {}

    vec3 at(f32 t) const { return origin + t * direction; }
};

#endif // PHOSPHOR_RAY_HPP