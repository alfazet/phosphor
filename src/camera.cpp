#include "camera.hpp"

Camera::Camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect) {
    this->position = position;
    this->target = look_at;
    this->up = up;
    this->hfov = hfov_deg;
    this->aspect_ratio = aspect;
    this->defocus_angle = 0.0;
    this->focus_distance = 1.0;

    this->recalculate();
}

void Camera::focus(f32 defocus_angle, f32 focus_distance) {
    this->defocus_angle = defocus_angle;
    this->focus_distance = focus_distance;
    f32 defocus_radius = focus_distance * std::tan(glm::radians(defocus_angle / 2.0f));
    this->defocus_disk_u = this->u * defocus_radius;
    this->defocus_disk_v = this->v * defocus_radius;

    this->recalculate();
}

void Camera::recalculate() {
    vec3 w_dir = glm::normalize(position - this->target);
    vec3 u_dir = glm::normalize(cross(up, w_dir));
    vec3 v_dir = glm::cross(w_dir, u_dir);
    f32 hfov_rad = glm::radians(this->hfov);
    f32 half_width = glm::tan(hfov_rad * 0.5f) * this->focus_distance;
    f32 half_height = half_width / this->aspect_ratio;
    vec3 horizontal = 2.0f * half_width * u_dir;
    vec3 vertical = 2.0f * half_height * v_dir;
    vec3 lower_left = this->position - 0.5f * horizontal - 0.5f * vertical - w_dir * this->focus_distance;

    this->horizontal = horizontal;
    this->vertical = vertical;
    this->lower_left_corner = lower_left;
    this->u = u_dir;
    this->v = v_dir;
    this->w = w_dir;
}

CameraParams Camera::to_params() const {
    CameraParams p;
    p.position = vec3_to_float4(this->position);
    p.lower_left_corner = vec3_to_float4(this->lower_left_corner);
    p.horizontal = vec3_to_float4(this->horizontal);
    p.vertical = vec3_to_float4(this->vertical);
    p.defocus_disk_u = vec3_to_float4(this->defocus_disk_u);
    p.defocus_disk_v = vec3_to_float4(this->defocus_disk_v);
    p.defocus_angle = this->defocus_angle;
    p.focus_distance = this->focus_distance;

    return p;
}
