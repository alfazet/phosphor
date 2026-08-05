#ifndef PHOSPHOR_RAY_HPP
#define PHOSPHOR_RAY_HPP

#include "common.hpp"

struct Ray {
    vec3 origin{0.0f};
    vec3 direction{0.0f};

    // notation and all equations
    // related to ray differantials taken from
    // "Tracing Ray Differentials", Igehy 1999
    vec3 dp_dx{0.0f};
    vec3 dd_dx{0.0f};
    vec3 dp_dy{0.0f};
    vec3 dd_dy{0.0f};

    explicit Ray() = default;

    Ray(vec3 origin, vec3 direction) : origin(origin), direction(direction) {}

    Ray(vec3 origin, vec3 direction, vec3 dp_dx, vec3 dd_dx, vec3 dp_dy, vec3 dd_dy)
        : origin(origin), direction(direction), dp_dx(dp_dx), dd_dx(dd_dx), dp_dy(dp_dy), dd_dy(dd_dy) {}

    vec3 at(f32 t) const;

    f32 compute_uv_lod(f32 t, const vec3 &normal, const vec3 &tangent, const vec3 &bitangent, i32 tex_width,
                       i32 tex_height) const;

    vec3 dpx_at(f32 t, const vec3 &normal) const;
    vec3 dpy_at(f32 t, const vec3 &normal) const;

    vec3 reflect_dd_dx(const vec3 &normal) const;
    vec3 reflect_dd_dy(const vec3 &normal) const;

    vec3 refract_dd_dx(const vec3 &normal, const vec3 &incident_dir, f32 eta) const;
    vec3 refract_dd_dy(const vec3 &normal, const vec3 &incident_dir, f32 eta) const;
};

#endif // PHOSPHOR_RAY_HPP
