#ifndef PHOSPHOR_CAMERA_H
#define PHOSPHOR_CAMERA_H

#include "constants.h"
#include "typedefs.h"

typedef struct GPU_ALIGN CameraParams {
    float4 position;
    float4 lower_left_corner;
    float4 horizontal;
    float4 vertical;
    float4 defocus_disk_u;
    float4 defocus_disk_v;
    // 6 * 4 * 4 = 96

    f32 defocus_angle;
    f32 focus_distance;
    // 2 * 4 = 8

    // total: 104
    u8 _padding[8];
} CameraParams;

#ifdef __OPENCL_C_VERSION__

#include "random.h"

inline void make_ray(const CameraParams *cam, u32 px, u32 py, u32 width, u32 height, RngState *rng, float4 *out_origin,
                     float4 *out_dir) {
    f32 u = random_float(rng) - 0.5f;
    f32 v = random_float(rng) - 0.5f;
    f32 s = ((f32)px + 0.5f + u) / (f32)width;
    f32 t = 1.0f - ((f32)py + 0.5f + v) / (f32)height;

    float4 disk_offset = (float4)(0.0f);
    if (cam->defocus_angle > 0.0f) {
        f32 r1 = sqrt(random_float(rng));
        f32 r2 = 2.0f * PI * random_float(rng);
        f32 disk_offset_cos = cos(r2);
        f32 disk_offset_sin = sin(r2);
        disk_offset = r1 * (disk_offset_cos * cam->defocus_disk_u + disk_offset_sin * cam->defocus_disk_v);
    }
    *out_origin = cam->position + disk_offset;

    float4 dir = cam->lower_left_corner + s * cam->horizontal + t * cam->vertical - *out_origin;
    *out_dir = normalize(dir);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_CAMERA_H
