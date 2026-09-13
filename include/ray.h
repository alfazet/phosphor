#ifndef PHOSPHOR_RAY_H
#define PHOSPHOR_RAY_H

#include "typedefs.h"

struct Ray {
    float4 origin;
    float4 dir;
    float4 dp_dx, dd_dx, dp_dy, dd_dy;
};

#ifdef __OPENCL_C_VERSION__
inline float4 ray_at(float4 origin, float4 dir, f32 t) { return origin + t * dir; }
#endif

#endif // PHOSPHOR_RAY_H
