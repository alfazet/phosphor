#include "light.hpp"

LightSample PointLight::sample_light(RngState &rng) const {
    return {Ray(this->pos, random_unit_vector(rng)), this->power};
}

LightSample SpotLight::sample_light(RngState &rng) const {
    // TODO: implement
    return {};
}

// sample a random triangle from this mesh with importance sampling weighted by area
LightSample TexturedLight::sample_light(RngState &rng, const Triangles &triangles, const std::vector<Texture> &textures,
                                        f32 photon_frac) const {
    auto pref_sum = this->area_pref_sum;
    f32 total_area = pref_sum.back();
    usize sample_idx =
        std::lower_bound(pref_sum.begin(), pref_sum.end(), random_float(rng) * total_area) - pref_sum.begin();

    const Texture &tex = textures[this->tex_index];
    const Triangle &tri = triangles.at(sample_idx);
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
    vec3 power = emission * total_area * PI * photon_frac;

    return {Ray(point, dir), power};
}

// estimate the total power of an emissive light source
// by sampling in the middle of each triangle - this is only used once
// to decide how to distribute photons between the light sources so it's
// fine even if it's not that accurate
vec3 TexturedLight::total_power(const std::vector<Texture> &textures) const {
    vec3 total_power = vec3(0.0f);
    const Texture &tex = textures[this->tex_index];
    for (const auto &tri : this->triangles) {
        vec3 emission = sample(&tex, tri.uv_at(0.5, 0.5));
        total_power += emission * tri.area() * glm::pi<f32>();
    }

    return total_power;
}
