#ifndef PHOSPHOR_CAMERA_HPP
#define PHOSPHOR_CAMERA_HPP

#include "camera.h"
#include "glm_bundle.hpp"
#include "random.h"
#include "ray.h"
#include "typedefs.h"

#include <vector>

Camera make_camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect);

Ray get_camera_ray(const Camera &cam, f32 s, f32 t);

void generate_primary_rays(const Camera &camera, RngState &rng, u32 image_width, u32 image_height, u32 iters,
                           std::vector<float4> &origins, std::vector<float4> &directions);

#endif // PHOSPHOR_CAMERA_HPP
