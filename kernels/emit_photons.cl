#include "bsdfs.h"
#include "constants.h"
#include "hit.h"
#include "light_sampling.h"
#include "material.h"
#include "random.h"
#include "shading.h"
#include "surface_hit.h"
#include "texture_meta.h"
#include "typedefs.h"
#include "utils.h"

__kernel void
emit_photons(__global float4 *photon_pos, __global float4 *photon_power, __global float4 *photon_dir,
             __global float4 *photon_normal, __global u32 *photon_count,

             __global const Light *lights, const u32 n_lights, const u32 max_photons, const u32 offset,
             const u32 photons_to_emit, const u32 seed,

             __global const BvhNode *tree, __global const float4 *tri_v0, __global const float4 *tri_v1,
             __global const float4 *tri_v2, __global const float4 *tri_n0, __global const float4 *tri_n1,
             __global const float4 *tri_n2, __global const float2 *tri_uv0, __global const float2 *tri_uv1,
             __global const float2 *tri_uv2, __global const float4 *tri_t0, __global const float4 *tri_t1,
             __global const float4 *tri_t2, __global const u32 *tri_mat_index, const u32 n_tris,

             __global const float4 *etri_v0, __global const float4 *etri_v1, __global const float4 *etri_v2,
             __global const float4 *etri_n0, __global const float4 *etri_n1, __global const float4 *etri_n2,
             __global const float2 *etri_uv0, __global const float2 *etri_uv1, __global const float2 *etri_uv2,
             __global const float4 *etri_t0, __global const float4 *etri_t1, __global const float4 *etri_t2,
             __global const u32 *etri_mat_index,

             __global const Material *materials, __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,

             __global const f32 *light_pref_sum, const f32 total_luminance, const float4 scene_center,
             const f32 scene_radius) {
    u32 tid = get_global_id(0) + offset;
    if (tid - offset >= photons_to_emit)
        return;

    RngState rng = pcg_seed(seed + tid);

    float4 origin, dir, power;
    sample_light(&rng, lights, n_lights, light_pref_sum, total_luminance, scene_center, scene_radius, etri_v0, etri_v1,
                 etri_v2, etri_n0, etri_n1, etri_n2, etri_uv0, etri_uv1, etri_uv2, tex_meta, tex_atlas, &origin, &dir,
                 &power);
    // normalise by total photon count so that stored values represent per-photon flux
    power /= (f32)photons_to_emit;

    f32 curr_ior = AIR_IOR;
    float4 throughput = WHITE;

    for (u32 depth = 0; depth < MAX_PHOTON_BOUNCES; depth++) {
        HitRecord rec;
        bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2,
                                   tri_mat_index, n_tris, origin, dir, EPS, INF, &rec);
        if (!hit)
            return;

        SurfaceHit surf_hit = process_hit(&rec, origin, dir, tri_uv0, tri_uv1, tri_uv2);
        Material mat = materials[surf_hit.mat_index];

        float4 vol_trans = WHITE;
        if (curr_ior != AIR_IOR && mat.thickness > 0.0f && mat.att_dist > EPS) {
            vol_trans = beer_lambert(mat.att_color, mat.att_dist, surf_hit.t);
            power *= vol_trans;
            throughput *= vol_trans;
        }

        f32 bary_w = 1.0f - rec.u - rec.v;
        float4 tan_raw = bary_w * tri_t0[surf_hit.tri_index] + rec.u * tri_t1[surf_hit.tri_index] +
                         rec.v * tri_t2[surf_hit.tri_index];
        float4 tangent = (length(tan_raw.xyz) > EPS) ? normalize(tan_raw) : (float4)(1.0f, 0.0f, 0.0f, 0.0f);
        float4 bitangent = normalize(cross(surf_hit.normal, tangent));

        ShadingContext ctx =
            evaluate_material(&mat, surf_hit.uv, surf_hit.normal, tangent, bitangent, tex_meta, tex_atlas, vol_trans);

        if (depth >= MIN_PHOTON_RR_DEPTH) {
            f32 q = fmax(throughput.x, fmax(throughput.y, throughput.z));
            q = clamp(q, 0.05f, 1.0f);
            if (random_float(&rng) > q)
                return;
            f32 inv_q = 1.0f / q;
            throughput *= inv_q;
            power *= inv_q;
        }

        float4 view = -dir;
        BsdfSample bsdf =
            sample_bsdf(&rng, &ctx, ctx.shading_normal, surf_hit.normal, view, &curr_ior, surf_hit.front_face);

        if (bsdf.event == BSDF_DIFFUSE || bsdf.event == BSDF_METALLIC) {
            u32 idx = atomic_inc(photon_count);
            if (idx < max_photons) {
                photon_pos[idx] = surf_hit.position;
                photon_power[idx] = power;
                photon_dir[idx] = view;
                photon_normal[idx] = surf_hit.normal;
            } else {
                return;
            }
        }
        power *= bsdf.throughput;
        throughput *= bsdf.throughput;

        if (bsdf.event == BSDF_TRANSMIT)
            origin = surf_hit.position - surf_hit.normal * EPS;
        else
            origin = surf_hit.position + surf_hit.normal * EPS;
        dir = bsdf.dir;
    }
}
