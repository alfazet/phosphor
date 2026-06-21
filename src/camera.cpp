#include "camera.hpp"

Camera::Camera(vec3 position, vec3 target, vec3 up, f32 vfov_degrees, f32 aspect_ratio)
    : position_(position), target_(target), up_(up), vfov_(vfov_degrees), aspect_ratio_(aspect_ratio) {
    update();
}

void Camera::update() {
    const f32 theta = glm::radians(vfov_);
    const f32 h = glm::tan(theta / 2.0f);
    const f32 viewport_height = 2.0f * h;
    const f32 viewport_width = aspect_ratio_ * viewport_height;

    w_ = glm::normalize(position_ - target_);
    u_ = glm::normalize(glm::cross(up_, w_));
    v_ = glm::cross(w_, u_);

    horizontal_ = viewport_width * u_;
    vertical_ = viewport_height * v_;
    lower_left_corner_ = position_ - horizontal_ / 2.0f - vertical_ / 2.0f - w_;
}

void Camera::set_position(const vec3 &position) {
    position_ = position;
    update();
}

Ray Camera::get_ray(f32 s, f32 t) const {
    const vec3 direction = lower_left_corner_ + s * horizontal_ + t * vertical_ - position_;
    return Ray(position_, glm::normalize(direction));
}
