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

struct TexturedLight {
    usize tex_index;
    std::vector<Triangle> triangles;
    std::vector<f32> area_pref_sum;

    LightSample sample_light(RngState &rng, const Triangles &triangles, const std::vector<Texture> &textures,
                             f32 photon_frac) const;

    vec3 total_power(const std::vector<Texture> &textures) const;
};

#endif // PHOSPHOR_LIGHT_HPP
