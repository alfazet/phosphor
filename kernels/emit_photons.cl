#include "bsdfs.h"
#include "constants.h"
#include "hit.h"
#include "light.h"
#include "material.h"
#include "random.h"
#include "texture_meta.h"
#include "typedefs.h"
#include "utils.h"

static inline u32 select_light(RngState *rng, __global const f32 *pref_sum, u32 n_lights, f32 total_luminance,
                               f32 *scale) {
    if (n_lights == 0 || total_luminance < EPS) {
        *scale = 1.0f;
        return 0;
    }

    f32 r = random_float(rng) * total_luminance;
    u32 low = 0, high = n_lights;
    while (low < high) {
        u32 mid = (low + high) / 2;
        if (pref_sum[mid] < r)
            low = mid + 1;
        else
            high = mid;
    }
    if (low >= n_lights)
        low = n_lights - 1;

    f32 luminance = (low == 0) ? pref_sum[0] : (pref_sum[low] - pref_sum[low - 1]);
    *scale = fmax(luminance / total_luminance, EPS);

    return low;
}

static inline void sample_point_light(RngState *rng, Light *light, float4 *origin, float4 *dir, float4 *power) {
    *origin = light->position;
    *dir = random_unit_vector(rng);
    *power = light->power;
}

static inline void sample_spot_light(RngState *rng, Light *light, float4 *origin, float4 *dir, float4 *power) {
    *origin = light->position;

    f32 inner = light->aux.x;
    f32 outer = light->aux.y;
    f32 cos_inner = cos(inner);
    f32 cos_outer = cos(outer);
    f32 r1 = random_float(rng);
    f32 r2 = random_float(rng);

    f32 cos_theta = 1.0f - r1 * (1.0f - cos_outer);
    f32 sin_theta = sqrt(fmax(0.0f, 1.0f - cos_theta * cos_theta));
    f32 phi = 2.0f * PI * r2;

    float4 local_dir = (float4)(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta, 0.0f);

    float4 tangent, bitangent;
    float4 light_dir = normalize(light->direction);
    make_tbn(light_dir, &tangent, &bitangent);
    *dir = normalize(local_dir.x * tangent + local_dir.y * bitangent + local_dir.z * light_dir);

    f32 t = clamp((cos_theta - cos_outer) / (cos_inner - cos_outer), 0.0f, 1.0f);
    f32 falloff = t * t * (-2.0f * t + 3.0f);

    *power = light->power * falloff;
}

static inline void sample_textured_light(RngState *rng, Light *light, __global const float4 *etri_v0,
                                         __global const float4 *etri_v1, __global const float4 *etri_v2,
                                         __global const float4 *etri_n0, __global const float4 *etri_n1,
                                         __global const float4 *etri_n2, __global const float2 *etri_uv0,
                                         __global const float2 *etri_uv1, __global const float2 *etri_uv2,
                                         __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                                         float4 *origin, float4 *dir, float4 *power

) {
    u32 tex_index = as_uint(light->aux.x);
    u32 start = as_uint(light->aux.y);
    u32 n_triangles = as_uint(light->aux.z);

    u32 chosen_idx = start;
    if (n_triangles > 1) {
        f32 total_area = 0.0f;
        for (u32 i = 0; i < n_triangles; i++) {
            u32 t = start + i;
            float4 e1 = etri_v1[t] - etri_v0[t];
            float4 e2 = etri_v2[t] - etri_v0[t];
            f32 area = 0.5f * length(cross(e1, e2));
            total_area += area;
        }

        if (total_area > EPS) {
            f32 r = random_float(rng) * total_area;
            f32 accum = 0.0f;
            for (u32 i = 0; i < n_triangles; i++) {
                u32 t = start + i;
                float4 e1 = etri_v1[t] - etri_v0[t];
                float4 e2 = etri_v2[t] - etri_v0[t];
                f32 area = 0.5f * length(cross(e1, e2));
                accum += area;
                if (r <= accum || i == n_triangles - 1) {
                    chosen_idx = t;
                    break;
                }
            }
        }
    }

    f32 r1 = random_float(rng);
    f32 r2 = random_float(rng);
    f32 sqrt_r1 = sqrt(r1);
    f32 u = 1.0f - sqrt_r1;
    f32 v = r2 * sqrt_r1;
    f32 w = 1.0f - u - v;

    float4 v0 = etri_v0[chosen_idx];
    float4 v1 = etri_v1[chosen_idx];
    float4 v2 = etri_v2[chosen_idx];
    *origin = w * v0 + u * v1 + v * v2;

    float4 n0 = etri_n0[chosen_idx];
    float4 n1 = etri_n1[chosen_idx];
    float4 n2 = etri_n2[chosen_idx];
    float4 normal = w * n0 + u * n1 + v * n2;
    if (length(normal) > EPS) {
        normal = normalize(normal);
    } else {
        float4 e1 = v1 - v0;
        float4 e2 = v2 - v0;
        normal = normalize(cross(e1, e2));
    }

    float2 uv0 = etri_uv0[chosen_idx];
    float2 uv1 = etri_uv1[chosen_idx];
    float2 uv2 = etri_uv2[chosen_idx];
    float2 uv = (float2)(w * uv0.x + u * uv1.x + v * uv2.x, w * uv0.y + u * uv1.y + v * uv2.y);

    *dir = random_in_unit_hemisphere(rng, normal);

    float4 emissive_power = light->power;
    if (tex_index != NO_TEXTURE) {
        float4 tex_color = sample_texture(tex_meta, tex_atlas, tex_index, uv);
        emissive_power *= tex_color;
    }
    *power = emissive_power;
}

