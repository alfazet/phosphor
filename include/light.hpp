#ifndef PHOSPHOR_LIGHT_HPP
#define PHOSPHOR_LIGHT_HPP

#include "common.hpp"
#include "random.hpp"
#include "ray.hpp"
#include "triangle.hpp"
#include "triangles.hpp"

#include <algorithm>

#include <algorithm>

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

struct AreaLight {
    vec3 pos;
    vec3 edge_u;
    vec3 edge_v;
    vec3 emission;

    LightSample sample_light(RngState &rng) const;
};

struct TexturedLight {
    usize tex_index;
    std::vector<Triangle> triangles;
    std::vector<f32> area_pref_sum;

    LightSample sample_light(RngState &rng, const Triangles &triangles, const std::vector<Texture> &textures,
                             f32 photon_frac) const;
};

#endif // PHOSPHOR_LIGHT_HPP
