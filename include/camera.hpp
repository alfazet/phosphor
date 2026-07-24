#ifndef PHOSPHOR_CAMERA_HPP
#define PHOSPHOR_CAMERA_HPP

#include "common.hpp"
#include "ray.hpp"
#include "logger.hpp"
#include "random.hpp"

constexpr f32 DEFAULT_CAMERA_HFOV = 60.0;
constexpr f32 DEFAULT_CAMERA_RATIO = 16.0 / 9.0;

struct Camera {
    vec3 position;
    vec3 target;
    vec3 up;

    vec3 lower_left_corner{};
    vec3 horizontal{};
    vec3 vertical{};
    vec3 u{}, v{}, w{};

    f32 hfov;
    f32 aspect_ratio;

    Camera(vec3 position, vec3 target, vec3 up, f32 hfov_degrees, f32 aspect_ratio);
    Camera(const vec3 &minp, const vec3 &maxp, f32 hfov_degrees, f32 aspect_ratio);

    Ray get_ray(RngState& rng, f32 s, f32 t) const;

    void set_position(const vec3 &position);

    friend void print_camera(const Camera &camera);



    void update();


};

#endif // PHOSPHOR_CAMERA_HPP