static inline void sample_directional_light(RngState *rng, Light *light, float4 center, f32 radius, float4 *origin, float4 *dir, float4 *power)
{
    // https://stackoverflow.com/questions/5837572/generate-a-random-point-within-a-circle-uniformly
    f32 r1 = radius * sqrt(random_float(rng));
    f32 r2 = 2 * PI * random_float(rng);
    float4 disk_offset = r1 * (cos(r2) * light->tangent + sin(r2) * light->bitangent);

    *dir = light->direction;
    *origin = center - light->direction * 2.0f * radius + disk_offset;
    *power = light->power;
}

inline void sample_light(RngState *rng, __global const Light *lights, u32 n_lights, __global const f32 *light_pref_sum,
                         f32 total_luminance, float4 scene_center, f32 scene_radius, __global const float4 *etri_v0,
                         __global const float4 *etri_v1, __global const float4 *etri_v2, __global const float4 *etri_n0,
                         __global const float4 *etri_n1, __global const float4 *etri_n2,
                         __global const float2 *etri_uv0, __global const float2 *etri_uv1,
                         __global const float2 *etri_uv2, __global const Material *materials,
                         __global const u32 *etri_mat_index, __global const TextureMeta *tex_meta,
                         __global const u8 *tex_atlas, float4 *origin, float4 *dir, float4 *power) {
    if (n_lights == 0 || total_luminance < EPS) {
        *origin = (float4)(0.0f);
        *dir = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
        *power = (float4)(0.0f);
        return;
    }

    f32 scale;
    u32 idx = select_light(rng, light_pref_sum, n_lights, total_luminance, &scale);
    Light light = lights[idx];

    if (light.kind == LIGHT_POINT) {
        sample_point_light(rng, &light, origin, dir, power);
    } else if (light.kind == LIGHT_SPOT) {
        sample_spot_light(rng, &light, origin, dir, power);
    } else if (light.kind == LIGHT_DIRECTIONAL) {
        sample_directional_light(rng, &light, scene_center, scene_radius, origin, dir, power);
    } else {
        sample_textured_light(rng, &light, etri_v0, etri_v1, etri_v2, etri_n0, etri_n1, etri_n2, etri_uv0, etri_uv1,
                              etri_uv2, tex_meta, tex_atlas, origin, dir, power);
    }
    *power /= scale;
}

