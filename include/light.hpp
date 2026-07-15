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
    u32 texture_index;
    u32 triangle_index;
};

#endif // PHOSPHOR_LIGHT_HPP
