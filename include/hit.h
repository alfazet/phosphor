#ifndef PHOSPHOR_HIT_H
#define PHOSPHOR_HIT_H

#include "typedefs.h"

typedef struct HitRecord {
    f32 t;
    f32 u;
    f32 v;
    float4 normal;
    u32 tri_index;
    u32 mat_index;
} HitRecord;

inline bool intersect_scene(float4 origin, float4 dir, __global const float4 *tri_v0, __global const float4 *tri_v1,
                            __global const float4 *tri_v2, __global const u32 *tri_mat_index, usize n_tris,
                            HitRecord *out) {
    f32 closest_t = INFINITY;
    i32 hit_tri = -1;
    f32 hit_u = 0.0f, hit_v = 0.0f;

    // brute-force O(n_tris) scan, no acceleration structure
    for (usize t = 0; t < n_tris; t++) {
        float4 v0 = tri_v0[t], v1 = tri_v1[t], v2 = tri_v2[t];
        float4 edge1 = v1 - v0;
        float4 edge2 = v2 - v0;

        // moller-trumbore; w components of all vectors involved are 0, so
        // dot()/cross() behave as their 3D counterparts
        float4 h = cross(dir, edge2);
        f32 a = dot(edge1, h);
        if (fabs(a) < EPS)
            continue;

        f32 f = 1.0f / a;
        float4 s = origin - v0;
        f32 u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f)
            continue;

        float4 q = cross(s, edge1);
        f32 v = f * dot(dir, q);
        if (v < 0.0f || u + v > 1.0f)
            continue;

        f32 tt = f * dot(edge2, q);
        if (tt > EPS && tt < closest_t) {
            closest_t = tt;
            hit_tri = (i32)t;
            hit_u = u;
            hit_v = v;
        }
    }

    if (hit_tri < 0)
        return false;

    out->t = closest_t;
    out->tri_index = hit_tri;
    out->u = hit_u;
    out->v = hit_v;
    out->mat_index = tri_mat_index[hit_tri];
    float4 v0 = tri_v0[hit_tri], v1 = tri_v1[hit_tri], v2 = tri_v2[hit_tri];
    out->normal = normalize(cross(v1 - v0, v2 - v0));

    return true;
}

#endif // PHOSPHOR_HIT_H
