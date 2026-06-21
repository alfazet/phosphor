#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "Material.hpp"
#include "common.hpp"
#include "photon.hpp"
#include "pointlight.hpp"
#include "ray.hpp"
#include "sphere.hpp"
#include <vector>
#include "random.hpp"
#include <algorithm>
#include <limits>

class Scene {
  public:
    Scene() = default;

    void AddLight(const Pointlight &light);
    void AddObject(const Sphere &object);

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out) const;

    void Emit(int photons_per_light, int max_bounces = 8);

    const std::vector<Photon> &photons() const { return photons_; }
    vec3 GetColor(const vec3 &pos, int n) const;

  private:
    void trace_photon(const Ray &r, vec3 power, int depth, int max_bounces);

    std::vector<Sphere> spheres_;
    std::vector<Pointlight> lights_;
    std::vector<Photon> photons_;
};

#endif // PHOSPHOR_SCENE_HPP