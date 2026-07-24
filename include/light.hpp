#ifndef PHOSPHOR_LIGHT_HPP
#define PHOSPHOR_LIGHT_HPP

#include "common.hpp"
#include "random.hpp"
#include "ray.hpp"

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
    u32 starting_tri_idx; // index into Scene::triangles_
    u32 n_triangles;
    // TODO: this is very memory intensive,
    // once we have a BVH this will be replaced by
    // accessing the total area of all meshes contained
    // in a given tree node
    std::vector<f32> area_pref_sum;

    LightSample sample_light(RngState &rng, const std::vector<Triangle> &triangles,
                             const std::vector<Texture> &textures, f32 photon_frac) const;
};

#endif // PHOSPHOR_LIGHT_HPP
