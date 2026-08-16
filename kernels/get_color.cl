#include "constants.h"
#include "material.h"
#include "texture_meta.h"
#include "typedefs.h"
#include "bvh_node.h"
#include "hit.h"

__kernel void get_color(__global const float4 *ray_origin, __global const float4 *ray_dir, const usize n_rays,
                             __global const float4 *tri_v0, __global const float4 *tri_v1,
                             __global const float4 *tri_v2, __global const float2 *tri_uv0,
                             __global const float2 *tri_uv1, __global const float2 *tri_uv2, __global const BvhNode *tree,
                             __global const u32 *tri_mat_index, const usize n_tris, __global const Material *materials,
                             __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                             __global float4 *out_color) {
    usize i = get_global_id(0);
        if (i >= n_rays)
            return;

        float4 origin = ray_origin[i];
        float4 dir = ray_dir[i];

        HitRecord rec;
        bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_mat_index, (u32)n_tris, origin, dir, EPS, INF, &rec);

        if (!hit) {
            out_color[i] = (float4)(0.0f, 0.0f, 0.0f, 1.0f); // miss -> black background
            return;
        }

        // interpolate UV with the barycentric coords scene_intersect returned
        float2 uv0 = tri_uv0[rec.tri_index], uv1 = tri_uv1[rec.tri_index], uv2 = tri_uv2[rec.tri_index];
        f32 w = 1.0f - rec.u - rec.v;
        float2 uv = (float2)(w * uv0.x + rec.u * uv1.x + rec.v * uv2.x, w * uv0.y + rec.u * uv1.y + rec.v * uv2.y);

        Material mat = materials[rec.mat_index];
        if (mat.diff_index == NO_TEXTURE) {
            out_color[i] = mat.base_color;
            return;
        }

        TextureMeta meta = tex_meta[mat.diff_index];

        f32 u_wrapped = uv.x - floor(uv.x);
        f32 v_wrapped = uv.y - floor(uv.y);
        u32 px = min((u32)(u_wrapped * (f32)meta.width), meta.width - 1);
        u32 py = min((u32)(v_wrapped * (f32)meta.height), meta.height - 1);

        u32 texel = meta.atlas_offset + (py * meta.width + px) * 3u;
        f32 r = (f32)tex_atlas[texel + 0] / 255.0f;
        f32 g = (f32)tex_atlas[texel + 1] / 255.0f;
        f32 b = (f32)tex_atlas[texel + 2] / 255.0f;

        out_color[i] = (float4)(r, g, b, 1.0f);
    }