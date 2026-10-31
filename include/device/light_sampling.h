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

typedef struct Bary {
    f32 u, v, w;
} Bary;

typedef struct EmissivePoint {
    u32 tri_idx;
    Bary bary;
    float4 pos;
    float4 normal;
} EmissivePoint;

typedef struct LightFragment {
    float4 radiance;
    float4 dir;
    f32 dist;
    f32 n_dot_dir;
} LightFragment;

static inline f32 spotlight_falloff(f32 cos_angle, f32 cos_inner, f32 cos_outer) {
    f32 t = clamp((cos_angle - cos_outer) / (cos_inner - cos_outer + EPS), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static inline float4 light_flux(const Light *light, f32 scale) {
    return (float4)(light->power.x, light->power.y, light->power.z, 0.0f) / scale;
}

static inline void fill_light_vecs(float4 from, float4 to, float4 *dir, f32 *dist, f32 *dist_sq) {
    float4 d = to - from;
    *dist_sq = fmax(dot(d, d), EPS);
    *dist = sqrt(*dist_sq);
    *dir = d / *dist;
}

static inline u32 choose_emissive_triangle(RngState *rng, __global const TriangleVertices *etri_verts, u32 start,
                                           u32 n_triangles, f32 total_area) {
    if (n_triangles <= 1 || total_area <= EPS)
        return start;

    f32 r = random_float(rng) * total_area;
    f32 accum = 0.0f;
    for (u32 i = 0; i < n_triangles; i++) {
        TriangleVertices ev = etri_verts[start + i];
        accum += 0.5f * length(cross(ev.v1 - ev.v0, ev.v2 - ev.v0));
        if (r <= accum)
            return start + i;
    }

    return start + n_triangles - 1;
}

static inline Bary sample_barycentric(RngState *rng) {
    f32 r1 = random_float(rng);
    f32 r2 = random_float(rng);
    f32 s = sqrt(r1);

    Bary b;
    b.u = 1.0f - s;
    b.v = r2 * s;
    b.w = 1.0f - b.u - b.v;

    return b;
}

static inline EmissivePoint sample_emissive_point(RngState *rng, __global const TriangleVertices *etri_verts,
                                                  __global const TriangleNormals *etri_normals, u32 start,
                                                  u32 n_triangles, f32 total_area) {
    EmissivePoint p;
    p.tri_idx = choose_emissive_triangle(rng, etri_verts, start, n_triangles, total_area);
    p.bary = sample_barycentric(rng);

    TriangleVertices ev = etri_verts[p.tri_idx];
    p.pos = p.bary.w * ev.v0 + p.bary.u * ev.v1 + p.bary.v * ev.v2;

    TriangleNormals en = etri_normals[p.tri_idx];
    float4 n = p.bary.w * en.n0 + p.bary.u * en.n1 + p.bary.v * en.n2;
    if (length(n) > EPS)
        p.normal = normalize(n);
    else
        p.normal = normalize(cross(ev.v1 - ev.v0, ev.v2 - ev.v0));

    return p;
}

static inline float4 apply_emissive_texture(float4 color, u32 tex_index, const EmissivePoint *p,
                                            __global const TriangleUvs *etri_uvs, __global const u32 *etri_mat_index,
                                            __global const UvTransformBlock *uv_transforms,
                                            __global const TextureMeta *tex_meta, __global const u8 *tex_atlas) {
    if (tex_index == NO_TEXTURE)
        return color;
    TriangleUvs euv = etri_uvs[p->tri_idx];
    float2 uv = (float2)(p->bary.w * euv.uv0.x + p->bary.u * euv.uv1.x + p->bary.v * euv.uv2.x,
                         p->bary.w * euv.uv0.y + p->bary.u * euv.uv1.y + p->bary.v * euv.uv2.y);
    u32 mat_idx = etri_mat_index[p->tri_idx];
    float4 tex_color = sample_texture_uv(tex_meta, tex_atlas, uv_transforms, mat_idx, tex_index, uv, UV_SLOT_EMIS);

    return color * srgb_to_linear(tex_color);
}

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
    f32 cos_inner = cos(light->aux.x);
    f32 cos_outer = cos(light->aux.y);

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

    f32 falloff = spotlight_falloff(cos_theta, cos_inner, cos_outer);
    f32 solid_angle = 2.0f * PI * (1.0f - cos_outer);
    *power = light->power * falloff * solid_angle;
}

static inline void sample_textured_light(RngState *rng, Light *light, __global const TriangleVertices *etri_verts,
                                         __global const TriangleNormals *etri_normals,
                                         __global const TriangleUvs *etri_uvs, __global const u32 *etri_mat_index,
                                         __global const Material *materials,
                                         __global const UvTransformBlock *uv_transforms,
                                         __global const TextureMeta *tex_meta, __global const u8 *tex_atlas,
                                         float4 *origin, float4 *dir, float4 *power) {
    u32 tex_idx = as_uint(light->aux.x);
    u32 start = as_uint(light->aux.y);
    u32 n_triangles = as_uint(light->aux.z);
    f32 total_area = light->aux.w;

    EmissivePoint p = sample_emissive_point(rng, etri_verts, etri_normals, start, n_triangles, total_area);
    *dir = random_in_unit_hemisphere(rng, p.normal);
    *origin = p.pos + EPS * *dir;
    *power =
        apply_emissive_texture(light->power, tex_idx, &p, etri_uvs, etri_mat_index, uv_transforms, tex_meta, tex_atlas);
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
                         __global const Material *materials, __global const UvTransformBlock *uv_transforms,
                         __global const TextureMeta *tex_meta, __global const u8 *tex_atlas, float4 *origin,
                         float4 *dir, float4 *power) {
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
        sample_textured_light(rng, &light, etri_verts, etri_normals, etri_uvs, etri_mat_index, materials, uv_transforms,
                              tex_meta, tex_atlas, origin, dir, power);
    *power /= scale;
}

// selects a light source and evaluates its contribution to lighting at the given fragment
// with `pos` and `normal`, the returned bool denotes if the source contributes or not
static inline bool
sample_light_fragment(RngState *rng, float4 pos, float4 normal, __global const Light *lights, u32 n_lights,
                      __global const f32 *lum_pref_sum, f32 total_lum, __global const TriangleVertices *etri_verts,
                      __global const TriangleNormals *etri_normals, __global const TriangleUvs *etri_uvs,
                      __global const u32 *etri_mat_index, __global const UvTransformBlock *uv_transforms,
                      __global const TextureMeta *tex_meta, __global const u8 *tex_atlas, LightFragment *out) {
    if (n_lights == 0 || total_lum < EPS)
        return false;

    f32 scale;
    u32 light_idx = select_light(rng, lum_pref_sum, n_lights, total_lum, &scale);
    Light light = lights[light_idx];
    float4 flux = light_flux(&light, scale);

    float4 dir;
    f32 dist, dist_sq, n_dot_dir;
    float4 radiance;

    if (light.kind == LIGHT_POINT || light.kind == LIGHT_SPOT) {
        fill_light_vecs(pos, light.position, &dir, &dist, &dist_sq);
        n_dot_dir = dot(dir, normal);
        if (n_dot_dir <= 0.0f)
            return false;

        f32 cone_factor = 1.0f;
        if (light.kind == LIGHT_SPOT) {
            f32 cos_dir = dot(-dir, normalize(light.direction));
            cone_factor = spotlight_falloff(cos_dir, cos(light.aux.x), cos(light.aux.y));
            if (cone_factor < EPS)
                return false;
        }
        radiance = flux * cone_factor * n_dot_dir / dist_sq;

    } else if (light.kind == LIGHT_DIRECTIONAL) {
        dir = normalize(-light.direction);
        dist = INF;
        n_dot_dir = dot(dir, normal);
        if (n_dot_dir <= 0.0f)
            return false;
        radiance = flux * n_dot_dir;

    } else {
        u32 tex_index = as_uint(light.aux.x);
        u32 start = as_uint(light.aux.y);
        u32 n_triangles = as_uint(light.aux.z);
        f32 total_area = light.aux.w;
        if (total_area < EPS)
            return false;

        EmissivePoint p = sample_emissive_point(rng, etri_verts, etri_normals, start, n_triangles, total_area);
        fill_light_vecs(pos, p.pos, &dir, &dist, &dist_sq);
        n_dot_dir = dot(dir, normal);
        if (n_dot_dir <= 0.0f)
            return false;

        float4 emissive_flux =
            apply_emissive_texture(flux, tex_index, &p, etri_uvs, etri_mat_index, uv_transforms, tex_meta, tex_atlas);
        f32 cos_l = fmax(dot(-dir, p.normal), 0.0f);
        radiance = emissive_flux * n_dot_dir * cos_l / (PI * dist_sq);
    }

    if (dot(radiance, radiance) < EPS * EPS)
        return false;
    out->radiance = radiance;
    out->dir = dir;
    out->dist = dist;
    out->n_dot_dir = n_dot_dir;

    return true;
}

static inline float4 shadow_transmittance(float4 origin, float4 dir, f32 dist, __global const Material *materials,
                                          __global const BvhNode *tree, __global const TriangleVertices *tri_verts,
                                          __global const u32 *tri_mat_index, u32 n_tris) {
    float4 trans = WHITE;
    f32 t_min = EPS, t_max = dist - EPS;
    for (u32 depth = 0; depth < MAX_RAY_BOUNCES; depth++) {
        HitRecord hit_rec;
        if (!scene_intersect(tree, tri_verts, tri_mat_index, n_tris, origin, dir, t_min, t_max, &hit_rec))
            break;

        Material mat = materials[hit_rec.mat_index];
        if (mat.transmission < EPS)
            return BLACK;
        trans *= mat.base_color * mat.transmission;
        if (dot(trans, trans) < EPS)
            return BLACK;
        t_min = hit_rec.t + EPS;
    }

    return trans;
}

inline float4
direct_lighting_diffuse(RngState *rng, float4 pos, float4 normal, float4 base_color, __global const Light *lights,
                        u32 n_lights, __global const f32 *lum_pref_sum, f32 total_lum, float4 scene_center,
                        f32 scene_radius, __global const TriangleVertices *etri_verts,
                        __global const TriangleNormals *etri_normals, __global const TriangleUvs *etri_uvs,
                        __global const u32 *etri_mat_index, __global const Material *materials,
                        __global const UvTransformBlock *uv_transforms, __global const TextureMeta *tex_meta,
                        __global const u8 *tex_atlas, __global const BvhNode *tree,
                        __global const TriangleVertices *tri_verts, __global const u32 *tri_mat_index, u32 n_tris) {
    LightFragment frag;
    if (!sample_light_fragment(rng, pos, normal, lights, n_lights, lum_pref_sum, total_lum, etri_verts, etri_normals,
                               etri_uvs, etri_mat_index, uv_transforms, tex_meta, tex_atlas, &frag))
        return BLACK;
    float4 trans = shadow_transmittance(pos, frag.dir, frag.dist, materials, tree, tri_verts, tri_mat_index, n_tris);

    return frag.radiance * (base_color / PI) * trans;
}

inline float4
direct_lighting_specular(RngState *rng, float4 pos, float4 normal, float4 view, float4 f0, f32 roughness,
                         __global const Light *lights, u32 n_lights, __global const f32 *lum_pref_sum, f32 total_lum,
                         float4 scene_center, f32 scene_radius, __global const TriangleVertices *etri_verts,
                         __global const TriangleNormals *etri_normals, __global const TriangleUvs *etri_uvs,
                         __global const u32 *etri_mat_index, __global const Material *materials,
                         __global const UvTransformBlock *uv_transforms, __global const TextureMeta *tex_meta,
                         __global const u8 *tex_atlas, __global const BvhNode *tree,
                         __global const TriangleVertices *tri_verts, __global const u32 *tri_mat_index, u32 n_tris) {
    f32 n_dot_v = dot(view, normal);
    if (n_dot_v < EPS)
        return BLACK;

    LightFragment frag;
    if (!sample_light_fragment(rng, pos, normal, lights, n_lights, lum_pref_sum, total_lum, etri_verts, etri_normals,
                               etri_uvs, etri_mat_index, uv_transforms, tex_meta, tex_atlas, &frag))
        return BLACK;
    if (frag.n_dot_dir < EPS)
        return BLACK;

    f32 alpha = roughness * roughness;
    float4 h = normalize(frag.dir + view);
    f32 n_dot_h = fmax(dot(normal, h), 0.0f);

    f32 ggx_term = ggx_ndf(n_dot_h, alpha);
    float4 fresnel_term = fresnel4(f0, view, h);
    f32 smith_term = smith_g1_ggx(frag.n_dot_dir, alpha) * smith_g1_ggx(n_dot_v, alpha);
    float4 specular = frag.radiance * ggx_term * fresnel_term * smith_term / (4.0f * frag.n_dot_dir * n_dot_v);
    if (dot(specular, specular) < EPS * EPS)
        return BLACK;
    float4 trans = shadow_transmittance(pos, frag.dir, frag.dist, materials, tree, tri_verts, tri_mat_index, n_tris);

    return specular * trans;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_LIGHT_SAMPLING_H
