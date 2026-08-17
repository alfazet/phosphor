#ifndef PHOSPHOR_HIT_H
#define PHOSPHOR_HIT_H

#include "bounding_box.h"
#include "bvh_node.h"
#include "constants.h"
#include "helpers.h"
#include "typedefs.h"

typedef struct HitRecord {
    f32 t;
    f32 u;
    f32 v;
    float4 normal;
    u32 tri_index;
    u32 mat_index;
    bool front_face;
} HitRecord;

inline bool triangle_hit_single(u32 t, float4 origin, float4 dir, __global const float4 *tri_v0,
                                __global const float4 *tri_v1, __global const float4 *tri_v2,
                                __global const float4 *tri_uv0, __global const float4 *tri_uv1,
                                __global const float4 *tri_uv2, __global const float4 *tri_n0,
                                __global const float4 *tri_n1, __global const float4 *tri_n2,
                                __global const u32 *tri_mat_index, f32 t_min, f32 t_max, HitRecord *out) {
    float4 v0 = tri_v0[t], v1 = tri_v1[t], v2 = tri_v2[t];
    float4 uv0 = tri_uv0[t], uv1 = tri_uv1[t], uv2 = tri_uv2[t];
    float4 n0 = tri_n0[t], n1 = tri_n1[t], n2 = tri_n2[t];
    float4 edge1 = v1 - v0;
    float4 edge2 = v2 - v0;

    // moller-trumbore; w components of all vectors involved are 0, so
    // dot()/cross() behave as their 3D counterparts
    float4 h = cross(dir, edge2);
    f32 a = dot(edge1, h);
    if (fabs(a) < EPS)
        return false;

    f32 f = 1.0f / a;
    float4 s = origin - v0;
    f32 u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    float4 q = cross(s, edge1);
    f32 v = f * dot(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    f32 tt = f * dot(edge2, q);
    if (tt < t_min || tt > t_max)
        return false;

    float4 outward_normal = normalize(compute_bary(u, v, n0, n1, n2));
    bool front_face = dot(dir, outward_normal) < 0.0f;
    float4 normal = front_face ? outward_normal : -outward_normal;

    out->t = tt;
    out->u = u;
    out->v = v;
    out->tri_index = t;
    out->mat_index = tri_mat_index[t];
    out->normal = normalize(cross(edge1, edge2));
    out->front_face = front_face;

    return true;
}

inline bool bbox_hit(BoundingBox bbox, float4 origin, float4 dir, f32 t_min, f32 t_max) {
    f32 inv_dx = 1.0f / dir.x;
    f32 tx0 = (bbox.bbox_min.x - origin.x) * inv_dx;
    f32 tx1 = (bbox.bbox_max.x - origin.x) * inv_dx;
    if (tx0 < tx1) {
        if (tx0 > t_min)
            t_min = tx0;
        if (tx1 < t_max)
            t_max = tx1;
    } else {
        if (tx1 > t_min)
            t_min = tx1;
        if (tx0 < t_max)
            t_max = tx0;
    }
    if (t_max <= t_min)
        return false;

    f32 inv_dy = 1.0f / dir.y;
    f32 ty0 = (bbox.bbox_min.y - origin.y) * inv_dy;
    f32 ty1 = (bbox.bbox_max.y - origin.y) * inv_dy;
    if (ty0 < ty1) {
        if (ty0 > t_min)
            t_min = ty0;
        if (ty1 < t_max)
            t_max = ty1;
    } else {
        if (ty1 > t_min)
            t_min = ty1;
        if (ty0 < t_max)
            t_max = ty0;
    }
    if (t_max <= t_min)
        return false;

    f32 inv_dz = 1.0f / dir.z;
    f32 tz0 = (bbox.bbox_min.z - origin.z) * inv_dz;
    f32 tz1 = (bbox.bbox_max.z - origin.z) * inv_dz;
    if (tz0 < tz1) {
        if (tz0 > t_min)
            t_min = tz0;
        if (tz1 < t_max)
            t_max = tz1;
    } else {
        if (tz1 > t_min)
            t_min = tz1;
        if (tz0 < t_max)
            t_max = tz0;
    }
    if (t_max <= t_min)
        return false;

    return true;
}

inline bool scene_intersect(__global const BvhNode *tree, __global const float4 *tri_v0, __global const float4 *tri_v1,
                            __global const float4 *tri_v2, __global const float4 *tri_uv0,
                            __global const float4 *tri_uv1, __global const float4 *tri_uv2,
                            __global const float4 *tri_n0, __global const float4 *tri_n1, __global const float4 *tri_n2,
                            __global const u32 *tri_mat_index, u32 n_tris, float4 origin, float4 dir, f32 t_min,
                            f32 t_max, HitRecord *rec) {
    i32 p = 0;
    u32 stack[BVH_STACK_SIZE];

    stack[0] = 1; // 0 was left blank
    bool found = false;
    f32 closest = t_max;

    while (p >= 0) {
        u32 index = stack[p];
        BvhNode node = tree[index];
        p--;

        if (!bbox_hit(node.bbox, origin, dir, t_min, closest))
            continue;

        if (node.triangle_index != -1) {
            HitRecord candidate;
            if (triangle_hit_single(node.triangle_index, origin, dir, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2,
                                    tri_n0, tri_n1, tri_n2, tri_mat_index, t_min, closest, &candidate)) {
                closest = candidate.t;
                *rec = candidate;
                found = true;
            }
            continue;
        }
        stack[++p] = 2 * index;
        stack[++p] = 2 * index + 1;
    }

    return found;
}

#endif // PHOSPHOR_HIT_H
