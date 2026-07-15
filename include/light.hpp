#ifndef PHOSPHOR_LIGHT_HPP
#define PHOSPHOR_LIGHT_HPP

#include "common.hpp"
#include "ray.hpp"

struct LightSample {
    Ray ray;
    vec3 power;
};

struct PointLight {
    vec3 pos;
    vec3 power;
};

struct AreaLight {
    vec3 position;
    vec3 edge_u;
    vec3 edge_v;

    vec3 emission;
};

struct TexturedLight {
    usize tex_index;
    u32 triangle_start; // index into Scene::triangles_
    u32 triangle_count;
    f32 total_area;
};

#endif // PHOSPHOR_LIGHT_HPP
