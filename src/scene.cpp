#include "scene.hpp"

#include "random.hpp"

void Scene::add_light(const PointLight &light) { lights_.push_back(light); }
void Scene::add_triangle(const Triangle &object) { triangles_.push_back(object); }

bool Scene::hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out) const {
    HitRecord temp;
    bool hit_anything = false;
    f32 closest = t_max;
    const Triangle *closest_triangle = nullptr;

    // space for improvement - do not check all objects in scene
    for (const auto &object : triangles_) {
        if (object.hit(r, t_min, closest, temp)) {
            hit_anything = true;
            closest = temp.t;
            rec = temp;
            closest_triangle = &object;
        }
    }

    if (hit_anything)
        mat_out = closest_triangle->mat();

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
    photon_map_.store({rec.point, power, phi, theta});

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

    photon_map_.build();
}

vec3 Scene::get_color(const vec3 &pos, const vec3 &normal, const u32 n) const {
    std::vector<const Photon *> nearest;
    photon_map_.locate(pos, n, 1.0f, nearest);
    if (nearest.empty())
        return vec3(0.0f);

    vec3 flux(0.0f);
    float max_dist_sq = 0.0f;
    for (auto p : nearest) {
        vec3 from(glm::cos(p->phi) * glm::sin(p->theta), glm::sin(p->phi) * glm::sin(p->theta), glm::cos(p->theta));
        // don't count photons coming from "inside" the surface
        if (glm::dot(from, normal) > 0.0f)
            continue;
        float dist = glm::dot(p->pos - pos, p->pos - pos);
        max_dist_sq = glm::max(max_dist_sq, dist);
        flux += p->power;
    }
    f32 area = glm::pi<f32>() * max_dist_sq;
    if (area < EPS)
        return vec3(0.0f);

    return flux / area;
}
