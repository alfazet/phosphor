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
    float4 position;
    float4 target;
    float4 up;

    float4 lower_left_corner;
    float4 horizontal;
    float4 vertical;
    float4 u, v, w;

    f32 hfov;
    f32 aspect_ratio;

    Camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect);

    Ray get_ray(f32 s, f32 t) const;

    std::pair<std::vector<float4>, std::vector<float4>> generate_rays(RngState &rng, u32 image_width, u32 image_height,
                                                                      u32 iters) const;
};

#endif // PHOSPHOR_CAMERA_HPP
