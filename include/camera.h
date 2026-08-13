#ifndef PHOSPHOR_CAMERA_H
#define PHOSPHOR_CAMERA_H

#include "types.h"

#define DEFAULT_CAMERA_HFOV = 60.0f;
#define DEFAULT_CAMERA_RATIO = (16.0f / 9.0f);

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
};

#endif // PHOSPHOR_CAMERA_H
