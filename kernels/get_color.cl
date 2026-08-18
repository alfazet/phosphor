#include "bvh_node.h"
#include "constants.h"
#include "hit.h"
#include "material.h"
#include "photon.h"
#include "photon_hash.h"
#include "texture_meta.h"
#include "typedefs.h"
#include "random.h"

__kernel void get_color(__global const float4 *ray_origin, __global const float4 *ray_dir, const usize n_rays,
                        __global const float4 *tri_v0, __global const float4 *tri_v1, __global const float4 *tri_v2, __global const float4 *tri_n0, __global const float4 *tri_n1, __global const float4 *tri_n2,
                        __global const float2 *tri_uv0, __global const float2 *tri_uv1, __global const float2 *tri_uv2,
                         __global const float4 *tri_t0, __global const float4 *tri_t1, __global const float4 *tri_t2,
                        __global const BvhNode *tree, __global const u32 *tri_mat_index, const usize n_tris,
                        __global const Material *materials, __global const TextureMeta *tex_meta,
                        __global const u8 *tex_atlas, __global const Photon *photons, const u32 photon_count,
                        const f32 search_radius, const u32 samples, __global float4 *out_color,
                        __global const u32 *cell_start, __global const u32 *cell_end, const PhotonHashInfo info) {
    usize i = get_global_id(0);
    if (i >= n_rays)
        return;

    float4 origin = ray_origin[i];
    float4 dir = ray_dir[i];

    HitRecord rec;
    bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2,
                               tri_mat_index, (u32)n_tris, origin, dir, EPS, INF, &rec);

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
    f32 metallic = mat.metallic;
    f32 roughness = mat.roughness;
    if (mat.metal_rough_index != NO_TEXTURE) {
        float4 mr = sample_texture(tex_meta, tex_atlas, mat.metal_rough_index, uv);
        metallic *= mr.z;  // channel B
        roughness *= mr.y; // channel G
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
    u32 nei_count = get_photon_nei(hit_pos, info, cell_start, cell_end, starts, ends);

    f32 radius_sq = search_radius * search_radius;
    f32 max_dist_sq = 0.0f;
    u32 collected = 0;

    float4 t, b;
    make_tbn(normal, &t, &b);

    for (u32 i = 0; i < nei_count; i++) {
        for (u32 p = starts[i]; p < ends[i]; p++) {
            Photon ph = photons[p];
            float4 diff = hit_pos - ph.pos;
            f32 dist_sq = dot(diff.xyz, diff.xyz);
            f32 dist_v = fabs(dot(diff.xyz, normal.xyz));
            f32 dist_h = dot(diff.xyz, b.xyz) * dot(diff.xyz, b.xyz) + dot(diff.xyz, t.xyz) * dot(diff.xyz, t.xyz);
            // if (dist_sq < radius_sq && dot(-ph.dir.xyz, normal.xyz) < 0.0f && dot(ph.normal.xyz, normal.xyz) > 0.9f) {
            if (dist_v < DELTA && dist_h < radius_sq && dot(-ph.dir.xyz, normal.xyz) < 0.0f) {
                max_dist_sq = fmax(max_dist_sq, dist_sq);
                flux += ph.power;
            }
        }
    }
    //
    // f32 nearest_dist_sq[100];
    // float4 nearest_power[100];
    //
    // for (u32 c = 0; c < nei_count; c++) {
    //     for (u32 p = starts[c]; p < ends[c]; p++) {
    //         Photon ph = photons[p];
    //         float4 diff = ph.pos - hit_pos;
    //         f32 dist_sq = dot(diff.xyz, diff.xyz);
    //
    //         if (dist_sq >= radius_sq)
    //             continue;
    //         // don't count photons coming from "inside" the surface
    //         if (dot(-ph.dir.xyz, normal.xyz) > 0.0f)
    //             continue;
    //
    //         if (collected < samples) {
    //             nearest_dist_sq[collected] = dist_sq;
    //             nearest_power[collected] = ph.power;
    //             collected++;
    //             max_dist_sq = fmax(max_dist_sq, dist_sq);
    //         } else if (dist_sq < max_dist_sq) {
    //             u32 farthest = 0;
    //             for (u32 j = 1; j < samples; j++) {
    //                 if (nearest_dist_sq[j] > nearest_dist_sq[farthest])
    //                     farthest = j;
    //             }
    //             nearest_dist_sq[farthest] = dist_sq;
    //             nearest_power[farthest] = ph.power;
    //
    //             max_dist_sq = nearest_dist_sq[0];
    //             for (u32 j = 1; j < samples; j++) {
    //                 if (nearest_dist_sq[j] > max_dist_sq)
    //                     max_dist_sq = nearest_dist_sq[j];
    //             }
    //         }
    //     }
    // }

    // for (u32 j = 0; j < collected; j++)
    //     flux += nearest_power[j];

    f32 area = PI * max_dist_sq;
    float4 diffuse_color = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    if (area > EPS) {
        diffuse_color = flux * ((1.0f - metallic) * base_color / PI) / area;
    }

    out_color[i] = diffuse_color;
    out_color[i].w = 1.0f;
}
