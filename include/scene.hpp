#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "light.hpp"
#include "material.hpp"
#include "photon_map.hpp"
#include "random.hpp"
#include "ray.hpp"
#include "texture.hpp"
#include "image.hpp"

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

    void emit(u32 photons_per_light, u32 max_bounces = 8);

    vec3 get_color(const vec3 &pos, const vec3 &normal, u32 n, Material &mat, vec2 &uv) const;

    Camera &get_camera();

    void set_camera(i32 i);

    void add_default_camera() {
        BoundingBox b = get_bounding_box();
        Camera def = Camera();
        cameras_.emplace_back(b.min, b.max, def.hfov, def.aspect_ratio);
    }

    const std::vector<Triangle> &triangles() const { return triangles_; }
    const std::vector<Texture> &textures() const { return textures_; }

    void generate_row(Image &img, u32 row_number, u32 image_height, u32 image_width, u32 n);
    void generate_image(RngState rng, u32 image_height, u32 n, u32 photons_per_light, u32 max_bounces,
                        const char *output_path, u32 thread_number);
    BoundingBox get_bounding_box() const;

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
