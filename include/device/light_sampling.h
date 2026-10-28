#ifndef PHOSPHOR_LIGHT_SAMPLING_H
#define PHOSPHOR_LIGHT_SAMPLING_H

#ifdef __OPENCL_C_VERSION__

#include "common/constants.h"
#include "common/light.h"
#include "common/material.h"
#include "common/random.h"
#include "common/texture_meta.h"
#include "common/triangle.h"
#include "common/typedefs.h"
#include "common/utils.h"
#include "device/bsdfs.h"
#include "device/hit.h"

static inline u32 select_light(RngState *rng, __global const f32 *pref_sum, u32 n_lights, f32 total_lum, f32 *scale) {
    if (n_lights == 0 || total_lum < EPS) {
        *scale = 1.0f;
        return 0;
    }

    f32 r = random_float(rng) * total_lum;
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

    f32 lum = (low == 0) ? pref_sum[0] : (pref_sum[low] - pref_sum[low - 1]);
    *scale = fmax(lum / total_lum, EPS);

    return low;
}

static inline void sample_point_light(RngState *rng, Light *light, float4 *origin, float4 *dir, float4 *power) {
    *origin = light->position;
    *dir = random_unit_vector(rng);
    *power = light->power * (4.0f * PI);
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
    f32 angle_factor = 2.0f * PI * (1.0f - cos_outer);
    *power = light->power * falloff * angle_factor;
}

