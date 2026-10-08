#ifndef PHOSPHOR_ENVMAP_H
#define PHOSPHOR_ENVMAP_H

#ifdef __OPENCL_C_VERSION__

#include "constants.h"
#include "typedefs.h"

inline float4 sample_envmap(__global const f32 *envmap_data, u32 width, u32 height, float4 dir) {
    f32 theta = acos(clamp(dir.z, -1.0f, 1.0f));
    f32 phi = atan2(dir.y, dir.x);
    f32 u = (phi + PI) / (2.0f * PI);
    f32 v = clamp(1.0f - theta / PI, 0.0f, 1.0f);
    u = u - floor(u);
    u32 px = min((u32)(u * (f32)width), width - 1);
    u32 py = min((u32)(v * (f32)height), height - 1);
    u32 idx = 3 * (py * width + px);

    return (float4)(envmap_data[idx], envmap_data[idx + 1], envmap_data[idx + 2], 0.0f);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_ENVMAP_H
