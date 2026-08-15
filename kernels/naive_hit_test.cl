#include "constants.h"
#include "material.h"
#include "texture_meta.h"
#include "typedefs.h"

// fully generated and not touched; test purposes

// standalone test kernel: no BVH, no shading model, no lights.
// for each ray: loop over every triangle, keep the closest hit, and if there
// is one, sample the diffuse texture at the interpolated UV (mip level 0,
// nearest-neighbor). if the material has no diffuse texture, just use its
// base_color. rays that miss everything get a flat background color.

__kernel void naive_hit_test(__global const float4 *ray_origin, __global const float4 *ray_dir, const usize n_rays,
                             __global const float4 *tri_v0, __global const float4 *tri_v1,
                             __global const float4 *tri_v2, __global const float2 *tri_uv0,
                             __global const float2 *tri_uv1, __global const float2 *tri_uv2,
                             __global const u32 *tri_mat_index, const usize n_tris, __global const Material *materials,
                             __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                             __global float4 *out_color) {
    usize i = get_global_id(0);
    if (i >= n_rays)
        return;

    float4 origin = ray_origin[i];
    float4 dir = ray_dir[i];

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

    if (hit_tri < 0) {
        out_color[i] = (float4)(0.0f, 0.0f, 0.0f, 1.0f); // miss -> black background
        return;
    }

    // interpolate UV with the barycentric coords from the winning triangle
    float2 uv0 = tri_uv0[hit_tri], uv1 = tri_uv1[hit_tri], uv2 = tri_uv2[hit_tri];
    f32 w = 1.0f - hit_u - hit_v;
    float2 uv = (float2)(w * uv0.x + hit_u * uv1.x + hit_v * uv2.x, w * uv0.y + hit_u * uv1.y + hit_v * uv2.y);

    Material mat = materials[tri_mat_index[hit_tri]];
    if (mat.diff_index == NO_TEXTURE) {
        out_color[i] = mat.base_color;
        return;
    }

    TextureMeta meta = tex_meta[mat.diff_index];

    // wrap uv into [0, 1) and sample mip level 0, nearest-neighbor
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
