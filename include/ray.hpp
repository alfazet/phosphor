#ifndef PHOSPHOR_RAY_HPP
#define PHOSPHOR_RAY_HPP

#include "common.hpp"

struct Ray {
    vec3 origin{0.0f};
    vec3 direction{0.0f};

    // notation used is the same as in
    // "Tracing Ray Differentials", Igehy 1999
    vec3 dp_dx{0.0f};
    vec3 dd_dx{0.0f};
    vec3 dp_dy{0.0f};
    vec3 dd_dy{0.0f};

    explicit Ray() = default;

    Ray(vec3 origin, vec3 direction) : origin(origin), direction(direction) {}

    vec3 at(f32 t) const;

    f32 compute_uv_lod(f32 t, const vec3 &normal, const vec3 &tangent, const vec3 &bitangent, i32 tex_width,
                       i32 tex_height) const;
};

#endif // PHOSPHOR_RAY_HPP
