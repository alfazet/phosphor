#include "bsdfs.h"
#include "bvh_node.h"
#include "camera.h"
#include "constants.h"
#include "hit.h"
#include "hitpoint.h"
#include "light_sampling.h"
#include "material.h"
#include "random.h"
#include "shading.h"
#include "surface_hit.h"
#include "texture_meta.h"
#include "typedefs.h"

__kernel void camera_pass(
    // camera and image params
    const CameraParams camera, const u32 image_width, const u32 image_height, const u32 seed, const u32 direct_samples,

    // scene geometry
    __global const float4 *tri_v0, __global const float4 *tri_v1, __global const float4 *tri_v2,
    __global const float4 *tri_n0, __global const float4 *tri_n1, __global const float4 *tri_n2,
    __global const float2 *tri_uv0, __global const float2 *tri_uv1, __global const float2 *tri_uv2,
    __global const float4 *tri_t0, __global const float4 *tri_t1, __global const float4 *tri_t2,
    __global const BvhNode *tree, __global const u32 *tri_mat_index, const u32 n_triangles,

    // materials and textures
    __global const Material *materials, __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,

    // lights for direct illumination
    __global const Light *lights, const u32 n_lights, __global const f32 *light_pref_sum, const f32 total_luminance,
    const float4 scene_center, const f32 scene_radius,

    // emissive triangles
    __global const float4 *etri_v0, __global const float4 *etri_v1, __global const float4 *etri_v2,
    __global const float4 *etri_n0, __global const float4 *etri_n1, __global const float4 *etri_n2,
    __global const float2 *etri_uv0, __global const float2 *etri_uv1, __global const float2 *etri_uv2,
    __global const u32 *etri_mat_index,

    // output
    __global HitPoint *hit_points) {

    u32 tid = get_global_id(0);
    u32 n_pixels = image_width * image_height;
    if (tid >= n_pixels)
        return;

    u32 px = tid % image_width;
    u32 py = tid / image_width;
    RngState rng = pcg_seed(seed + tid);

    float4 origin, dir;
    make_ray(&camera, px, py, image_width, image_height, &rng, &origin, &dir);

    float4 throughput = WHITE;
    float4 emission = BLACK;
    f32 curr_ior = AIR_IOR;

    HitPoint hp;
    hp.position = ZERO;
    hp.normal = ZERO;
    hp.throughput = WHITE;
    hp.base_color = BLACK;
    hp.direct = BLACK;
    hp.emission = BLACK;
    hp.metallic = 0.0f;
    hp.is_valid = 0;

    for (i32 depth = 0; depth < MAX_RAY_BOUNCES; depth++) {
        HitRecord rec;
        bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2,
                                   tri_mat_index, n_triangles, origin, dir, EPS, INF, &rec);
        if (!hit)
            break; // ray escaped the scene

        SurfaceHit surf_hit = process_hit(&rec, origin, dir, tri_uv0, tri_uv1, tri_uv2);
        Material mat = materials[surf_hit.mat_index];

        float4 vol_trans = WHITE;
        if (curr_ior != AIR_IOR && mat.thickness > 0.0f && mat.att_dist > EPS)
            vol_trans = beer_lambert(mat.att_color, mat.att_dist, surf_hit.t);

        f32 bary_w = 1.0f - rec.u - rec.v;
        float4 tan_raw = bary_w * tri_t0[surf_hit.tri_index] + rec.u * tri_t1[surf_hit.tri_index] +
                         rec.v * tri_t2[surf_hit.tri_index];
        float4 tangent = (length(tan_raw) > EPS) ? normalize(tan_raw) : (float4)(1.0f, 0.0f, 0.0f, 0.0f);
        float4 bitangent = normalize(cross(surf_hit.normal, tangent));

        ShadingContext ctx =
            evaluate_material(&mat, surf_hit.uv, surf_hit.normal, tangent, bitangent, tex_meta, tex_atlas, vol_trans);
        emission += throughput * ctx.emissive;

        float4 view = -dir;
        BsdfSample bsdf =
            sample_bsdf(&rng, &ctx, ctx.shading_normal, surf_hit.normal, view, &curr_ior, surf_hit.front_face);
        bsdf.throughput *= vol_trans;

        if (bsdf.event == BSDF_DIFFUSE) {
            float4 direct_sum = BLACK;
            for (u32 d = 0; d < direct_samples; d++) {
                direct_sum += direct_lighting(
                    &rng, surf_hit.position, ctx.shading_normal, ctx.base_color, ctx.metallic, lights, n_lights,
                    light_pref_sum, total_luminance, scene_center, scene_radius, etri_v0, etri_v1, etri_v2, etri_n0,
                    etri_n1, etri_n2, etri_uv0, etri_uv1, etri_uv2, etri_mat_index, materials, tex_meta, tex_atlas, tree,
                    tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2, tri_mat_index, n_triangles);
            }
            float4 direct = direct_sum / (f32)direct_samples;

            hp.position = surf_hit.position;
            hp.normal = surf_hit.normal;
            hp.throughput = throughput;
            hp.base_color = ctx.base_color;
            hp.direct = throughput * ctx.occlusion * direct;
            hp.emission = emission;
            hp.metallic = ctx.metallic;
            hp.is_valid = 1;
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

    hit_points[tid] = hp;
}
