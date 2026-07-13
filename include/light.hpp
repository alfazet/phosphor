#ifndef PHOSPHOR_LIGHT_HPP
#define PHOSPHOR_LIGHT_HPP

struct LightSample {
    Ray ray;
    vec3 power;
};

struct PointLight {
    vec3 position;
    vec3 power;
};

LightSample sample_point_light(const PointLight &l) { return {Ray(l.position, random_unit_vector()), l.power}; }

struct AreaLight {
    vec3 position;
    vec3 edge_u;
    vec3 edge_v;

    vec3 emission;
};

LightSample sample_area_light(const AreaLight &l) {
    float u = random_float();
    float v = random_float();

    vec3 pos = l.position + u * l.edge_u + v * l.edge_v;

    vec3 normal = normalize(cross(l.edge_u, l.edge_v));

    vec3 dir = random_in_hemisphere(normal);

    return {Ray(pos, dir), l.emission};
}

#endif // PHOSPHOR_LIGHT_HPP
