#include "bvh_node.h"
#include "constants.h"
#include "hit.h"
#include "material.h"
#include "photon.h"
#include "photon_hash.h"
#include "texture_meta.h"
#include "typedefs.h"

__kernel void get_color(__global const float4 *ray_origin, __global const float4 *ray_dir, const usize n_rays,
                        __global const float4 *tri_v0, __global const float4 *tri_v1, __global const float4 *tri_v2,
                        __global const float2 *tri_uv0, __global const float2 *tri_uv1, __global const float2 *tri_uv2, __global const float2 *tri_n0, __global const float2 *tri_n1,
                            __global const float2 *tri_n2,
                        __global const BvhNode *tree, __global const u32 *tri_mat_index, const usize n_tris,
                        __global const Material *materials, __global const TextureMeta *tex_meta,
                        __global const u8 *tex_atlas, __global const Photon *photons, __global const u32 *photon_count,
                        const f32 search_radius, __global float4 *out_color, __global const u32 *cell_start,
                        __global const u32 *cell_end, const PhotonHashInfo info) {
    usize i = get_global_id(0);
    if (i >= n_rays)
        return;

    float4 origin = ray_origin[i];
    float4 dir = ray_dir[i];

    HitRecord rec;
    bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2, tri_mat_index, (u32)n_tris, origin, dir, EPS, INF, &rec);

    if (!hit) {
        out_color[i] = (float4)(0.0f, 0.0f, 0.0f, 1.0f); // miss -> black background
        return;
    }

    // interpolate UV with the barycentric coords scene_intersect returned
    float2 uv0 = tri_uv0[rec.tri_index], uv1 = tri_uv1[rec.tri_index], uv2 = tri_uv2[rec.tri_index];
    f32 w = 1.0f - rec.u - rec.v;
    float2 uv = (float2)(w * uv0.x + rec.u * uv1.x + rec.v * uv2.x, w * uv0.y + rec.u * uv1.y + rec.v * uv2.y);

    Material mat = materials[rec.mat_index];
    float4 base_color = mat.base_color;
    if (mat.diff_index != NO_TEXTURE) {
        base_color = sample_texture(tex_meta, tex_atlas, mat.diff_index, uv);
    }

    // photon gather
    float4 hit_pos = origin + dir * rec.t;
    float4 normal = rec.normal;

    // used to visualize how the space is partitionaed using spatial hashing
    /*
    u32 h = photon_hash(hit_pos, info);
    f32 hue = fmod((f32)h * 0.61803f, 1.0f); // golden-ratio scatter for visual distinctness
    out_color[i] = (float4)(hue, fmod(hue*3.0f,1.0f), fmod(hue*7.0f,1.0f), 1.0f);
    return;
    */

    float4 flux = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

    u32 starts[27];
    u32 ends[27];
    for(u32 i=0; i<27; i++) {
        i32 x = i%3-1;
        i32 y = (i/3)%3-1;
        i32 z = (i/9)%3-1;
        float4 hit_pos2 = hit_pos;
        hit_pos2.x += (f32)x*info.cell_sizes.x;
        hit_pos2.y += (f32)y*info.cell_sizes.y;
        hit_pos2.z += (f32)z*info.cell_sizes.z;

        u32 h = photon_hash(hit_pos2, info);
        starts[i] = cell_start[h];
        ends[i] = cell_end[h];
    }

    f32 max_dist_sq = 0.0f;
    f32 radius_sq = search_radius * search_radius;

    for (u32 i = 0; i < 27; i++) {
        u32 start = starts[i];
        u32 end = ends[i];
        for (u32 p = start; p < end; p++) {
            Photon ph = photons[p];
            float4 diff = ph.pos - hit_pos;
            f32 dist_sq = dot(diff, diff);

            if (dist_sq < radius_sq) {
                max_dist_sq = fmax(max_dist_sq, dist_sq);
                flux += ph.power;
            }
        }
    }

    f32 area = PI * max_dist_sq;
    float4 gathered = (area > EPS) ? flux / area : (float4)(0.0f);

    out_color[i] = gathered * base_color;
    out_color[i].w = 1.0f;
}
