#ifndef PHOSPHOR_CAMERA_HPP
#define PHOSPHOR_CAMERA_HPP

#include "common/camera.h"
#include "common/typedefs.h"
#include "host/glm_bundle.hpp"

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

    void focus(f32 defocus_angle, f32 focus_distance);

    void recalculate();

    CameraParams to_params() const;
};

#endif // PHOSPHOR_CAMERA_HPP
