#ifndef PHOSPHOR_CAMERA_HPP
#define PHOSPHOR_CAMERA_HPP

#include "glm_bundle.hpp"
#include "random.h"
#include "ray.h"
#include "typedefs.h"

#include <vector>

constexpr f32 DEFAULT_CAMERA_HFOV = 60.0f;
constexpr f32 DEFAULT_CAMERA_RATIO = 16.0f / 9.0f;

struct Camera {
    vec3 position;
    vec3 target;
    vec3 up;

    vec3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    vec3 defocus_disk_u, defocus_disk_v;

    f32 hfov;
    f32 aspect_ratio;

    f32 defocus_angle;
    f32 focus_distance;

    Camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect);
    void recalculate();

    void focus(f32 defocus_angle, f32 focus_distance);
    vec3 get_ray_origin(RngState &rng) const;
    Ray get_ray(RngState &rng, f32 s, f32 t) const;

    std::pair<std::vector<float4>, std::vector<float4>> generate_rays(RngState &rng, u32 image_width, u32 image_height,
                                                                      u32 iters) const;
};

#endif // PHOSPHOR_CAMERA_HPP
