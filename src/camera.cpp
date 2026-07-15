#include "camera.hpp"

// TODO: some gltf models don't have a specified camera, so to display those we need a default for now
// (camera position, light positions, powers, colors etc. will be parsed from a config file later on)
Camera::Camera()
    : position_{vec3(1, 1, 1)}, target_{vec3(0, 0, 0)}, up_{vec3(0, 1, 0)}, hfov_{60}, aspect_ratio_{16.0 / 9.0} {
    update();
}

Camera::Camera(vec3 position, vec3 target, vec3 up, f32 hfov_degrees, f32 aspect_ratio)
    : position_(position), target_(target), up_(up), hfov_(hfov_degrees), aspect_ratio_(aspect_ratio) {
    update();
}

void Camera::update() {
    if (hfov_ <= 0 || hfov_ >= 180)
        throw std::invalid_argument("hfov must be between 0 and 180 degrees exclusive");
    if (aspect_ratio_ <= 0)
        throw std::invalid_argument("aspect_ratio must be positive");
    const f32 w = glm::tan(glm::radians(hfov_) / 2.0f);
    const f32 viewport_width = 2.0f * w;
    const f32 viewport_height = viewport_width / aspect_ratio_;

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
