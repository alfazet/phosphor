#ifndef PHOSPHOR_BSDFS_H
#define PHOSPHOR_BSDFS_H

#ifdef __OPENCL_C_VERSION__

#include "constants.h"
#include "random.h"
#include "typedefs.h"

// "Sampling the GGX Distribution of Visible Normals", Heitz 2018
inline float4 ggx_sample_vndf(RngState *rng, float4 normal, float4 view, f32 roughness) {
    f32 alpha = roughness * roughness;
    f32 u1 = random_float(rng);
    f32 u2 = random_float(rng);

    float4 tangent, bitangent;
    make_tbn(normal, &tangent, &bitangent);

    float4 view_local = normalize((float4)(dot(view, tangent), dot(view, bitangent), dot(view, normal), 0.0f));

    float4 vh = normalize((float4)(alpha * view_local.x, alpha * view_local.y, view_local.z, 0.0f));
    f32 len_sq = vh.x * vh.x + vh.y * vh.y;

    float4 T1 =
        (len_sq > 0.0f) ? (float4)(-vh.y, vh.x, 0.0f, 0.0f) * (1.0f / sqrt(len_sq)) : (float4)(1.0f, 1.0f, 1.0f, 0.0f);
    float4 T2 = cross(vh, T1);

    f32 r = sqrt(u1);
    f32 phi = 2.0f * PI * u2;
    f32 t1 = r * cos(phi);
    f32 t2 = r * sin(phi);
    f32 s = 0.5f * (1.0f + vh.z);
    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;

    f32 nh_z_sq = fmax(0.0f, 1.0f - t1 * t1 - t2 * t2);
    float4 nh = t1 * T1 + t2 * T2 + sqrt(nh_z_sq) * vh;

    float4 ne_local = normalize((float4)(alpha * nh.x, alpha * nh.y, fmax(0.0f, nh.z), 0.0f));
    float4 ne = normalize(ne_local.x * tangent + ne_local.y * bitangent + ne_local.z * normal);

    return ne;
}

// Smith G1 masking function for the GGX distribution
// "Microfacet Models for Refraction through Rough Surfaces", Walter et al, eq 34
inline f32 smith_g1_ggx(f32 theta, f32 alpha) {
    if (theta <= 0.0f)
        return 0.0f;
    f32 tan_theta = tan(theta);
    return 2.0f / (1.0f + sqrt(1.0f + alpha * alpha * tan_theta * tan_theta));
}

inline f32 fresnel(f32 R_0, float4 incoming, float4 normal) {
    f32 cos_theta = fabs(dot(incoming, normal));
    return R_0 + (1.0f - R_0) * pow(1.0f - cos_theta, 5);
}

// https://en.wikipedia.org/wiki/Schlick's_approximation
inline f32 fresnel_refracted(f32 ior_1, f32 ior_2, float4 incoming, float4 normal) {
    f32 R_0 = pow((ior_1 - ior_2) / (ior_1 + ior_2), 2);
    return fresnel(R_0, incoming, normal);
}

inline float4 reflect(float4 incoming, float4 normal) {
    float4 reflected = normalize(incoming - 2.0f * dot(incoming, normal) * normal);
    if (dot(reflected, normal) < 0.0f)
        return (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    return reflected;
}

inline float4 refract(float4 incoming, float4 normal, f32 eta1, f32 eta2) {
    f32 eta = eta1 / eta2;
    f32 cos_i = dot(incoming, normal);
    f32 sin_t2 = eta * eta * (1.0f - cos_i * cos_i);

    if (sin_t2 > 1.0f)
        return reflect(incoming, normal);

    float4 refracted = eta * incoming - (eta * cos_i + sqrt(1.0f - sin_t2)) * normal;
    return normalize(refracted);
}

inline float4 reflect_or_refract(RngState *rng, float4 incoming, float4 normal, f32 curr_ior, f32 mat_ior,
                                 f32 mat_transmission, bool front_face) {
    f32 eta1 = front_face ? curr_ior : mat_ior;
    f32 eta2 = front_face ? mat_ior : curr_ior;
    f32 reflection_prob = fresnel_refracted(eta1, eta2, -incoming, normal);
    f32 x = random_float(rng);

    if (x < reflection_prob) {
        return reflect(incoming, normal);
    } else if (x < (1.0f - reflection_prob) * mat_transmission) {
        return refract(incoming, normal, eta1, eta2);
    }

    return (float4)(0.0f, 0.0f, 0.0f, 0.0f);
}

inline float4 ggx_sample_direction(RngState *rng, float4 incoming, float4 normal, f32 roughness, f32 curr_ior,
                                   f32 mat_ior, f32 mat_transmission, bool front_face) {
    if (roughness < EPS)
        return reflect_or_refract(rng, incoming, normal, curr_ior, mat_ior, mat_transmission, front_face);

    float4 h = ggx_sample_vndf(rng, normal, -incoming, roughness);
    float4 outcome = reflect_or_refract(rng, incoming, h, curr_ior, mat_ior, mat_transmission, front_face);

    if (length(outcome) < EPS)
        return -incoming;
    return normalize(outcome);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_BSDFS_H
