#ifndef PHOSPHOR_LIGHT_SAMPLING_H
#define PHOSPHOR_LIGHT_SAMPLING_H

#ifdef __OPENCL_C_VERSION__

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

    f32 t = clamp((cos_theta - cos_outer) / (cos_inner - cos_outer + EPS), 0.0f, 1.0f);
    f32 falloff = t * t * (-2.0f * t + 3.0f);
    *power = light->power * falloff;
}

static inline void sample_textured_light(RngState *rng, Light *light, __global const float4 *etri_v0,
                                         __global const float4 *etri_v1, __global const float4 *etri_v2,
                                         __global const float4 *etri_n0, __global const float4 *etri_n1,
                                         __global const float4 *etri_n2, __global const float2 *etri_uv0,
                                         __global const float2 *etri_uv1, __global const float2 *etri_uv2,
                                         __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                                         float4 *origin, float4 *dir, float4 *power) {
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
            total_area += 0.5f * length(cross(e1, e2));
        }
        if (total_area > EPS) {
            f32 r = random_float(rng) * total_area;
            f32 accum = 0.0f;
            for (u32 i = 0; i < n_triangles; i++) {
                u32 t = start + i;
                float4 e1 = etri_v1[t] - etri_v0[t];
                float4 e2 = etri_v2[t] - etri_v0[t];
                accum += 0.5f * length(cross(e1, e2));
                if (r <= accum || i == n_triangles - 1) {
                    chosen_idx = t;
                    break;
                }
            }
        }
    }

    f32 r1 = random_float(rng), r2 = random_float(rng);
    f32 sqrt_r1 = sqrt(r1);
    f32 u = 1.0f - sqrt_r1;
    f32 v = r2 * sqrt_r1;
    f32 w = 1.0f - u - v;

    float4 v0 = etri_v0[chosen_idx], v1 = etri_v1[chosen_idx], v2 = etri_v2[chosen_idx];
    *origin = w * v0 + u * v1 + v * v2;

    float4 n0 = etri_n0[chosen_idx], n1 = etri_n1[chosen_idx], n2 = etri_n2[chosen_idx];
    float4 normal = w * n0 + u * n1 + v * n2;
    if (length(normal) > EPS)
        normal = normalize(normal);
    else
        normal = normalize(cross(v1 - v0, v2 - v0));

    float2 uv0_ = etri_uv0[chosen_idx], uv1_ = etri_uv1[chosen_idx], uv2_ = etri_uv2[chosen_idx];
    float2 uv = (float2)(w * uv0_.x + u * uv1_.x + v * uv2_.x, w * uv0_.y + u * uv1_.y + v * uv2_.y);

    *dir = random_in_unit_hemisphere(rng, normal);
    float4 emissive_power = light->power;
    if (tex_index != NO_TEXTURE) {
        float4 tex_color = sample_texture(tex_meta, tex_atlas, tex_index, uv);
        emissive_power *= tex_color;
    }
    *power = emissive_power;
}

static inline void sample_directional_light(RngState *rng, Light *light, float4 center, f32 radius, float4 *origin,
                                            float4 *dir, float4 *power) {
    // https://stackoverflow.com/questions/5837572/generate-a-random-point-within-a-circle-uniformly
    f32 r1 = radius * sqrt(random_float(rng));
    f32 r2 = 2.0f * PI * random_float(rng);
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
                         __global const float2 *etri_uv2, __global const TextureMeta *tex_meta,
                         __global const u8 *tex_atlas, float4 *origin, float4 *dir, float4 *power) {
    if (n_lights == 0 || total_luminance < EPS) {
        *origin = (float4)(0.0f);
        *dir = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
        *power = BLACK;
        return;
    }

    f32 scale;
    u32 idx = select_light(rng, light_pref_sum, n_lights, total_luminance, &scale);
    Light light = lights[idx];

    if (light.kind == LIGHT_POINT)
        sample_point_light(rng, &light, origin, dir, power);
    else if (light.kind == LIGHT_SPOT)
        sample_spot_light(rng, &light, origin, dir, power);
    else if (light.kind == LIGHT_DIRECTIONAL)
        sample_directional_light(rng, &light, scene_center, scene_radius, origin, dir, power);
    else
        sample_textured_light(rng, &light, etri_v0, etri_v1, etri_v2, etri_n0, etri_n1, etri_n2, etri_uv0, etri_uv1,
                              etri_uv2, tex_meta, tex_atlas, origin, dir, power);

    *power /= scale;
}

