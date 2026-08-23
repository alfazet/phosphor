#include "bsdfs.h"
#include "bvh_node.h"
#include "constants.h"
#include "hit.h"
#include "material.h"
#include "photon.h"
#include "photon_hash.h"
#include "random.h"
#include "texture_meta.h"
#include "typedefs.h"

__kernel void trace_rays(__global const float4 *ray_origin, __global const float4 *ray_dir, const u32 n_rays,
                         const u32 seed, __global const float4 *tri_v0, __global const float4 *tri_v1,
                         __global const float4 *tri_v2, __global const float4 *tri_n0, __global const float4 *tri_n1,
                         __global const float4 *tri_n2, __global const float2 *tri_uv0, __global const float2 *tri_uv1,
                         __global const float2 *tri_uv2, __global const float4 *tri_t0, __global const float4 *tri_t1,
                         __global const float4 *tri_t2, __global const BvhNode *tree, __global const u32 *tri_mat_index,
                         const u32 n_triagles, __global const Material *materials, __global const TextureMeta *tex_meta,
                         __global const u8 *tex_atlas, __global const Photon *photons, const u32 n_photons,
                         const f32 search_radius, const u32 samples, __global float4 *out_color,
                         __global const u32 *tree_index, __global const u32 *bucket_tree_offset,
                         __global const u32 *bucket_tree_size, const PhotonHashInfo info) {
    u32 tid = get_global_id(0);
    if (tid >= n_rays)
        return;

    RngState rng = pcg_seed(seed + tid);

    float4 stack_diffuse[MAX_RAY_BOUNCES];
    float4 stack_weight[MAX_RAY_BOUNCES];
    float4 stack_emissive[MAX_RAY_BOUNCES];
    bool stack_hit[MAX_RAY_BOUNCES];

    float4 origin = ray_origin[tid];
    float4 dir = ray_dir[tid];
    f32 curr_ior = AIR_IOR;
    float4 hit_pos, normal;

    i32 depth = 0;
    for (; depth < MAX_RAY_BOUNCES; depth++) {
        HitRecord rec;
        bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2,
                                   tri_mat_index, n_triagles, origin, dir, EPS, INF, &rec);
        stack_hit[depth] = hit;
        if (!hit)
            break;

        hit_pos = origin + dir * rec.t;
        normal = rec.front_face ? rec.normal : -rec.normal;

        // interpolate UV with the barycentric coords scene_intersect returned
        float2 uv0 = tri_uv0[rec.tri_index], uv1 = tri_uv1[rec.tri_index], uv2 = tri_uv2[rec.tri_index];
        f32 w = 1.0f - rec.u - rec.v;
        float2 uv = (float2)(w * uv0.x + rec.u * uv1.x + rec.v * uv2.x, w * uv0.y + rec.u * uv1.y + rec.v * uv2.y);

        Material mat = materials[rec.mat_index];

        // Beer-Lambert law for volumetric materials
        float4 vol_trans = (float4)(1.0f, 1.0f, 1.0f, 0.0f);
        if (curr_ior != AIR_IOR && mat.thickness > 0.0f && mat.att_dist > EPS) {
            f32 travel = rec.t;
            float4 att_color = mat.att_color;
            float4 sigma = (float4)(-log(fmax(att_color.x, EPS)), -log(fmax(att_color.y, EPS)),
                                    -log(fmax(att_color.z, EPS)), 0.0f) /
                           mat.att_dist;
            vol_trans.x = exp(-sigma.x * travel);
            vol_trans.y = exp(-sigma.y * travel);
            vol_trans.z = exp(-sigma.z * travel);
        }
        float4 base_color = mat.base_color * vol_trans;

        if (mat.diff_index != NO_TEXTURE) {
            base_color *= sample_texture(tex_meta, tex_atlas, mat.diff_index, uv);
        }
        f32 metallic = mat.metallic;
        f32 roughness = mat.roughness;
        f32 transmission = mat.transmission;
        f32 mat_ior = mat.ior;
        if (mat.metal_rough_index != NO_TEXTURE) {
            float4 mr = sample_texture(tex_meta, tex_atlas, mat.metal_rough_index, uv);
            metallic *= mr.z;  // channel B
            roughness *= mr.y; // channel G
        }

        stack_emissive[depth] = mat.emissive;
        if (mat.emis_index != NO_TEXTURE) {
            stack_emissive[depth] = sample_texture(tex_meta, tex_atlas, mat.emis_index, uv);
        }

        f32 mix_factor = fmax(metallic, transmission);

        float4 diffuse_color = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
        if (mix_factor < 1.0f - EPS) {
            float4 flux = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
            f32 radius_sq = search_radius * search_radius;
            f32 max_dist_sq = 0.0f;

            gather_photon_flux(hit_pos, info, tree_index, bucket_tree_offset, bucket_tree_size, photons, samples,
                               radius_sq, &flux, &max_dist_sq);

            f32 area = PI * max_dist_sq;
            if (area > EPS)
                diffuse_color = flux * ((1.0f - metallic) * base_color / PI) / area;
        }
        stack_diffuse[depth] = diffuse_color;
        stack_weight[depth] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

        float4 h = ggx_sample_vndf(&rng, normal, -dir, roughness);
        if (random_float(&rng) < metallic) {
            float4 reflected = reflect(dir, h);
            if (length(reflected) < EPS) {
                stack_weight[depth] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
                depth++;
                break;
            }
            stack_diffuse[depth] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
            stack_weight[depth] = fresnel4(base_color, -dir, h);

            origin = hit_pos + normal * EPS;
            dir = reflected;
            continue;
        }
        f32 ior_1 = rec.front_face ? curr_ior : mat_ior;
        f32 ior_2 = rec.front_face ? mat_ior : AIR_IOR;
        f32 fr = fresnel_refracted(ior_1, ior_2, -dir, h);
        if (random_float(&rng) < fr) {
            float4 reflected = reflect(dir, h);
            if (length(reflected) < EPS) {
                stack_weight[depth] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
                stack_diffuse[depth] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
                depth++;
                break;
            }

            stack_diffuse[depth] *= (1.0f - fr);
            stack_weight[depth] = (float4)(fr, fr, fr, fr);

            origin = hit_pos + normal * EPS;
            dir = reflected;
            continue;
        }
        if (random_float(&rng) < transmission) {
            float4 refracted = refract(dir, normal, ior_1, ior_2);
            if (length(refracted) < EPS) {
                stack_weight[depth] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
                depth++;
                break;
            }
            bool did_refract = dot(refracted, normal) < 0.0f;
            f32 next_ior = did_refract ? (rec.front_face ? mat_ior : AIR_IOR) : curr_ior;

            stack_diffuse[depth] *= (1.0f - transmission);
            stack_weight[depth] = (float4)(transmission, transmission, transmission, transmission);

            curr_ior = next_ior;
            origin = hit_pos - normal * EPS;
            dir = refracted;
            continue;
        }
        depth++;
        break;
    }

    float4 result = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    for (i32 i = depth - 1; i >= 0; i--) {
        if (!stack_hit[i])
            continue;
        result = stack_diffuse[i] + stack_weight[i] * result + stack_emissive[i];
    }

    out_color[tid] = result;
    out_color[tid].w = 1.0f;
}
