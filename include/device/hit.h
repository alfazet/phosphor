#ifndef PHOSPHOR_HIT_H
#define PHOSPHOR_HIT_H

#ifdef __OPENCL_C_VERSION__

#include "common/bounding_box.h"
#include "common/bvh_node.h"
#include "common/constants.h"
#include "common/triangle.h"
#include "common/typedefs.h"
#include "common/utils.h"

typedef struct HitRecord {
    f32 t;
    f32 u;
    f32 v;
    u32 tri_index;
    u32 mat_index;
} HitRecord;

typedef struct SurfaceHit {
    float4 position;
    float4 normal;
    float2 uv;
    f32 t;
    u32 mat_index;
    u32 tri_index;
    bool front_face;
} SurfaceHit;

inline float4 compute_bary(f32 bary_x, f32 bary_y, float4 v0, float4 v1, float4 v2) {
    f32 w0 = 1.0f - bary_x - bary_y;
    f32 w1 = bary_x;
    f32 w2 = bary_y;

    float4 result;
    result.x = w0 * v0.x + w1 * v1.x + w2 * v2.x;
    result.y = w0 * v0.y + w1 * v1.y + w2 * v2.y;
    result.z = w0 * v0.z + w1 * v1.z + w2 * v2.z;
    result.w = 0.0f;

    return result;
}

inline bool is_triangle_hit(u32 tri_idx, float4 origin, float4 dir, __global const TriangleVertices *tri_verts,
                            __global const u32 *tri_mat_index, f32 t_min, f32 t_max, HitRecord *hit_rec) {
    TriangleVertices tri_v = tri_verts[tri_idx];
    float4 e1 = tri_v.v1 - tri_v.v0;
    float4 e2 = tri_v.v2 - tri_v.v0;

    float4 h = cross(dir, e2);
    f32 a = dot(e1, h);
    if (fabs(a) < EPS)
        return false;

    f32 f = 1.0f / a;
    float4 s = origin - tri_v.v0;
    f32 u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    float4 q = cross(s, e1);
    f32 v = f * dot(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    f32 t = f * dot(e2, q);
    if (t < t_min || t > t_max)
        return false;

    hit_rec->t = t;
    hit_rec->u = u;
    hit_rec->v = v;
    hit_rec->tri_index = tri_idx;
    hit_rec->mat_index = tri_mat_index[tri_idx];

    return true;
}

inline bool bbox_hit(BoundingBox bbox, float4 origin, float4 dir, f32 t_min, f32 t_max) {
    f32 inv_dx = 1.0f / dir.x;
    f32 tx0 = (bbox.min.x - origin.x) * inv_dx;
    f32 tx1 = (bbox.max.x - origin.x) * inv_dx;
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
    f32 ty0 = (bbox.min.y - origin.y) * inv_dy;
    f32 ty1 = (bbox.max.y - origin.y) * inv_dy;
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
    f32 tz0 = (bbox.min.z - origin.z) * inv_dz;
    f32 tz1 = (bbox.max.z - origin.z) * inv_dz;
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

inline bool scene_intersect(__global const BvhNode *tree, __global const TriangleVertices *tri_verts,
                            __global const u32 *tri_mat_index, u32 n_tris, float4 origin, float4 dir, f32 t_min,
                            f32 t_max, HitRecord *hit_rec) {
    i32 p = 0;
    u32 stack[BVH_STACK_SIZE];

    stack[0] = 0;
    bool found = false;
    f32 closest = t_max;

    while (p >= 0) {
        u32 index = stack[p];
        BvhNode node = tree[index];
        p--;

        if (!bbox_hit(node.bbox, origin, dir, t_min, closest))
            continue;

        u32 triangle_index = as_uint(node.bbox.min.w);
        if (triangle_index != -1) {
            HitRecord candidate;
            if (is_triangle_hit(triangle_index, origin, dir, tri_verts, tri_mat_index, t_min, closest, &candidate)) {
                closest = candidate.t;
                *hit_rec = candidate;
                found = true;
            }
            continue;
        }
        if (p + 2 >= BVH_STACK_SIZE)
            continue;

        u32 right_child = as_uint(node.bbox.max.w);
        stack[++p] = right_child;
        stack[++p] = index + 1;
    }

    return found;
}

inline SurfaceHit process_hit(const HitRecord *hit_rec, float4 origin, float4 dir,
                              __global const TriangleNormals *tri_normals, __global const TriangleUvs *tri_uvs) {
    SurfaceHit surf_hit;
    surf_hit.t = hit_rec->t;
    surf_hit.mat_index = hit_rec->mat_index;
    surf_hit.tri_index = hit_rec->tri_index;
    surf_hit.position = origin + dir * hit_rec->t;

    f32 u = hit_rec->u, v = hit_rec->v;
    f32 w = 1.0f - u - v;

    TriangleNormals tn = tri_normals[hit_rec->tri_index];
    float4 outward_normal = normalize(compute_bary(u, v, tn.n0, tn.n1, tn.n2));
    bool front_face = dot(dir, outward_normal) < 0.0f;
    float4 normal = front_face ? outward_normal : -outward_normal;
    surf_hit.front_face = front_face;
    surf_hit.normal = normal;

    TriangleUvs tuv = tri_uvs[hit_rec->tri_index];
    surf_hit.uv =
        (float2)(w * tuv.uv0.x + u * tuv.uv1.x + v * tuv.uv2.x, w * tuv.uv0.y + u * tuv.uv1.y + v * tuv.uv2.y);

    return surf_hit;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_HIT_H
