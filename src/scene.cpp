#include "scene.hpp"

void Scene::add_light(const PointLight &light) { lights_.push_back(light); }

void Scene::add_object(const Sphere &object) { spheres_.push_back(object); }

bool Scene::hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out) const {
    HitRecord temp;
    bool hit_anything = false;
    f32 closest = t_max;
    const Sphere *closest_sphere = nullptr;

    for (const auto &object : spheres_) {
        if (object.hit(r, t_min, closest, temp)) {
            hit_anything = true;
            closest = temp.t;
            rec = temp;
            closest_sphere = &object;
        }
    }

    if (hit_anything)
        mat_out = closest_sphere->mat();

    return hit_anything;
}

void Scene::trace_photon(const Ray &r, vec3 power, u32 depth, u32 max_bounces) {
    if (depth >= max_bounces)
        return;

    HitRecord rec;
    Material mat;
    if (!hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat))
        return;

    f32 phi = glm::atan(r.direction.y, r.direction.x);
    f32 theta = glm::acos(glm::clamp(r.direction.z, -1.0f, 1.0f));
    photons_.emplace_back(rec.point, power, phi, theta);

    // source: page 63
    const f32 xi = random_float();
    vec3 d = mat.diff;
    vec3 s = mat.spec;
    f32 rho_r = glm::max(d.r + s.r, glm::max(d.g + s.g, d.b + s.b));
    f32 rho_d = rho_r * (d.r + d.g + d.b) / (d.r + d.g + d.b + s.r + s.g + s.b);
    f32 rho_s = rho_r - rho_d;

    if (xi < rho_d) {
        const vec3 new_dir = random_in_hemisphere(rec.normal);
        trace_photon(Ray(rec.point, new_dir), power * mat.diff / rho_d, depth + 1, max_bounces);
    } else if (xi < rho_s + rho_d) {
        const vec3 new_dir = glm::reflect(r.direction, rec.normal);
        trace_photon(Ray(rec.point, new_dir), power * mat.spec / rho_s, depth + 1, max_bounces);
    }
    // else the photon is absorbed
}

void Scene::emit(u32 photons_per_light, u32 max_bounces) {
    for (const auto &light : lights_) {
        const vec3 photon_power = vec3(light.power / static_cast<f32>(photons_per_light));
        for (u32 i = 0; i < photons_per_light; i++) {
            const vec3 dir = random_unit_vector();
            trace_photon(Ray(light.pos, dir), photon_power, 0, max_bounces);
        }
    }
}

// naive and slow (and probably wrong)
vec3 Scene::get_color(const vec3 &pos, const u32 n) const {
    u32 k = n;
    if (photons_.empty())
        return vec3(0.0f);

    std::vector<std::pair<f32, const Photon *>> dists;
    dists.reserve(photons_.size());
    for (const auto &p : photons_) {
        const vec3 diff = p.pos - pos;
        dists.emplace_back(glm::dot(diff, diff), &p);
    }

    if (dists.size() < n)
        k = dists.size();

    // this will take a long time
    std::sort(dists.begin(), dists.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    vec3 flux(0.0f);
    for (u32 i = 0; i < k; i++)
        flux += dists[i].second->power;
    const f32 radius = glm::sqrt(dists[k - 1].first);
    const f32 area = glm::pi<f32>() * radius * radius;

    return flux / area;
}
