#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "photonmap.hpp"
#include "pointlight.hpp"
#include "ray.hpp"

#include <vector>

class Scene {
  public:
    Scene() {}

    void add_light(const PointLight &light);
    void add_triangle(const Triangle &object);

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out) const;

    void emit(u32 photons_per_light, u32 max_bounces = 8);

    vec3 get_color(const vec3 &pos, const vec3 &normal, u32 n) const;
    Camera &get_camera() { return camera_; }
    void set_camera(const Camera &camera) { camera_ = camera; }
    std::vector<Triangle> &triangles() { return triangles_; }

  private:
    void trace_photon(const Ray &r, vec3 power, u32 depth, u32 max_bounces);

    std::vector<Triangle> triangles_;
    std::vector<PointLight> lights_;
    Camera camera_;
    PhotonMap photon_map_;
};

#endif // PHOSPHOR_SCENE_HPP