// compute direct lighting by light-source sampling with next-event estimation
inline float4
direct_lighting(RngState *rng, float4 pos, float4 normal, float4 base_color, f32 metallic, __global const Light *lights,
                u32 n_lights, __global const f32 *light_pref_sum, f32 total_luminance, float4 scene_center,
                f32 scene_radius, __global const float4 *etri_v0, __global const float4 *etri_v1,
                __global const float4 *etri_v2, __global const float4 *etri_n0, __global const float4 *etri_n1,
                __global const float4 *etri_n2, __global const float2 *etri_uv0, __global const float2 *etri_uv1,
                __global const float2 *etri_uv2, __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                __global const BvhNode *tree, __global const float4 *tri_v0, __global const float4 *tri_v1,
                __global const float4 *tri_v2, __global const float2 *tri_uv0, __global const float2 *tri_uv1,
                __global const float2 *tri_uv2, __global const float4 *tri_n0, __global const float4 *tri_n1,
                __global const float4 *tri_n2, __global const u32 *tri_mat_index, u32 n_tris) {
    if (n_lights == 0 || total_luminance < EPS)
        return BLACK;

    f32 scale;
    u32 light_idx = select_light(rng, light_pref_sum, n_lights, total_luminance, &scale);
    Light light = lights[light_idx];
    float4 flux = (float4)(light.power.x, light.power.y, light.power.z, 0.0f) / scale;

    float4 shadow_origin = pos + normal * EPS;
    float4 shadow_dir;
    f32 shadow_dist;
    float4 irradiance;

    if (light.kind == LIGHT_POINT) {
        float4 to_light = light.position - pos;
        f32 dist_sq = fmax(dot(to_light, to_light), EPS);
        f32 dist = sqrt(dist_sq);
        shadow_dir = to_light / dist;
        shadow_dist = dist;

        f32 cos_i = dot(shadow_dir, normal);
        if (cos_i <= 0.0f)
            return BLACK;

        irradiance = flux * cos_i / (4.0f * PI * dist_sq);

    } else if (light.kind == LIGHT_SPOT) {
        float4 to_light = light.position - pos;
        f32 dist_sq = fmax(dot(to_light, to_light), EPS);
        f32 dist = sqrt(dist_sq);
        shadow_dir = to_light / dist;
        shadow_dist = dist;

        f32 cos_i = dot(shadow_dir, normal);
        if (cos_i <= 0.0f)
            return BLACK;

        f32 cos_dir = dot(-shadow_dir, normalize(light.direction));
        f32 cos_outer = cos(light.aux.y);
        f32 cos_inner = cos(light.aux.x);
        f32 t = clamp((cos_dir - cos_outer) / (cos_inner - cos_outer + EPS), 0.0f, 1.0f);
        f32 falloff = t * t * (-2.0f * t + 3.0f);
        if (falloff < EPS)
            return BLACK;

        irradiance = flux * falloff * cos_i / (4.0f * PI * dist_sq);

    } else if (light.kind == LIGHT_DIRECTIONAL) {
        shadow_dir = normalize(-light.direction);
        shadow_dist = INF;
        f32 cos_i = dot(shadow_dir, normal);
        if (cos_i <= 0.0f)
            return BLACK;

        irradiance = flux * cos_i;

    } else {
        u32 tex_index = as_uint(light.aux.x);
        u32 start = as_uint(light.aux.y);
        u32 n_triangles = as_uint(light.aux.z);

        f32 total_area = 0.0f;
        for (u32 i = 0; i < n_triangles; i++) {
            u32 t = start + i;
            float4 e1 = etri_v1[t] - etri_v0[t];
            float4 e2 = etri_v2[t] - etri_v0[t];
            total_area += 0.5f * length(cross(e1, e2));
        }
        if (total_area < EPS)
            return BLACK;

        u32 chosen = start;
        f32 r_area = random_float(rng) * total_area;
        f32 accum = 0.0f;
        for (u32 i = 0; i < n_triangles; i++) {
            u32 t = start + i;
            float4 e1 = etri_v1[t] - etri_v0[t];
            float4 e2 = etri_v2[t] - etri_v0[t];
            accum += 0.5f * length(cross(e1, e2));
            if (r_area <= accum || i == n_triangles - 1) {
                chosen = t;
                break;
            }
        }

        f32 r1 = random_float(rng), r2 = random_float(rng);
        f32 sq = sqrt(r1);
        f32 bu = 1.0f - sq, bv = r2 * sq, bw = 1.0f - bu - bv;
        float4 light_pos = bw * etri_v0[chosen] + bu * etri_v1[chosen] + bv * etri_v2[chosen];

        float4 ln = bw * etri_n0[chosen] + bu * etri_n1[chosen] + bv * etri_n2[chosen];
        if (length(ln) > EPS)
            ln = normalize(ln);
        else
            ln = normalize(cross(etri_v1[chosen] - etri_v0[chosen], etri_v2[chosen] - etri_v0[chosen]));

        float4 to_light = light_pos - pos;
        f32 dist_sq = fmax(dot(to_light, to_light), EPS);
        f32 dist = sqrt(dist_sq);
        shadow_dir = to_light / dist;
        shadow_dist = dist;

        f32 cos_i = dot(shadow_dir, normal);
        if (cos_i <= 0.0f)
            return BLACK;

        float4 emissive_flux = flux;
        if (tex_index != NO_TEXTURE) {
            float2 uv0 = etri_uv0[chosen], uv1 = etri_uv1[chosen], uv2 = etri_uv2[chosen];
            float2 uv = (float2)(bw * uv0.x + bu * uv1.x + bv * uv2.x, bw * uv0.y + bu * uv1.y + bv * uv2.y);
            emissive_flux *= sample_texture(tex_meta, tex_atlas, tex_index, uv);
        }

        f32 cos_l = fmax(dot(-shadow_dir, ln), 0.0f);
        irradiance = emissive_flux * cos_i * cos_l / (PI * dist_sq);
    }

    if (dot(irradiance, irradiance) < EPS * EPS)
        return BLACK;

    HitRecord shadow_hit_rec;
    bool occluded =
        scene_intersect(tree, tri_v0, tri_v1, tri_v2, tri_uv0, tri_uv1, tri_uv2, tri_n0, tri_n1, tri_n2, tri_mat_index,
                        n_tris, shadow_origin, shadow_dir, EPS, shadow_dist - EPS, &shadow_hit_rec);
    if (occluded)
        return BLACK;

    return irradiance * (1.0f - metallic) * base_color / PI;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_LIGHT_SAMPLING_H
