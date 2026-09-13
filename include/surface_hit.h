#ifndef PHOSPHOR_SURFACE_HIT_H
#define PHOSPHOR_SURFACE_HIT_H

#ifdef __OPENCL_C_VERSION__

#include "constants.h"
#include "hit.h"
#include "typedefs.h"

typedef struct SurfaceHit {
    float4 position;
    float4 normal;
    float2 uv;
    u32 mat_index;
    u32 tri_index;
    f32 t;
    bool front_face;
} SurfaceHit;

inline SurfaceHit process_hit(const HitRecord *rec, float4 origin, float4 dir, __global const float2 *tri_uv0,
                              __global const float2 *tri_uv1, __global const float2 *tri_uv2) {
    SurfaceHit hit;
    hit.t = rec->t;
    hit.mat_index = rec->mat_index;
    hit.tri_index = rec->tri_index;
    hit.front_face = rec->front_face;
    hit.position = origin + dir * rec->t;
    hit.normal = rec->normal;

    float2 uv0 = tri_uv0[rec->tri_index];
    float2 uv1 = tri_uv1[rec->tri_index];
    float2 uv2 = tri_uv2[rec->tri_index];
    f32 w = 1.0f - rec->u - rec->v;
    hit.uv = (float2)(w * uv0.x + rec->u * uv1.x + rec->v * uv2.x, w * uv0.y + rec->u * uv1.y + rec->v * uv2.y);

    return hit;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_SURFACE_HIT_H
