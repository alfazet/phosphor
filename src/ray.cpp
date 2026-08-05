#include "ray.hpp"

vec3 Ray::at(f32 t) const { return this->origin + t * this->direction; }

f32 Ray::compute_uv_lod(f32 t, const vec3 &normal, const vec3 &tangent, const vec3 &bitangent, i32 tex_width,
                        i32 tex_height) const {
    f32 denom = glm::dot(this->direction, normal);
    if (glm::abs(denom) < EPS)
        return 0.0f;

    f32 dt_x = -glm::dot(this->dp_dx, normal) / denom;
    f32 dt_y = -glm::dot(this->dp_dy, normal) / denom;
    vec3 dpx = this->dp_dx + dt_x * this->direction + t * this->dd_dx;
    vec3 dpy = this->dp_dy + dt_y * this->direction + t * this->dd_dy;

    vec2 duvx = vec2(glm::dot(tangent, dpx), glm::dot(bitangent, dpx));
    vec2 duvy = vec2(glm::dot(tangent, dpy), glm::dot(bitangent, dpy));
    f32 len_x = glm::length(duvx * vec2(tex_width, tex_height));
    f32 len_y = glm::length(duvy * vec2(tex_width, tex_height));

    f32 len_max = glm::max(len_x, len_y);
    if (len_max < EPS)
        return 0.0f;

    return glm::max(0.0f, glm::log2(len_max));
}

vec3 Ray::dpx_at(f32 t, const vec3 &normal) const {
    f32 denom = glm::dot(direction, normal);
    if (glm::abs(denom) < EPS)
        return vec3(0.0f);
    f32 dt = -glm::dot(dp_dx, normal) / denom;

    return dp_dx + dt * direction + t * dd_dx;
}

vec3 Ray::dpy_at(f32 t, const vec3 &normal) const {
    f32 denom = glm::dot(direction, normal);
    if (glm::abs(denom) < EPS)
        return vec3(0.0f);
    f32 dt = -glm::dot(dp_dy, normal) / denom;

    return dp_dy + dt * direction + t * dd_dy;
}

// the equations below assume dN/dx = 0 because that's most often the case (planar surfaces),
// and computing it when it's not the case seems beyond the scope of what we're doing here
// (Igehy, section 3.2)
vec3 reflect_dd(vec3 dd, const vec3 &n) { return dd - 2.0f * glm::dot(dd, n) * n; }

vec3 Ray::reflect_dd_dx(const vec3 &normal) const { return reflect_dd(dd_dx, normal); }

vec3 Ray::reflect_dd_dy(const vec3 &normal) const { return reflect_dd(dd_dy, normal); }

vec3 refract_dd(vec3 dd, const vec3 &d, const vec3 &n, f32 eta) {
    f32 dn_dot = glm::dot(d, n);
    f32 in_sqrt = 1.0f - eta * eta * (1.0f - dn_dot * dn_dot);
    if (in_sqrt <= EPS)
        return reflect_dd(dd, n); // total internal reflection
    f32 dprimen_dot = glm::sqrt(in_sqrt);
    f32 dmu = eta * glm::dot(dd, n) * (1.0f - eta * dn_dot / dprimen_dot);

    return eta * dd - dmu * n;
}

vec3 Ray::refract_dd_dx(const vec3 &normal, const vec3 &incident_dir, f32 eta) const {
    return refract_dd(dd_dx, incident_dir, normal, eta);
}

vec3 Ray::refract_dd_dy(const vec3 &normal, const vec3 &incident_dir, f32 eta) const {
    return refract_dd(dd_dy, incident_dir, normal, eta);
}
