#include "camera.hpp"

Camera::Camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect) {
    vec3 w_dir = normalize(position - look_at);
    vec3 u_dir = normalize(cross(up, w_dir));
    vec3 v_dir = cross(w_dir, u_dir);

    f32 hfov_rad = glm::radians(hfov_deg);
    f32 half_width = glm::tan(hfov_rad * 0.5f);
    f32 half_height = half_width / aspect;

    vec3 horizontal = 2.0f * half_width * u_dir;
    vec3 vertical = 2.0f * half_height * v_dir;
    vec3 lower_left = position - 0.5f * horizontal - 0.5f * vertical - w_dir;

    this->position = vec3_to_float4(position);
    this->target = vec3_to_float4(look_at);
    this->up = vec3_to_float4(up);
    this->lower_left_corner = vec3_to_float4(lower_left);
    this->horizontal = vec3_to_float4(horizontal);
    this->vertical = vec3_to_float4(vertical);
    this->u = vec3_to_float4(u_dir);
    this->v = vec3_to_float4(v_dir);
    this->w = vec3_to_float4(w_dir);
    this->hfov = hfov_deg;
    this->aspect_ratio = aspect;
}

Ray Camera::get_ray(f32 s, f32 t) const {
    vec3 position(this->position.x, this->position.y, this->position.z);
    vec3 lower_left_corner(this->lower_left_corner.x, this->lower_left_corner.y, this->lower_left_corner.z);
    vec3 horizontal(this->horizontal.x, this->horizontal.y, this->horizontal.z);
    vec3 vertical(this->vertical.x, this->vertical.y, this->vertical.z);
    vec3 direction = lower_left_corner + s * horizontal + t * vertical - position;
    vec3 dir_n = normalize(direction);

    Ray r{};
    r.origin = float4{{position.x, position.y, position.z, 0.0f}};
    r.dir = float4{{dir_n.x, dir_n.y, dir_n.z, 0.0f}};

    return r;
}

CameraParams Camera::to_params() const {
    CameraParams p;
    p.position = this->position;
    p.lower_left_corner = this->lower_left_corner;
    p.horizontal = this->horizontal;
    p.vertical = this->vertical;

    return p;
}
