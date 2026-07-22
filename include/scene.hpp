#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "image.hpp"
#include "light.hpp"
#include "material.hpp"
#include "photon_map.hpp"
#include "random.hpp"
#include "ray.hpp"
#include "texture.hpp"

#include <optional>
#include <ostream>
#include <vector>

struct BoundingBox {
    vec3 min;
    vec3 max;
};

class Scene {
  public:
    void add_point_light(const PointLight &light);
    void add_textured_light(const TexturedLight &light);
    void add_triangle(const Triangle &object);
    void add_camera(const Camera &camera);
    void add_texture(const Texture &texture);

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out, vec2 &uv) const;

    void emit(u32 photons_per_light, u32 max_bounces, u32 thread_number);
    void run_thread_emit(u32 id, u32 photons, ProgressScope &img_progress, u32 max_bounces, const vec3 photon_power, const vec3 light_pos);
    void run_thread_textured_emit(u32 id, u32 photons, ProgressScope &img_progress, u32 max_bounces, f32 fraction, const TexturedLight &light);

    vec3 get_color(const Ray &ray, const HitRecord& rec, u32 n, Material &mat, vec2 &uv, u32 depth_left);

    Camera &get_camera();

    void set_camera(i32 i);

    void add_default_camera();

    const std::vector<Triangle> &triangles() const { return triangles_; }
    const std::vector<Texture> &textures() const { return textures_; }

    void generate_row(Image &img, u32 row_number, u32 image_height, u32 image_width, u32 n, u32 sample_number);
    void run_thread_image_generation(u32 offset, u32 thread_number, ProgressScope &img_progress, Image &img, u32 image_height,
                    u32 image_width, u32 n, u32 sample_number);
    void generate_image(RngState rng, u32 image_height, u32 n, u32 photons_per_light, u32 max_bounces,
                        const char *output_path, u32 thread_number, u32 sample_number);
    BoundingBox get_bounding_box() const;

  private:
    void trace_photon(u32 id, const Ray &r, vec3 power, u32 depth, u32 max_bounces);

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
