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
    void add_light(const PointLight &light);
    void add_triangle(const Triangle &object);
    void add_camera(const Camera &camera);

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out) const;

    void emit(u32 photons_per_light, u32 max_bounces = 8);

    vec3 get_color(const vec3 &pos, const vec3 &normal, u32 n, Material &mat) const;
    Camera &get_camera() {
        if (chosen_camera < 0)
            throw std::runtime_error("No camera set");
        return cameras_[chosen_camera];
    }
    void set_camera(i32 i);
    std::vector<Triangle> &triangles() { return triangles_; }

    void generate_image(u32 image_height, u32 n, u32 photons_per_light, u32 max_bounces);

  private:
    void trace_photon(const Ray &r, vec3 power, u32 depth, u32 max_bounces);

    std::vector<Triangle> triangles_;
    std::vector<PointLight> lights_;
    std::vector<Camera> cameras_;
    i32 chosen_camera = -1;
    PhotonMap photon_map_;
};

#endif // PHOSPHOR_SCENE_HPP