__kernel void
emit_photons(__global float4 *photon_pos, __global float4 *photon_power, __global float4 *photon_dir,
             __global float4 *photon_normal, __global u32 *photon_count,

             __global const Light *lights, const u32 n_lights, const u32 max_photons, const u32 offset,
             const u32 photons_to_emit, const u32 seed, __global const BvhNode *tree, __global const float4 *tri_v0,
             __global const float4 *tri_v1, __global const float4 *tri_v2, __global const float4 *tri_n0,
             __global const float4 *tri_n1, __global const float4 *tri_n2, __global const float2 *tri_uv0,
             __global const float2 *tri_uv1, __global const float2 *tri_uv2, __global const float4 *tri_t0,
             __global const float4 *tri_t1, __global const float4 *tri_t2, __global const u32 *tri_mat_index,
             const u32 n_tris, __global const float4 *etri_v0, __global const float4 *etri_v1,
             __global const float4 *etri_v2, __global const float4 *etri_n0, __global const float4 *etri_n1,
             __global const float4 *etri_n2, __global const float2 *etri_uv0, __global const float2 *etri_uv1,
             __global const float2 *etri_uv2, __global const float4 *etri_t0, __global const float4 *etri_t1,
             __global const float4 *etri_t2, __global const u32 *etri_mat_index, __global const Material *materials,
             __global const TextureMeta *tex_meta, __global const u8 *tex_atlas, __global const f32 *light_pref_sum,
             const f32 total_luminance, const float4 scene_center, const f32 scene_radius) {
    u32 tid = get_global_id(0) + offset;
    if (tid - offset >= photons_to_emit)
        return;

    RngState rng = pcg_seed(seed + tid);

    float4 origin, dir, power;
    sample_light(&rng, lights, n_lights, light_pref_sum, total_luminance, scene_center, scene_radius, etri_v0, etri_v1,
                 etri_v2, etri_n0, etri_n1, etri_n2, etri_uv0, etri_uv1, etri_uv2, materials, tri_mat_index, tex_meta,
                 tex_atlas, &origin, &dir, &power);
    power /= (f32)photons_to_emit;

    f32 curr_ior = AIR_IOR;
    for (u32 depth = 0; depth < MAX_PHOTON_BOUNCES; depth++) {
        HitRecord rec;
        bool hit = scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2,
                                   tri_mat_index, (u32)n_tris, origin, dir, EPS, INF, &rec);

        if (!hit)
            return;

        float4 hit_pos = origin + dir * rec.t;
        float4 normal = rec.front_face ? rec.normal : -rec.normal;

        float2 uv0 = tri_uv0[rec.tri_index], uv1 = tri_uv1[rec.tri_index], uv2 = tri_uv2[rec.tri_index];
        f32 w = 1.0f - rec.u - rec.v;
        float2 uv = (float2)(w * uv0.x + rec.u * uv1.x + rec.v * uv2.x, w * uv0.y + rec.u * uv1.y + rec.v * uv2.y);

        Material mat = materials[rec.mat_index];
        if (curr_ior != AIR_IOR && mat.thickness > 0.0f && mat.att_dist > EPS) {
            f32 travel = rec.t;
            float4 att_color = mat.att_color;
            float4 sigma = (float4)(-log(fmax(att_color.x, EPS)), -log(fmax(att_color.x, EPS)),
                                    -log(fmax(att_color.x, EPS)), 0.0f) /
                           mat.att_dist;
            power.x *= exp(-sigma.x * travel);
            power.y *= exp(-sigma.y * travel);
            power.z *= exp(-sigma.z * travel);
        }

        float4 base_color = mat.base_color;
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

        float4 h = ggx_sample_vndf(&rng, normal, -dir, roughness);
        if (random_float(&rng) < metallic) {
            float4 reflected = reflect(dir, h);
            if (length(reflected) < EPS)
                return;

            u32 idx = atomic_inc(photon_count);
            if (idx < max_photons) {
                photon_pos[idx] = hit_pos;
                photon_power[idx] = power;
                photon_dir[idx] = -dir;
                photon_normal[idx] = normal;
            } else {
                return;
            }

            power *= fresnel4(base_color, dir, h);
            origin = hit_pos + normal * EPS;
            dir = reflected;
            continue;
        }

        f32 ior_1 = rec.front_face ? curr_ior : mat_ior;
        f32 ior_2 = rec.front_face ? mat_ior : curr_ior;
        f32 fr = fresnel_refracted(ior_1, ior_2, -dir, h);
        if (random_float(&rng) < fr) {
            float4 reflected = reflect(dir, h);
            if (length(reflected) < EPS)
                return;

            origin = hit_pos + normal * EPS;
            dir = reflected;
        } else {
            if (random_float(&rng) < transmission) {
                float4 refracted = refract(dir, normal, ior_1, ior_2);
                if (length(refracted) < EPS) {
                    return;
                }
                bool did_refract = dot(refracted, normal) < 0.0f;
                curr_ior = did_refract ? (rec.front_face ? mat.ior : AIR_IOR) : curr_ior;

                origin = hit_pos - normal * EPS;
                dir = refracted;
            } else {
                u32 idx = atomic_inc(photon_count);
                if (idx < max_photons) {
                    photon_pos[idx] = hit_pos;
                    photon_power[idx] = power;
                    photon_dir[idx] = -dir;
                    photon_normal[idx] = normal;
                } else {
                    return;
                }
                float4 new_dir = random_in_unit_hemisphere(&rng, normal);
                power *= base_color;
                origin = hit_pos + normal * EPS;
                dir = new_dir;
            }
        }
    }
}