static inline void sample_textured_light(RngState *rng, Light *light, __global const TriangleVertices *etri_verts,
                                         __global const TriangleNormals *etri_normals,
                                         __global const TriangleUvs *etri_uvs, __global const u32 *etri_mat_index,
                                         __global const Material *materials, __global const TextureMeta *tex_meta,
                                         __global const u8 *tex_atlas, float4 *origin, float4 *dir, float4 *power) {
    u32 tex_index = as_uint(light->aux.x);
    u32 start = as_uint(light->aux.y);
    u32 n_triangles = as_uint(light->aux.z);

    u32 chosen_idx = start;
    if (n_triangles > 1) {
        f32 total_area = light->aux.w;
        if (total_area > EPS) {
            f32 r = random_float(rng) * total_area;
            f32 accum = 0.0f;
            for (u32 i = 0; i < n_triangles; i++) {
                u32 t = start + i;
                TriangleVertices ev = etri_verts[t];
                float4 e1 = ev.v1 - ev.v0;
                float4 e2 = ev.v2 - ev.v0;
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

    TriangleVertices chosen_ev = etri_verts[chosen_idx];
    float4 v0 = chosen_ev.v0, v1 = chosen_ev.v1, v2 = chosen_ev.v2;
    *origin = w * v0 + u * v1 + v * v2;

    TriangleNormals chosen_en = etri_normals[chosen_idx];
    float4 normal = w * chosen_en.n0 + u * chosen_en.n1 + v * chosen_en.n2;
    if (length(normal) > EPS)
        normal = normalize(normal);
    else
        normal = normalize(cross(v1 - v0, v2 - v0));

    TriangleUvs chosen_euv = etri_uvs[chosen_idx];
    float2 uv = (float2)(w * chosen_euv.uv0.x + u * chosen_euv.uv1.x + v * chosen_euv.uv2.x,
                         w * chosen_euv.uv0.y + u * chosen_euv.uv1.y + v * chosen_euv.uv2.y);

    *dir = random_in_unit_hemisphere(rng, normal);
    *origin += EPS * *dir;

    float4 emissive_power = light->power;
    if (tex_index != NO_TEXTURE) {
        u32 mat_idx = etri_mat_index[chosen_idx];
        Material mat = materials[mat_idx];
        float4 tex_color = sample_texture_uv(&mat, tex_meta, tex_atlas, tex_index, uv, mat.emis_transform);
        emissive_power *= srgb_to_linear(tex_color);
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

    *power = light->power * PI * radius * radius;
}

inline void sample_light(RngState *rng, __global const Light *lights, u32 n_lights, __global const f32 *lum_pref_sum,
                         f32 total_lum, float4 scene_center, f32 scene_radius,
                         __global const TriangleVertices *etri_verts, __global const TriangleNormals *etri_normals,
                         __global const TriangleUvs *etri_uvs, __global const u32 *etri_mat_index,
                         __global const Material *materials, __global const TextureMeta *tex_meta,
                         __global const u8 *tex_atlas, float4 *origin, float4 *dir, float4 *power) {
    if (n_lights == 0 || total_lum < EPS) {
        *origin = (float4)(0.0f);
        *dir = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
        *power = BLACK;
        return;
    }

    f32 scale;
    u32 idx = select_light(rng, lum_pref_sum, n_lights, total_lum, &scale);
    Light light = lights[idx];

    if (light.kind == LIGHT_POINT)
        sample_point_light(rng, &light, origin, dir, power);
    else if (light.kind == LIGHT_SPOT)
        sample_spot_light(rng, &light, origin, dir, power);
    else if (light.kind == LIGHT_DIRECTIONAL)
        sample_directional_light(rng, &light, scene_center, scene_radius, origin, dir, power);
    else
        sample_textured_light(rng, &light, etri_verts, etri_normals, etri_uvs, etri_mat_index, materials, tex_meta,
                              tex_atlas, origin, dir, power);

    *power /= scale;
}

// compute direct lighting (reflected radiance) by light-source sampling with next-event estimation
inline float4 direct_lighting(RngState *rng, float4 pos, float4 normal, float4 base_color,
                              __global const Light *lights, u32 n_lights, __global const f32 *lum_pref_sum,
                              f32 total_lum, float4 scene_center, f32 scene_radius,
                              __global const TriangleVertices *etri_verts, __global const TriangleNormals *etri_normals,
                              __global const TriangleUvs *etri_uvs, __global const u32 *etri_mat_index,
                              __global const Material *materials, __global const TextureMeta *tex_meta,
                              __global const u8 *tex_atlas, __global const BvhNode *tree,
                              __global const TriangleVertices *tri_verts, __global const u32 *tri_mat_index,
                              u32 n_tris) {
    if (n_lights == 0 || total_lum < EPS)
        return BLACK;

    f32 scale;
    u32 light_idx = select_light(rng, lum_pref_sum, n_lights, total_lum, &scale);
    Light light = lights[light_idx];
    float4 flux = (float4)(light.power.x, light.power.y, light.power.z, 0.0f) / scale;

    float4 shadow_origin = pos;
    float4 shadow_dir;
    f32 shadow_dist;
    float4 radiance;

    if (light.kind == LIGHT_POINT) {
        float4 to_light = light.position - pos;
        f32 dist_sq = fmax(dot(to_light, to_light), EPS);
        f32 dist = sqrt(dist_sq);
        shadow_dir = to_light / dist;
        shadow_dist = dist;

        f32 cos_i = dot(shadow_dir, normal);
        if (cos_i <= 0.0f)
            return BLACK;

        radiance = flux * cos_i / dist_sq;

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

        radiance = flux * falloff * cos_i / dist_sq;

    } else if (light.kind == LIGHT_DIRECTIONAL) {
        shadow_dir = normalize(-light.direction);
        shadow_dist = INF;
        f32 cos_i = dot(shadow_dir, normal);
        if (cos_i <= 0.0f)
            return BLACK;

        radiance = flux * cos_i;

    } else {
        u32 tex_index = as_uint(light.aux.x);
        u32 start = as_uint(light.aux.y);
        u32 n_triangles = as_uint(light.aux.z);

        f32 total_area = light.aux.w;
        if (total_area < EPS)
            return BLACK;

        u32 chosen = start;
        f32 r_area = random_float(rng) * total_area;
        f32 accum = 0.0f;
        for (u32 i = 0; i < n_triangles; i++) {
            u32 t = start + i;
            TriangleVertices ev = etri_verts[t];
            float4 e1 = ev.v1 - ev.v0;
            float4 e2 = ev.v2 - ev.v0;
            accum += 0.5f * length(cross(e1, e2));
            if (r_area <= accum || i == n_triangles - 1) {
                chosen = t;
                break;
            }
        }

        f32 r1 = random_float(rng), r2 = random_float(rng);
        f32 sq = sqrt(r1);
        f32 bu = 1.0f - sq, bv = r2 * sq, bw = 1.0f - bu - bv;
        TriangleVertices chosen_ev = etri_verts[chosen];
        float4 light_pos = bw * chosen_ev.v0 + bu * chosen_ev.v1 + bv * chosen_ev.v2;

        TriangleNormals chosen_en = etri_normals[chosen];
        float4 ln = bw * chosen_en.n0 + bu * chosen_en.n1 + bv * chosen_en.n2;
        if (length(ln) > EPS)
            ln = normalize(ln);
        else
            ln = normalize(cross(chosen_ev.v1 - chosen_ev.v0, chosen_ev.v2 - chosen_ev.v0));

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
            TriangleUvs chosen_euv = etri_uvs[chosen];
            float2 uv = (float2)(bw * chosen_euv.uv0.x + bu * chosen_euv.uv1.x + bv * chosen_euv.uv2.x,
                                 bw * chosen_euv.uv0.y + bu * chosen_euv.uv1.y + bv * chosen_euv.uv2.y);
            u32 mat_idx = etri_mat_index[chosen];
            Material mat = materials[mat_idx];
            float4 tex_color = sample_texture_uv(&mat, tex_meta, tex_atlas, tex_index, uv, mat.emis_transform);
            emissive_flux *= srgb_to_linear(tex_color);
        }

        f32 cos_l = fmax(dot(-shadow_dir, ln), 0.0f);
        radiance = emissive_flux * cos_i * cos_l / (PI * dist_sq);
    }

    if (dot(radiance, radiance) < EPS * EPS)
        return BLACK;

    float4 trans = WHITE;
    f32 t_min = EPS, t_max = shadow_dist - EPS;
    for (u32 depth = 0; depth < MAX_RAY_BOUNCES; depth++) {
        HitRecord shadow_hit_rec;
        bool hit = scene_intersect(tree, tri_verts, tri_mat_index, n_tris, shadow_origin, shadow_dir, t_min, t_max,
                                   &shadow_hit_rec);
        if (!hit)
            break;
        Material mat = materials[shadow_hit_rec.mat_index];
        if (mat.transmission < EPS)
            return BLACK;

        trans *= mat.base_color * mat.transmission;
        if (dot(trans, trans) < EPS)
            return BLACK;
        t_min = shadow_hit_rec.t + EPS;
    }

    return radiance * (base_color / PI) * trans;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_LIGHT_SAMPLING_H
