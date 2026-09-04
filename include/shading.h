#ifndef PHOSPHOR_SHADING_H
#define PHOSPHOR_SHADING_H

#ifdef __OPENCL_C_VERSION__

#include "bsdfs.h"
#include "constants.h"
#include "material.h"
#include "random.h"
#include "texture_meta.h"
#include "typedefs.h"
#include "utils.h"

inline float4 srgb_to_linear(float4 c) { return (float4)(pow(c.x, 2.2f), pow(c.y, 2.2f), pow(c.z, 2.2f), c.w); }

typedef struct ShadingContext {
    float4 base_color;
    float4 emissive;
    float4 shading_normal;
    f32 metallic;
    f32 roughness;
    f32 transmission;
    f32 ior;
} ShadingContext;

inline float4 apply_normal_map(float4 map_sample, float4 geom_normal, float4 tangent, float4 bitangent) {
    float4 n_ts = (float4)(map_sample.x * 2.0f - 1.0f, map_sample.y * 2.0f - 1.0f, map_sample.z * 2.0f - 1.0f, 0.0f);
    float4 perturbed = n_ts.x * tangent + n_ts.y * bitangent + n_ts.z * geom_normal;
    f32 len = length(perturbed);

    return (len > EPS) ? (perturbed / len) : geom_normal;
}

// evaluate all material parameters at a surface hit point by sampling textures
//
// returns a struct with:
//   base_color     – can be tinted by volumetric attenuation
//   emissive       – emitted radiance (for emissive meshes)
//   metallic       – 0 = dielectric, 1 = metallic
//   roughness      – roughness
//   transmission   – fraction of light that passes through
//   ior            – index of refraction
//   shading_normal – new normal after applying the normal map
inline ShadingContext evaluate_material(const Material *mat, float2 uv, float4 geom_normal, float4 tangent,
                                        float4 bitangent, __global const TextureMeta *tex_meta,
                                        __global const u8 *tex_atlas, float4 vol_trans) {
    ShadingContext ctx;

    ctx.base_color = mat->base_color * vol_trans;
    if (mat->diff_index != NO_TEXTURE) {
        float4 tex = sample_texture_uv(mat, tex_meta, tex_atlas, mat->diff_index, uv, mat->diff_transform);
        ctx.base_color *= srgb_to_linear(tex);
    }

    ctx.emissive = mat->emissive;
    if (mat->emis_index != NO_TEXTURE) {
        float4 tex = sample_texture_uv(mat, tex_meta, tex_atlas, mat->emis_index, uv, mat->emis_transform);
        ctx.emissive *= srgb_to_linear(tex);
    }

    ctx.metallic = mat->metallic;
    ctx.roughness = mat->roughness;
    if (mat->metal_rough_index != NO_TEXTURE) {
        float4 mr = sample_texture_uv(mat, tex_meta, tex_atlas, mat->metal_rough_index, uv, mat->metal_rough_transform);
        ctx.metallic *= mr.z;  // B channel
        ctx.roughness *= mr.y; // G channel
    }
    ctx.roughness = clamp(ctx.roughness, MIN_ROUGHNESS, 1.0f);

    ctx.transmission = mat->transmission;
    ctx.ior = mat->ior;

    ctx.shading_normal = geom_normal;
    if (mat->norm_index != NO_TEXTURE) {
        float4 map_sample = sample_texture_uv(mat, tex_meta, tex_atlas, mat->norm_index, uv, mat->norm_transform);
        ctx.shading_normal = apply_normal_map(map_sample, geom_normal, tangent, bitangent);
    }

    return ctx;
}

inline float4 beer_lambert(float4 att_color, f32 att_dist, f32 travel) {
    if (att_dist < EPS)
        return WHITE;
    float4 sigma =
        (float4)(-log(fmax(att_color.x, EPS)), -log(fmax(att_color.y, EPS)), -log(fmax(att_color.z, EPS)), 0.0f) /
        att_dist;

    return (float4)(exp(-sigma.x * travel), exp(-sigma.y * travel), exp(-sigma.z * travel), 0.0f);
}

// diffuse scatter: store the photon and scatter the ray
#define BSDF_DIFFUSE 0
// metallic reflection: store the photon and reflect the ray
#define BSDF_METALLIC 1
// Fresnel dielectric reflection: don't store the photon and reflect the ray
#define BSDF_FRESNEL 2
// transmission (after refraction): don't store the photon and refract the ray
#define BSDF_TRANSMIT 3

typedef struct BsdfSample {
    float4 dir;
    float4 throughput;
    u32 event;
} BsdfSample;

inline BsdfSample sample_bsdf(RngState *rng, const ShadingContext *ctx, float4 shading_normal, float4 geom_normal,
                              float4 view, f32 *curr_ior, bool front_face) {
    BsdfSample s;
    float4 h = ggx_sample_vndf(rng, shading_normal, view, ctx->roughness);

    if (random_float(rng) < ctx->metallic) {
        // metallic reflection
        float4 reflected = reflect(-view, h);
        if (dot(reflected, geom_normal) <= 0.0f)
            reflected = reflect(-view, geom_normal);
        s.dir = reflected;
        s.throughput = fresnel4(ctx->base_color, view, h);
        s.event = BSDF_METALLIC;
        return s;
    }

    f32 ior_1 = front_face ? *curr_ior : ctx->ior;
    f32 ior_2 = front_face ? ctx->ior : *curr_ior;
    f32 fr = fresnel_refracted(ior_1, ior_2, view, h);

    if (random_float(rng) < fr) {
        // dielectric reflection
        float4 reflected = reflect(-view, h);
        s.dir = reflected;
        s.throughput = (float4)(fr, fr, fr, 0.0f);
        s.event = BSDF_FRESNEL;
        return s;
    }

    if (random_float(rng) < ctx->transmission) {
        // dielectric transmission
        bool tir;
        float4 refracted = refract(-view, shading_normal, ior_1, ior_2, &tir);
        if (tir) {
            float4 reflected = reflect(-view, shading_normal);
            s.dir = reflected;
            s.throughput = WHITE;
            s.event = BSDF_FRESNEL;
            return s;
        }
        bool transmitted = dot(refracted, shading_normal) < 0.0f;
        *curr_ior = transmitted ? (front_face ? ctx->ior : AIR_IOR) : *curr_ior;
        s.dir = refracted;
        s.throughput = (float4)(ctx->transmission, ctx->transmission, ctx->transmission, 0.0f) * ctx->base_color;
        s.event = BSDF_TRANSMIT;
        return s;
    }

    // diffuse
    s.dir = random_in_unit_hemisphere(rng, shading_normal);
    s.throughput = ctx->base_color;
    s.event = BSDF_DIFFUSE;

    return s;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_SHADING_H
