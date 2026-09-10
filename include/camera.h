#ifndef PHOSPHOR_CAMERA_H
#define PHOSPHOR_CAMERA_H

#include "constants.h"
#include "typedefs.h"

typedef struct GPU_ALIGN CameraParams {
    float4 position;
    float4 lower_left_corner;
    float4 horizontal;
    float4 vertical;

    // total: 64
} CameraParams;

#ifdef __OPENCL_C_VERSION__

#include "random.h"

inline void make_ray(const CameraParams *cam, u32 px, u32 py, u32 width, u32 height, RngState *rng,
                                float4 *out_origin, float4 *out_dir) {
    f32 u = random_float(rng) - 0.5f;
    f32 v = random_float(rng) - 0.5f;

    f32 s = ((f32)px + 0.5f + u) / (f32)width;
    f32 t = 1.0f - ((f32)py + 0.5f + v) / (f32)height;

    float4 dir = cam->lower_left_corner + s * cam->horizontal + t * cam->vertical - cam->position;

    *out_origin = cam->position;
    *out_dir = normalize(dir);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_CAMERA_H
