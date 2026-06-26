#ifndef PHOSPHOR_CAMERA_HPP
#define PHOSPHOR_CAMERA_HPP

#include "common.hpp"
#include "ray.hpp"

class Camera {
  public:
    Camera() = default;
    Camera(vec3 position, vec3 target, vec3 up, f32 vfov_degrees, f32 aspect_ratio=16.0/9.0);

    Ray get_ray(f32 s, f32 t) const;

    void set_position(const vec3 &position);
    const vec3 &position() const { return position_; }
    const f32 aspect_ratio() const { return aspect_ratio_; }

  private:
    void update();

    vec3 position_;
    vec3 target_;
    vec3 up_;
    f32 vfov_;
    f32 aspect_ratio_;

    vec3 lower_left_corner_{};
    vec3 horizontal_{};
    vec3 vertical_{};
    vec3 u_{}, v_{}, w_{};
};

#endif // PHOSPHOR_CAMERA_HPP