#include "light.hpp"

LightSample PointLight::sample_light(RngState &rng) const {
    return {Ray(this->pos, random_unit_vector(rng)), this->power};
}

LightSample SpotLight::sample_light(RngState &rng) const {
    f32 cos_inner = glm::cos(this->inner);
    f32 cos_outer = glm::cos(this->outer);
    f32 r1 = random_float(rng);
    f32 r2 = random_float(rng);
    f32 cos_theta = 1.0f - r1 * (1.0f - cos_outer);
    f32 sin_theta = glm::sqrt(1.0f - cos_theta * cos_theta);
    f32 phi = 2.0f * PI * r2;

    vec3 local_dir(sin_theta * glm::cos(phi), sin_theta * glm::sin(phi), cos_theta);
    vec3 tangent, bitangent;
    make_tbn(this->dir, tangent, bitangent);
    vec3 world_dir = glm::normalize(local_dir.x * tangent + local_dir.y * bitangent + local_dir.z * this->dir);

    f32 t = glm::clamp((cos_theta - cos_outer) / (cos_inner - cos_outer), 0.0f, 1.0f);
    f32 falloff = t * t * (-2.0f * t + 3.0f); // smoothstep

    return {Ray(this->pos, world_dir), this->power * falloff};
}

void DirectionalLight::prepare(const BoundingBox &bbox) {
    make_tbn(this->dir, tangent, bitangent);
    auto center = vec3((bbox.x.start + bbox.x.end) * 0.5f, (bbox.y.start + bbox.y.end) * 0.5f,
                    (bbox.z.start + bbox.z.end) * 0.5f);
    const vec3 corners[8] = {
        {bbox.x.start, bbox.y.start, bbox.z.start}, {bbox.x.end, bbox.y.start, bbox.z.start},
        {bbox.x.start, bbox.y.end, bbox.z.start},   {bbox.x.end, bbox.y.end, bbox.z.start},
        {bbox.x.start, bbox.y.start, bbox.z.end},   {bbox.x.end, bbox.y.start, bbox.z.end},
        {bbox.x.start, bbox.y.end, bbox.z.end},     {bbox.x.end, bbox.y.end, bbox.z.end},
    };

    radius = 0.0f;
    for (const auto &corner : corners) {
        vec3 offset = corner - center;
        vec2 projected(glm::dot(offset, tangent), glm::dot(offset, bitangent));
        radius = glm::max(radius, glm::length(projected));
    }
    origin = center - this->dir * bbox.longest_size() * 10.0f;
}

LightSample DirectionalLight::sample_light(RngState &rng) const {
    // https://stackoverflow.com/questions/5837572/generate-a-random-point-within-a-circle-uniformly
    f32 r1 = radius * glm::sqrt(random_float(rng));
    f32 r2 = 2 * PI * random_float(rng);
    vec3 disk_offset = r1 * (glm::cos(r2) * tangent + glm::sin(r2) * bitangent);

    // vec3 total_power = power * (PI * radius * radius) ;
    vec3 total_power = power;
    return {Ray(origin + disk_offset, this->dir), total_power};
}

// sample a random triangle from this mesh with importance sampling weighted by area
LightSample TexturedLight::sample_light(RngState &rng, const Triangles &triangles, const std::vector<Texture> &textures) const {
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
    vec3 power = emission * total_area * PI;

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
