#ifndef PHOSPHOR_CAMERA_H
#define PHOSPHOR_CAMERA_H

#include "typedefs.h"

#define DEFAULT_CAMERA_HFOV = 60.0f;
#define DEFAULT_CAMERA_RATIO = (16.0f / 9.0f);

typedef struct Camera {
    float4 position;
    float4 target;
    float4 up;

    float4 lower_left_corner;
    float4 horizontal;
    float4 vertical;
    float4 u, v, w;
    // 9 * 4 * 4 = 144

    f32 hfov;
    f32 aspect_ratio;
    // 2 * 4 = 8

    // total: 152
    u8 _padding[8];
} Camera __attribute__((aligned(16)));

#endif // PHOSPHOR_CAMERA_H
