#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "common.hpp"
#include "material.hpp"
#include "photon.hpp"
#include "pointlight.hpp"
#include "random.hpp"
#include "ray.hpp"
#include "sphere.hpp"

#include <algorithm>
#include <vector>

class Scene {
  public:
    Scene() = default;

    void add_light(const PointLight &light);
    void add_object(const Sphere &object);

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out) const;

    void emit(u32 photons_per_light, u32 max_bounces = 8);

    const std::vector<Photon> &photons() const { return photons_; }
    vec3 get_color(const vec3 &pos, u32 n) const;

  private:
    void trace_photon(const Ray &r, vec3 power, u32 depth, u32 max_bounces);

    std::vector<Sphere> spheres_;
    std::vector<PointLight> lights_;
    std::vector<Photon> photons_;
};

#endif // PHOSPHOR_SCENE_HPP
