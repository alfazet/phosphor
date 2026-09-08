#include "camera.hpp"

#include "logger.hpp"
#include "random.h"

void Camera::recalculate() {
    vec3 w_dir = normalize(position - this->target);
    vec3 u_dir = normalize(cross(up, w_dir));
    vec3 v_dir = cross(w_dir, u_dir);
    f32 hfov_rad = glm::radians(this->hfov);
    f32 half_width = glm::tan(hfov_rad * 0.5f) * focus_distance;
    f32 half_height = half_width / this->aspect_ratio;
    vec3 horizontal = 2.0f * half_width * u_dir;
    vec3 vertical = 2.0f * half_height * v_dir;
    vec3 lower_left = position - 0.5f * horizontal - 0.5f * vertical - w_dir * focus_distance;
    this->horizontal = horizontal;
    this->vertical = vertical;
    this->lower_left_corner = lower_left;
    this->u = u_dir;
    this->v = v_dir;
    this->w = w_dir;
}

Camera::Camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect) {
    this->position = position;
    this->target = look_at;
    this->up = up;
    this->hfov = hfov_deg;
    this->aspect_ratio = aspect;
    this->defocus_angle = 0.0;
    this->focus_distance = 1.0;
    recalculate();
}

void Camera::focus(f32 defocus_angle, f32 focus_distance) {
    this->defocus_angle = defocus_angle;
    this->focus_distance = focus_distance;
    f32 defocus_radius = focus_distance * std::tan(glm::radians(defocus_angle / 2.0f));
    this->defocus_disk_u = this->u * defocus_radius;
    this->defocus_disk_v = this->v * defocus_radius;
    recalculate();
}

vec3 Camera::get_ray_origin(RngState &rng) const {
    vec3 disk_offset = vec3{0.0f, 0.0f, 0.0f};
    if (defocus_angle > 0.0) {
        f32 r1 = sqrt(random_float(&rng));
        f32 r2 = 2.0f * PI * random_float(&rng);
        f32 disk_offset_cos = std::cos(r2);
        f32 disk_offset_sin = std::sin(r2);
        disk_offset = r1 * (disk_offset_cos * this->defocus_disk_u + disk_offset_sin * this->defocus_disk_v);
    }
    return position + disk_offset;
}

Ray Camera::get_ray(RngState &rng, f32 s, f32 t) const {
    vec3 position = get_ray_origin(rng);
    vec3 direction = lower_left_corner + s * horizontal + t * vertical - position;
    vec3 dir_n = normalize(direction);
    Ray r{};
    r.origin = float4{{position.x, position.y, position.z, 0.0f}};
    r.dir = float4{{dir_n.x, dir_n.y, dir_n.z, 0.0f}};

    return r;
}

std::pair<std::vector<float4>, std::vector<float4>> Camera::generate_rays(RngState &rng, u32 image_width,
                                                                          u32 image_height, u32 image_iters) const {
    std::vector<float4> origins(image_width * image_height * image_iters);
    std::vector<float4> dirs(image_width * image_height * image_iters);
    for (u32 y = 0; y < image_height; y++) {
        for (u32 x = 0; x < image_width; x++) {
            for (u32 j = 0; j < image_iters; j++) {
                const f32 s = (x + 0.5f + random_float(&rng) - 0.5f) / static_cast<f32>(image_width);
                const f32 t = 1.0f - (y + 0.5f + random_float(&rng) - 0.5f) / static_cast<f32>(image_height);
                Ray r = this->get_ray(rng, s, t);
                u32 idx = (y * image_width + x) * image_iters + j;
                origins[idx] = r.origin;
                dirs[idx] = r.dir;
            }
        }
    }

    return {origins, dirs};
}
