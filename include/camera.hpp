#ifndef PHOSPHOR_CAMERA_HPP
#define PHOSPHOR_CAMERA_HPP

#include "common.hpp"
#include "ray.hpp"

constexpr f32 DEFAULT_CAMERA_HFOV = 60.0;
constexpr f32 DEFAULT_CAMERA_RATIO = 16.0 / 9.0;

class Camera {
  public:
    Camera(vec3 position, vec3 target, vec3 up, f32 hfov_degrees, f32 aspect_ratio);
    Camera(const vec3 &minp, const vec3 &maxp, f32 hfov_degrees, f32 aspect_ratio);

    Ray get_ray(f32 s, f32 t) const;

    void set_position(const vec3 &position);
    const vec3 &position() const { return position_; }

    friend void print_camera(const Camera &camera);

    f32 hfov;
    f32 aspect_ratio;

  private:
    void update();

    vec3 position_;
    vec3 target_;
    vec3 up_;

    vec3 lower_left_corner_{};
    vec3 horizontal_{};
    vec3 vertical_{};
    vec3 u_{}, v_{}, w_{};
};

#endif // PHOSPHOR_CAMERA_HPP
