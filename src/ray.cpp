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
