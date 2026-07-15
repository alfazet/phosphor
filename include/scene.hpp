#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "light.hpp"
#include "material.hpp"
#include "photonmap.hpp"
#include "ray.hpp"
#include "texture.hpp"

#include <vector>

class Scene {
  public:
    void add_point_light(const PointLight &light);
    void add_textured_light(const TexturedLight &light);
    void add_triangle(const Triangle &object);
    void add_camera(const Camera &camera);
    void add_texture(const Texture &texture);

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out, i32 &texture_index, vec2 &uv) const;

    void emit(u32 photons_per_light, u32 max_bounces = 8);

    vec3 get_color(const vec3 &pos, const vec3 &normal, u32 n, Material &mat, i32 &texture_index, vec2 &uv) const;
    Camera &get_camera() {
        if (chosen_camera < 0)
            throw std::runtime_error("No camera set");
        return cameras_[chosen_camera];
    }
    void set_camera(i32 i);

    void add_default_camera() {
        cameras_.emplace_back();
    }

    const std::vector<Triangle> &triangles() const { return triangles_; }
    const std::vector<Texture> &textures() const { return textures_; }

    void generate_image(RngState rng, u32 image_height, u32 n, u32 photons_per_light, u32 max_bounces);
    friend void print_spanning_box(const Scene &scene);

  private:
    void trace_photon(const Ray &r, vec3 power, u32 depth, u32 max_bounces);

    RngState rng;
    std::vector<Triangle> triangles_;
    std::vector<PointLight> point_lights_;
    std::vector<TexturedLight> textured_lights_;
    std::vector<Camera> cameras_;
    std::vector<Texture> textures_;
    i32 chosen_camera = -1;
    PhotonMap photon_map_;
};

#endif // PHOSPHOR_SCENE_HPP
