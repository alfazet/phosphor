#include "camera.hpp"

Camera::Camera(vec3 position, vec3 target, vec3 up, f32 hfov_degrees, f32 aspect_ratio)
    : position_(position), target_(target), up_(up), hfov(hfov_degrees), aspect_ratio(aspect_ratio) {
    update();
}

Camera::Camera(const vec3 &minp, const vec3 &maxp, f32 hfov_degrees, f32 aspect_ratio)
    : hfov(hfov_degrees), aspect_ratio(aspect_ratio) {
    const vec3 center = (minp + maxp) * 0.5f;
    const vec3 size = maxp - minp;

    // inset fraction from the corner so we don't sit exactly on a wall
    constexpr f32 inset_fraction = 0.05f;
    const vec3 inset = size * inset_fraction;

    // clamp inset so tiny scenes don't put the camera on top of the target
    const f32 min_size = glm::max(glm::max(size.x, size.y), size.z) * 0.01f + 0.001f;
    const vec3 safe_inset = glm::max(inset, vec3(min_size));

    target_ = center;
    position_ = maxp - safe_inset;
    up_ = vec3(0, 1, 0);

    update();
}

void Camera::update() {
    ASSERT(hfov > 0 && hfov < 180, "hfov must be between 0 and 180 degrees exclusive");
    ASSERT(aspect_ratio > 0, "aspect_ratio must be positive");

    const f32 w = glm::tan(glm::radians(hfov) / 2.0f);
    const f32 viewport_width = 2.0f * w;
    const f32 viewport_height = viewport_width / aspect_ratio;

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

Ray Camera::get_ray(RngState &rng, f32 s, f32 t) const {
    const vec3 direction = lower_left_corner_ + s * horizontal_ + t * vertical_ - position_;
    return Ray(position_, glm::normalize(direction));
}
