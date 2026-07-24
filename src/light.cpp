#include "light.hpp"

LightSample PointLight::sample_light(RngState &rng) const { return {Ray(this->pos, random_unit_vector(rng)), power}; }

LightSample AreaLight::sample_light(RngState &rng) const {
    float u = random_float(rng);
    float v = random_float(rng);
    vec3 pos = this->pos + u * this->edge_u + v * this->edge_v;
    vec3 normal = glm::normalize(glm::cross(this->edge_u, this->edge_v));
    vec3 dir = random_in_unit_hemisphere(rng, normal);

    return {Ray(pos, dir), emission};
}

// sample a random triangle from this mesh with importance sampling weighted by area
LightSample TexturedLight::sample_light(RngState &rng, const std::vector<Triangle> &triangles,
                                        const std::vector<Texture> &textures, f32 photon_frac) const {
    auto pref_sum = this->area_pref_sum;
    f32 total_area = pref_sum.back();
    usize sample_idx =
        std::lower_bound(pref_sum.begin(), pref_sum.end(), random_float(rng) * total_area) - pref_sum.begin();

    const Texture &tex = textures[this->tex_index];
    const Triangle &tri = triangles[sample_idx];
    f32 u = random_float(rng);
    f32 v = random_float(rng);
    if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
    }
    vec3 point = tri.point_at(u, v);
    vec2 uv = tri.uv_at(u, v);
    vec3 emission = sample(&tex, uv);
    vec3 normal = tri.get_normal(vec2(u, v), textures);
    point += normal * 0.001f;
    vec3 dir = random_in_unit_hemisphere(rng, normal);
    vec3 power = emission * total_area * glm::pi<f32>() * photon_frac;

    return {Ray(point, dir), power};
}
