#include "common/bvh_node.h"
#include "common/camera.h"
#include "common/constants.h"
#include "common/material.h"
#include "common/random.h"
#include "common/texture_meta.h"
#include "common/triangle.h"
#include "common/typedefs.h"
#include "device/envmap.h"
#include "device/hit.h"
#include "device/light_sampling.h"
#include "device/shading.h"

__kernel void trace_rays(
    // camera and image params
    const CameraParams camera, const u32 image_width, const u32 image_height, const u32 seed, const u32 direct_samples,

    // scene geometry
    __global const TriangleVertices *tri_verts, __global const TriangleNormals *tri_normals,
    __global const TriangleTangents *tri_tangents, __global const TriangleUvs *tri_uvs,
    __global const BvhNode *bvh_nodes, __global const u32 *tri_mat_index, const u32 n_triangles,

    // materials and textures
    __global const Material *materials, __global const UvTransformBlock *uv_transforms,
    __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,

    // lights for direct illumination
    __global const Light *lights, const u32 n_lights, __global const f32 *lum_pref_sum, const f32 total_lum,
    const float4 scene_center, const f32 scene_radius,

    // emissive triangles
    __global const TriangleVertices *etri_verts, __global const TriangleNormals *etri_normals,
    __global const TriangleUvs *etri_uvs, __global const u32 *etri_mat_index,

    // envmap
    __global const f32 *envmap_data, const u32 envmap_width, const u32 envmap_height, const u32 has_envmap,

    // output
    __global float4 *hp_pos, __global float4 *hp_normal, __global float4 *hp_throughput, __global float4 *hp_base_color,
    __global float4 *hp_direct, __global float4 *hp_emission) {

    u32 tid = get_global_id(0);
    u32 n_pixels = image_width * image_height;
    if (tid >= n_pixels)
        return;

    u32 px = tid % image_width;
    u32 py = tid / image_width;
    RngState rng = pcg_seed(seed + tid);

    float4 origin, dir;
    make_ray(&camera, px, py, image_width, image_height, &rng, &origin, &dir);

    float4 position = ZERO;
    float4 normal = ZERO;
    float4 throughput = WHITE;
    float4 base_color = BLACK;
    float4 direct = BLACK;
    float4 emission = BLACK;

    IorStack ior_stack;
    init_ior_stack(&ior_stack, AIR_IOR);

    for (i32 depth = 0; depth < MAX_RAY_BOUNCES; depth++) {
        HitRecord rec;
        bool hit = scene_intersect(bvh_nodes, tri_verts, tri_mat_index, n_triangles, origin, dir, EPS, INF, &rec);
        if (!hit) {
            if (has_envmap)
                emission += throughput * sample_envmap(envmap_data, envmap_width, envmap_height, dir);
            break;
        }

        SurfaceHit surf_hit = process_hit(&rec, origin, dir, tri_normals, tri_uvs);
        Material mat = materials[surf_hit.mat_index];

        float4 vol_trans = WHITE;
        f32 curr_ior = current_ior(&ior_stack);
        if (curr_ior != AIR_IOR && mat.thickness > 0.0f && mat.att_dist > EPS) {
            vol_trans = beer_lambert(mat.att_color, mat.att_dist, surf_hit.t);
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

        ShadingContext ctx = evaluate_material(&mat, surf_hit.mat_index, surf_hit.uv, surf_hit.normal, tangent,
                                               bitangent, tex_meta, tex_atlas, uv_transforms, vol_trans);
        emission += throughput * ctx.emissive;

        float4 view = -dir;
        BsdfSample bsdf =
            sample_bsdf(&rng, &ctx, ctx.shading_normal, surf_hit.normal, view, &ior_stack, surf_hit.front_face);

        if (bsdf.event == BSDF_DIFFUSE) {
            float4 direct_sum = BLACK;
            for (u32 d = 0; d < direct_samples; d++) {
                direct_sum += direct_lighting(&rng, surf_hit.position, ctx.shading_normal, ctx.base_color, lights,
                                              n_lights, lum_pref_sum, total_lum, scene_center, scene_radius, etri_verts,
                                              etri_normals, etri_uvs, etri_mat_index, materials, uv_transforms,
                                              tex_meta, tex_atlas, bvh_nodes, tri_verts, tri_mat_index, n_triangles);
            }

            emission = (float4)(emission.xyz, 1.0f);
            position = surf_hit.position;
            normal = surf_hit.normal;
            throughput = throughput;
            base_color = ctx.base_color;
            direct = throughput * ctx.occlusion * (direct_sum / (f32)direct_samples);
            break;
        }

        throughput *= bsdf.throughput;
        if (depth >= RR_MIN_RAY_DEPTH) {
            f32 q = fmax(throughput.x, fmax(throughput.y, throughput.z));
            q = clamp(q, RR_MIN_Q, RR_MAX_Q);
            if (random_float(&rng) > q)
                break;
            throughput *= 1.0f / q;
        }

        float4 side = (dot(bsdf.dir, surf_hit.normal) > 0.0f) ? surf_hit.normal : -surf_hit.normal;
        origin = surf_hit.position + bsdf.dir * EPS + side * EPS;
        dir = bsdf.dir;
    }

    hp_emission[tid] = emission;
    hp_pos[tid] = position;
    hp_normal[tid] = normal;
    hp_throughput[tid] = throughput;
    hp_base_color[tid] = base_color;
    hp_direct[tid] = direct;
}
