#include "common/constants.h"
#include "common/material.h"
#include "common/random.h"
#include "common/texture_meta.h"
#include "common/triangle.h"
#include "common/typedefs.h"
#include "device/hit.h"
#include "device/light_sampling.h"
#include "device/photon.h"
#include "device/shading.h"

__kernel void emit_photons(
    // output photon arrays
    __global float4 *photon_pos, __global u32 *photon_power, __global u32 *photon_dir, __global u32 *n_photons,

    // light sources
    __global const Light *lights, const u32 n_lights, const u32 max_photons, const u32 offset,
    const u32 photons_to_emit, const u32 max_photon_bounces, const u32 seed,

    // scene geometry
    __global const BvhNode *tree, __global const TriangleVertices *tri_verts,
    __global const TriangleNormals *tri_normals, __global const TriangleTangents *tri_tangents,
    __global const TriangleUvs *tri_uvs, __global const u32 *tri_mat_index, const u32 n_tris,

    // emissive triangles
    __global const TriangleVertices *etri_verts, __global const TriangleNormals *etri_normals,
    __global const TriangleUvs *etri_uvs, __global const u32 *etri_mat_index,

    // materials, textures and lighting params
    __global const Material *materials, __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
    __global const f32 *lum_pref_sum, const f32 total_lum, const float4 scene_center, const f32 scene_radius) {

    u32 tid = get_global_id(0) + offset;
    if (tid - offset >= photons_to_emit)
        return;
    RngState rng = pcg_seed(seed + tid);

    float4 origin, dir, power;
    sample_light(&rng, lights, n_lights, lum_pref_sum, total_lum, scene_center, scene_radius, etri_verts, etri_normals,
                 etri_uvs, etri_mat_index, materials, tex_meta, tex_atlas, &origin, &dir, &power);

    IorStack ior_stack;
    init_ior_stack(&ior_stack, AIR_IOR);

    // the fraction of the original power that survives to the current bounce
    float4 throughput = (float4)(1.0f, 1.0f, 1.0f, 0.0f);

    for (u32 depth = 0; depth < max_photon_bounces; depth++) {
        HitRecord rec;
        bool hit = scene_intersect(tree, tri_verts, tri_mat_index, n_tris, origin, dir, EPS, INF, &rec);
        if (!hit)
            return; // photon escaped the scene

        SurfaceHit surf_hit = process_hit(&rec, origin, dir, tri_normals, tri_uvs);
        Material mat = materials[surf_hit.mat_index];

        float4 vol_trans = WHITE;
        f32 curr_ior = current_ior(&ior_stack);
        if (curr_ior != AIR_IOR && mat.thickness > 0.0f && mat.att_dist > EPS) {
            vol_trans = beer_lambert(mat.att_color, mat.att_dist, surf_hit.t);
            power *= vol_trans;
            throughput *= vol_trans;
        }

        f32 bary_w = 1.0f - rec.u - rec.v;
        TriangleTangents tt = tri_tangents[surf_hit.tri_index];
        float4 tangent_raw = bary_w * tt.t0 + rec.u * tt.t1 + rec.v * tt.t2;
        float4 bitangent_raw = (length(tangent_raw) > EPS) ? cross(surf_hit.normal, tangent_raw) : ZERO;
        float4 tangent, bitangent;
        if (length(bitangent_raw) > EPS) {
            bitangent = normalize(bitangent_raw);
            tangent = cross(bitangent, surf_hit.normal);
        } else {
            make_tbn(surf_hit.normal, &tangent, &bitangent);
        }

        ShadingContext ctx =
            evaluate_material(&mat, surf_hit.uv, surf_hit.normal, tangent, bitangent, tex_meta, tex_atlas, vol_trans);

        // russian roulette: after a minimum number of bounces, randomly terminate low-energy photons
        // survivors' power is boosted by 1/q to compensate for the terminated ones
        if (depth >= RR_MIN_PHOTON_DEPTH) {
            f32 q = fmax(throughput.x, fmax(throughput.y, throughput.z));
            q = clamp(q, RR_MIN_Q, RR_MAX_Q);
            if (random_float(&rng) > q)
                return;
            f32 inv_q = 1.0f / q;
            throughput *= inv_q;
            power *= inv_q;
        }

        float4 view = -dir;
        BsdfSample bsdf =
            sample_bsdf(&rng, &ctx, ctx.shading_normal, surf_hit.normal, view, &ior_stack, surf_hit.front_face);

        if (bsdf.event == BSDF_DIFFUSE && dot(dir, surf_hit.normal) < 0.0f) {
            u32 idx = atomic_inc(n_photons);
            if (idx < max_photons) {
                photon_pos[idx] = surf_hit.position;
                photon_power[idx] = encode_rgbe(power);
                photon_dir[idx] = encode_oct(dir);
            } else {
                return;
            }
        }

        power *= bsdf.throughput;
        throughput *= bsdf.throughput;
        float4 side = (dot(bsdf.dir, surf_hit.normal) > 0.0f) ? surf_hit.normal : -surf_hit.normal;
        origin = surf_hit.position + bsdf.dir * EPS + side * EPS;
        dir = bsdf.dir;
    }
}
