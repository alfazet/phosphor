#ifndef PHOSPHOR_RANDOM_HPP
#define PHOSPHOR_RANDOM_HPP

#include "common.hpp"
#include "triangle.hpp"

struct RngState {
    u32 state;
};

// values from https://github.com/imneme/pcg-c/blob/master/include/pcg_variants.h
inline u32 pcg_random(RngState &rng) {
    u32 oldstate = rng.state;
    rng.state = rng.state * 747796405u + 2891336453u;
    return (((oldstate >> ((oldstate >> 28u) + 4u)) ^ oldstate) * 277803737u) >> 16u;
}

inline f32 random_float(RngState &rng) { return static_cast<f32>(pcg_random(rng)) / 65535.0f; }

inline void pcg_seed(RngState &rng, u32 seed) {
    rng.state = 0u;
    rng.state = rng.state * 747796405u + 2891336453u;
    rng.state += seed;
    rng.state = rng.state * 747796405u + 2891336453u;
}

inline vec3 random_in_unit_sphere(RngState &rng) {
    vec3 p;
    do {
        p = 2.0f * vec3(random_float(rng), random_float(rng), random_float(rng)) - vec3(1.0f);
    } while (glm::dot(p, p) >= 1.0f);
    return p;
}

inline vec3 random_unit_vector(RngState &rng) { return glm::normalize(random_in_unit_sphere(rng)); }

inline vec3 random_in_hemisphere(RngState &rng, const vec3 &normal) {
    const vec3 v = random_unit_vector(rng);
    return (glm::dot(v, normal) > 0.0f) ? v : -v;
}

inline void make_tbn(const vec3 &n, vec3 &t, vec3 &b) {
    if (glm::abs(n.x) > glm::abs(n.y)) {
        // n crossed with (0, 1, 0)
        t = glm::normalize(vec3(-n.z, 0.0f, n.x));
    } else {
        // n crossed with (1, 0, 0)
        t = glm::normalize(vec3(0.0f, n.z, -n.y));
    }
    b = glm::cross(n, t);
}

// cosine-weighted random unit vector on a hemisphere (for diffuse)
inline vec3 random_in_hemisphere_cosine(RngState &rng, const vec3 &normal) {
    f32 r1 = random_float(rng);
    f32 r2 = random_float(rng);
    f32 phi = 2.0f * glm::pi<f32>() * r1;
    f32 sin_theta = glm::sqrt(r2);
    f32 cos_theta = glm::sqrt(1.0f - r2);
    vec3 local(sin_theta * glm::cos(phi), sin_theta * glm::sin(phi), cos_theta);
    vec3 tangent, bitangent;
    make_tbn(normal, tangent, bitangent);

    return glm::normalize(local.x * tangent + local.y * bitangent + local.z * normal);
}

// "Sampling the GGX Distribution of Visible Normals", Heitz 2018
inline vec3 ggx_sample_vndf(RngState &rng, const vec3 &normal, const vec3 &view, f32 roughness) {
    f32 alpha = roughness * roughness;
    f32 u1 = random_float(rng);
    f32 u2 = random_float(rng);

    vec3 tangent, bitangent;
    make_tbn(normal, tangent, bitangent);
    vec3 view_local = glm::normalize(vec3(glm::dot(view, tangent), glm::dot(view, bitangent), glm::dot(view, normal)));

    vec3 vh = glm::normalize(vec3(alpha * view_local.x, alpha * view_local.y, view_local.z));
    f32 len_sq = vh.x * vh.x + vh.y * vh.y;
    vec3 T1 = len_sq > 0 ? vec3(-vh.y, vh.x, 0.0f) * glm::inversesqrt(len_sq) : vec3(1.0f, 1.0f, 1.0f);
    vec3 T2 = glm::cross(vh, T1);

    f32 r = glm::sqrt(u1);
    f32 phi = 2.0f * glm::pi<f32>() * u2;
    float t1 = r * glm::cos(phi);
    float t2 = r * glm::sin(phi);
    float s = 0.5f * (1.0f + vh.z);
    t2 = (1.0f - s) * glm::sqrt(1.0f - t1 * t1) + s * t2;

    vec3 nh = t1 * T1 + t2 * T2 + glm::sqrt(glm::max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * vh;
    vec3 ne_local = glm::normalize(vec3(alpha * nh.x, alpha * nh.y, glm::max(0.0f, nh.z)));
    vec3 ne = glm::normalize(ne_local.x * tangent + ne_local.y * bitangent + ne_local.z * normal);

    return ne;
}

// Smith G1 masking function for the GGX distribution
// "Microfacet Models for Refraction through Rough Surfaces", Walter et al, eq 34
inline f32 smith_g1_ggx(f32 theta, f32 alpha) {
    if (theta <= 0.0f)
        return 0.0f;
    f32 tan = glm::tan(theta);
    return 2.0f / (1.0f + glm::sqrt(1.0f + alpha * alpha * tan * tan));
}

inline vec3 ggx_sample_direction(RngState &rng, const vec3 &incoming, const vec3 &normal, f32 roughness) {
    if (roughness < EPS)
        return glm::normalize(glm::reflect(incoming, normal));
    vec3 h = ggx_sample_vndf(rng, normal, -incoming, roughness);
    vec3 reflected = glm::reflect(incoming, h);
    if (glm::dot(reflected, normal) < 0.0f)
        return ZERO_VEC; // absorb if it reflected below the surface

    return glm::normalize(reflected);
}

static vec3 random_on_triangle(RngState &rng, const Triangle &tri, f32 &out_u, f32 &out_v) {
    f32 u = random_float(rng);
    f32 v = random_float(rng);
    if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
    }
    out_u = u;
    out_v = v;
    return tri.point_at(u, v);
}

#endif // PHOSPHOR_RANDOM_HPP
