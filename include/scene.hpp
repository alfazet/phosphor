#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.hpp"
#include "common.hpp"
#include "image.hpp"
#include "light.hpp"
#include "material.hpp"
#include "photon_map.hpp"
#include "random.hpp"
#include "ray.hpp"
#include "texture.hpp"
#include "triangle.hpp"
#include "triangles.hpp"

#include <vector>

struct BoundingBox {
    vec3 min;
    vec3 max;
};

struct Scene {
    void add_point_light(const PointLight &light);
    void add_textured_light(const TexturedLight &light);
    void add_triangle(const Triangle &object);
    void add_camera(const Camera &camera);
    void add_texture(const Texture &texture);


    void emit(u32 photons_per_light, u32 max_bounces, u32 n_threads);
    void run_thread_emit(u32 id, u32 photons, ProgressScope &img_progress, u32 max_bounces, const vec3 photon_power,
                         const vec3 light_pos);
    void run_thread_textured_emit(u32 id, u32 photons, ProgressScope &img_progress, u32 max_bounces, f32 fraction,
                                  const TexturedLight &light);

    vec3 get_color(RngState &rng, const Ray &ray, const HitRecord &rec, u32 n, Material &mat, vec2 &uv, u32 depth_left);

    Camera &get_camera();

    void set_camera(i32 i);

    void add_default_camera();

    void run_thread_image_generation(u32 offset, u32 n_threads, ProgressScope &img_progress, Image &img,
                                     u32 image_height, u32 image_width, u32 n, u32 image_iters);
    void generate_image_row(Image &img, u32 row_number, u32 image_height, u32 image_width, u32 n, u32 image_iters);
    void generate_image(u32 image_height, u32 n, u32 photons_per_light, u32 max_bounces,
                        const char *output_path, u32 n_threads, u32 image_iters);
    BoundingBox get_bounding_box() const;

    void trace_photon(RngState &rng, u32 id, const Ray &r, vec3 power, u32 depth, u32 max_bounces);

    Triangles objects;
    std::vector<PointLight> point_lights;
    std::vector<TexturedLight> textured_lights;
    std::vector<Camera> cameras;
    std::vector<Texture> textures;
    i32 chosen_camera = -1;
    PhotonMap photon_map;
};

#endif // PHOSPHOR_SCENE_HPP
