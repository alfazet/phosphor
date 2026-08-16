#include "bsdfs.h"
#include "constants.h"
#include "hit.h"
#include "light.h"
#include "material.h"
#include "photon.h"
#include "random.h"
#include "ray.h"
#include "texture_meta.h"
#include "typedefs.h"

__kernel void trace_photons(__global Photon *photons, __global u32 *photon_count, __global const Light *lights,
                            const u32 n_lights, const u32 max_photons, const u32 seed, __global const BvhNode *tree,
                            __global const float4 *tri_v0, __global const float4 *tri_v1, __global const float4 *tri_v2,
                            __global const float2 *tri_uv0, __global const float2 *tri_uv1,
                            __global const float2 *tri_uv2, __global const u32 *tri_mat_index, const usize n_tris,
                            __global const Material *materials, __global const TextureMeta *tex_meta,
                            __global const u8 *tex_atlas) {
    usize gid = get_global_id(0);
    if (gid >= max_photons)
        return;

    RngState rng = pcg_seed(seed + (u32)gid);

    // TODO: proper light sampling
    Light light = lights[0];

    float4 power = light.power / (f32)max_photons;
    float4 origin = light.position;
    float4 dir = random_unit_vector(&rng);

    f32 curr_ior = AIR_IOR;
    for (u32 depth = 0; depth < MAX_PHOTON_BOUNCES; depth++) {
        HitRecord rec;
        bool hit =
            scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_mat_index, (u32)n_tris, origin, dir, EPS, INF, &rec);

        if (!hit)
            return;

        float4 hit_pos = origin + dir * rec.t;
        float4 normal = rec.normal;

        float2 uv0 = tri_uv0[rec.tri_index], uv1 = tri_uv1[rec.tri_index], uv2 = tri_uv2[rec.tri_index];
        f32 w = 1.0f - rec.u - rec.v;
        float2 uv = (float2)(w * uv0.x + rec.u * uv1.x + rec.v * uv2.x, w * uv0.y + rec.u * uv1.y + rec.v * uv2.y);

        Material mat = materials[rec.mat_index];

        f32 phi = atan2(dir.y, dir.x);
        f32 theta = acos(clamp(dir.z, -1.0f, 1.0f));

        float4 base_color = mat.base_color;
        if (mat.diff_index != NO_TEXTURE) {
            base_color *= sample_texture(tex_meta, tex_atlas, mat.diff_index, uv);
        }

        f32 metallic = mat.metallic;
        f32 roughness = mat.roughness;
        if (mat.metal_rough_index != NO_TEXTURE) {
            float4 mr = sample_texture(tex_meta, tex_atlas, mat.metal_rough_index, uv);
            metallic *= mr.z;  // channel B
            roughness *= mr.y; // channel G
        }

        // https://github.com/KhronosGroup/glTF/blob/77b44be7bef26e01fb0b140e3d5bb1716421c5e9/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows-bjs/js/babylon.pbrUtilities.js#L12
        float4 dielectric_specular = (float4)(0.04f);
        float4 s = mix(dielectric_specular, base_color, metallic);
        f32 max_s = fmax(s.x, fmax(s.y, s.z));
        f32 d_factor =
            (1.0f - dielectric_specular.x) * (1.0f - metallic) * (1.0f - mat.transmission) / fmax(1.0f - max_s, EPS);
        float4 d = base_color * d_factor;

        f32 alpha = roughness * roughness;
        f32 g1 = smith_g1_ggx(acos(fmax(0.0f, dot(-dir, normal))), alpha);
        float4 s_eff = s * g1;
        // st = specular/transmission
        float4 st = s_eff + base_color * ((1.0f - max_s) * mat.transmission * (1.0f - metallic));

        f32 sum_d = d.x + d.y + d.z;
        f32 sum_st = st.x + st.y + st.z;
        f32 sum_total = sum_d + sum_st;
        f32 rho_r = fmax(d.x + st.x, fmax(d.y + st.y, d.z + st.z));
        f32 rho_d = (sum_total > EPS) ? (rho_r * sum_d / sum_total) : 0.0f;
        f32 rho_st = rho_r - rho_d;

        f32 xi = random_float(&rng);

        if (xi < rho_d) {
            // store diffuse photon
            u32 idx = atomic_inc(photon_count);
            if (idx < max_photons) {
                photons[idx].pos = hit_pos;
                photons[idx].power = power;
                photons[idx].dir = -dir;
            } else {
                return;
            }

            // float4 new_dir = random_in_unit_hemisphere(&rng, normal);
            float4 new_dir = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
            origin = hit_pos;
            dir = new_dir;
            power *= d / rho_d;
        } else if (xi < rho_d + rho_st) {
            float4 new_dir =
                ggx_sample_direction(&rng, dir, normal, roughness, curr_ior, mat.ior, mat.transmission, rec.front_face);
            if (length(new_dir) < EPS)
                return;

            bool refracted = dot(new_dir, normal) < 0.0f;
            curr_ior = refracted ? (rec.front_face ? mat.ior : AIR_IOR) : curr_ior;

            origin = hit_pos;
            dir = new_dir;
            power *= st / rho_st;

            if (refracted)
                return;
        } else {
            // absorbed
            u32 idx = atomic_inc(photon_count);
            if (idx < max_photons) {
                photons[idx].pos = hit_pos;
                photons[idx].power = power;
                photons[idx].dir = -dir;
            }
            return;
        }
    }
}
