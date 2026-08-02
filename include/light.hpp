#ifndef PHOSPHOR_LIGHT_HPP
#define PHOSPHOR_LIGHT_HPP

#include "common.hpp"
#include "random.hpp"
#include "ray.hpp"
#include "triangle.hpp"
#include "triangles.hpp"

struct LightSample {
    Ray ray;
    vec3 power;

    LightSample sample_light(RngState &rng) const;
};

struct PointLight {
    vec3 pos;
    vec3 power;

    LightSample sample_light(RngState &rng) const;
};

struct SpotLight {
    vec3 pos;
    vec3 power;
    vec3 dir;
    f32 inner;
    f32 outer;

    LightSample sample_light(RngState &rng) const;
};

struct DirectionalLight {
    vec3 dir;
    vec3 power;

    vec3 tangent;
    vec3 bitangent;
    vec3 origin;
    f32 radius;

    void prepare(const BoundingBox &bbox);
    LightSample sample_light(RngState &rng) const;
};

struct TexturedLight {
    usize tex_index;
    std::vector<Triangle> triangles;
    std::vector<f32> area_pref_sum;
    vec3 strength;

    LightSample sample_light(RngState &rng, const Triangles &triangles, const std::vector<Texture> &textures) const;

    vec3 total_power(const std::vector<Texture> &textures) const;
};

inline vec3 light_power(const PointLight &l, const std::vector<Texture> &) { return l.power; }
inline vec3 light_power(const SpotLight &l, const std::vector<Texture> &) { return l.power; }
inline vec3 light_power(const DirectionalLight &l, const std::vector<Texture> &) { return l.power; }
inline vec3 light_power(const TexturedLight &l, const std::vector<Texture> &tex) { return l.total_power(tex); }

#endif // PHOSPHOR_LIGHT_HPP
