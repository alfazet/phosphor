#include "bsdfs.h"
#include "bvh_node.h"
#include "constants.h"
#include "hit.h"
#include "light_sampling.h"
#include "material.h"
#include "photon_hash.h"
#include "random.h"
#include "shading.h"
#include "surface_hit.h"
#include "texture_meta.h"
#include "typedefs.h"

__kernel void
trace_rays(__global const float4 *ray_origin, __global const float4 *ray_dir, const u32 n_rays, const u32 seed,

           __global const float4 *tri_v0, __global const float4 *tri_v1, __global const float4 *tri_v2,
           __global const float4 *tri_n0, __global const float4 *tri_n1, __global const float4 *tri_n2,
           __global const float2 *tri_uv0, __global const float2 *tri_uv1, __global const float2 *tri_uv2,
           __global const float4 *tri_t0, __global const float4 *tri_t1, __global const float4 *tri_t2,
           __global const BvhNode *tree, __global const u32 *tri_mat_index, const u32 n_triangles,

           __global const Material *materials, __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,

           __global const float4 *photon_pos, __global const float4 *photon_power, __global const float4 *photon_dir,
           __global const float4 *photon_normal, const u32 n_photons, const f32 search_radius, const u32 samples,

           __global float4 *out_color,

           __global const u32 *tree_index, __global const u32 *bucket_tree_offset, __global const u32 *bucket_tree_size,
           const PhotonHashInfo info,

           __global const Light *lights, const u32 n_lights, __global const f32 *light_pref_sum,
           const f32 total_luminance, const float4 scene_center, const f32 scene_radius, __global const float4 *etri_v0,
           __global const float4 *etri_v1, __global const float4 *etri_v2, __global const float4 *etri_n0,
           __global const float4 *etri_n1, __global const float4 *etri_n2, __global const float2 *etri_uv0,
           __global const float2 *etri_uv1, __global const float2 *etri_uv2) {
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
    float4 throughput = WHITE;

    i32 depth = 0;
    for (; depth < MAX_RAY_BOUNCES; depth++) {
        stack_diffuse[depth] = BLACK;
        stack_weight[depth] = BLACK;
        stack_emissive[depth] = BLACK;

        HitRecord rec;
        bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2,
                                   tri_mat_index, n_triangles, origin, dir, EPS, INF, &rec);
        if (!hit)
            break;
        stack_hit[depth] = hit;

        SurfaceHit surf_hit = process_hit(&rec, origin, dir, tri_uv0, tri_uv1, tri_uv2);
        Material mat = materials[surf_hit.mat_index];

        float4 vol_trans = WHITE;
        if (curr_ior != AIR_IOR && mat.thickness > 0.0f && mat.att_dist > EPS)
            vol_trans = beer_lambert(mat.att_color, mat.att_dist, surf_hit.t);

        f32 bary_w = 1.0f - rec.u - rec.v;
        float4 tan_raw = bary_w * tri_t0[surf_hit.tri_index] + rec.u * tri_t1[surf_hit.tri_index] +
                         rec.v * tri_t2[surf_hit.tri_index];
        float4 tangent = (length(tan_raw.xyz) > EPS) ? normalize(tan_raw) : (float4)(1.0f, 0.0f, 0.0f, 0.0f);
        float4 bitangent = normalize(cross(surf_hit.normal, tangent));

        ShadingContext ctx =
            evaluate_material(&mat, surf_hit.uv, surf_hit.normal, tangent, bitangent, tex_meta, tex_atlas, vol_trans);
        stack_emissive[depth] = ctx.emissive;

        float4 view = -dir;
        BsdfSample bsdf =
            sample_bsdf(&rng, &ctx, ctx.shading_normal, surf_hit.normal, view, &curr_ior, surf_hit.front_face);
        bsdf.throughput *= vol_trans;

        if (bsdf.event == BSDF_DIFFUSE) {
            // diffuse: gather photons and add direct lighting, then stop

            float4 flux = (float4)(0.0f);
            f32 max_dist_sq = 0.0f;
            f32 radius_sq = search_radius * search_radius;
            gather_photon_flux(surf_hit.position, info, tree_index, bucket_tree_offset, bucket_tree_size, photon_pos,
                               photon_power, samples, radius_sq, &flux, &max_dist_sq);

            float4 indirect = (float4)(0.0f);
            f32 area = PI * max_dist_sq;
            if (area > EPS)
                indirect = flux * ((1.0f - ctx.metallic) * ctx.base_color / PI) / area;

            float4 direct = direct_lighting(
                &rng, surf_hit.position, ctx.shading_normal, ctx.base_color, ctx.metallic, lights, n_lights,
                light_pref_sum, total_luminance, scene_center, scene_radius, etri_v0, etri_v1, etri_v2, etri_n0,
                etri_n1, etri_n2, etri_uv0, etri_uv1, etri_uv2, tex_meta, tex_atlas, tree, tri_v0, tri_v1, tri_v2,
                tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2, tri_mat_index, n_triangles);

            stack_diffuse[depth] = indirect + direct;
            depth++;
            break;
        }

        // specular/metallic/transmission: continue the ray
        throughput *= bsdf.throughput;
        f32 rr_compensation = 1.0f;
        if (depth >= MIN_RAY_RR_DEPTH) {
            f32 q = fmax(throughput.x, fmax(throughput.y, throughput.z));
            q = clamp(q, 0.05f, 1.0f);
            if (random_float(&rng) > q) {
                stack_hit[depth] = false;
                depth++;
                break;
            }
            rr_compensation = 1.0f / q;
            throughput *= rr_compensation;
        }
        stack_weight[depth] = bsdf.throughput * rr_compensation;

        if (bsdf.event == BSDF_TRANSMIT)
            origin = surf_hit.position - surf_hit.normal * EPS;
        else
            origin = surf_hit.position + surf_hit.normal * EPS;
        dir = bsdf.dir;
    }

    float4 result = BLACK;
    for (i32 i = depth - 1; i >= 0; i--) {
        if (!stack_hit[i])
            continue;
        result = stack_diffuse[i] + stack_weight[i] * result + stack_emissive[i];
    }
    out_color[tid] = result;
}
