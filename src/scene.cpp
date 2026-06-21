#include "scene.hpp"

#include <ostream>

void Scene::AddLight(const Pointlight &light) { lights_.push_back(light); }

void Scene::AddObject(const Sphere &object) { spheres_.push_back(object); }

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

void Scene::trace_photon(const Ray &r, vec3 power, int depth, int max_bounces) {
    if (depth >= max_bounces)
        return;

    HitRecord rec;
    Material mat;
    if (!hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat))
        return;

    Photon p;
    p.pos = rec.point;
    p.power = power;
    p.phi = glm::atan(r.direction.y, r.direction.x);
    p.theta = glm::acos(glm::clamp(r.direction.z, -1.0f, 1.0f));
    photons_.push_back(p);

    const f32 xi = random_float();
    vec3 d = mat.diff;
    vec3 s = mat.spec;
    f32 P_r = glm::max(d.r + s.r, glm::max(d.g + s.g, d.b + s.b));
    f32 P_d = P_r*(d.r+d.g+d.b)/(d.r+d.g+d.b + s.r+s.g+s.b);
    f32 P_s = P_r - P_d;

    if (xi < P_d) {
        const vec3 new_dir = random_in_hemisphere(rec.normal);
        trace_photon(Ray(rec.point, new_dir), power * mat.color / P_d, depth + 1, max_bounces);
    } else if (xi < P_s + P_d) {
        const vec3 new_dir = glm::reflect(r.direction, rec.normal);
        trace_photon(Ray(rec.point, new_dir), power / P_s, depth + 1, max_bounces);
    }
}



void Scene::Emit(int photons_per_light, int max_bounces) {
    for (const auto &light : lights_) {
        const vec3 photon_power = vec3(light.power / static_cast<f32>(photons_per_light));
        for (int i = 0; i < photons_per_light; ++i) {
            const vec3 dir = random_unit_vector();
            trace_photon(Ray(light.pos, dir), photon_power, 0, max_bounces);
        }
    }
}

// naive and slow (and probably wrong)
vec3 Scene::GetColor(const vec3 &pos, int n) const {
    if (photons_.empty())
        return vec3(0.0f);

    std::vector<std::pair<f32, const Photon *>> dists;
    dists.reserve(photons_.size());
    for (const auto &p : photons_) {
        const vec3 diff = p.pos - pos;
        dists.emplace_back(glm::dot(diff, diff), &p);
    }

    std::sort(dists.begin(), dists.end(),[](const auto &a, const auto &b) { return a.first < b.first; });
    vec3 flux(0.0f);
    for (int i = 0; i < n; ++i)
        flux += dists[i].second->power;

    const f32 radius = glm::sqrt(dists[n - 1].first);
    const f32 area = glm::pi<f32>() * radius * radius;

    return flux / area;
}