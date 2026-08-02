#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "bounding_box.hpp"
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

struct Scene {
    Triangles objects;
    std::vector<PointLight> point_lights;
    std::vector<SpotLight> spot_lights;
    std::vector<DirectionalLight> dir_lights;
    std::vector<TexturedLight> textured_lights;
    std::vector<Camera> cameras;
    std::vector<Texture> textures;
    i32 chosen_camera = -1;
    PhotonMap photon_map;

    void add_point_light(const PointLight &light);
    void add_textured_light(const TexturedLight &light);
    void add_directional_light(const DirectionalLight &light);
    void add_camera(const Camera &camera);
    void add_texture(const Texture &texture);

    void emit(RngState &rng, u32 photons_per_light, u32 max_bounces, u32 n_threads);

    vec3 get_color(RngState &rng, const Ray &ray, const HitRecord &rec, u32 n, Material &mat, vec2 &uv,
                   u32 bounces_left, f32 curr_ior, f32 lod);

    Camera &get_camera();

    void set_camera(i32 i);

    void add_default_camera();

    void run_thread_image_generation(RngState rng, u32 offset, u32 n_threads, ProgressScope &img_progress, Image &img,
                                     u32 image_height, u32 image_width, u32 n_samples, u32 ray_depth, u32 image_iters);
    void generate_image_row(RngState &rng, Image &img, u32 row_number, u32 image_height, u32 image_width, u32 n_samples,
                            u32 ray_depth, u32 image_iters);
    void generate_image(RngState rng, u32 image_height, u32 n_samples, u32 photons_per_light, u32 max_photon_bounces,
                        u32 ray_bounces, const char *output_path, u32 n_threads, u32 image_iters);
    BoundingBox get_bounding_box() const;

    void trace_photon(RngState &rng, u32 id, const Ray &r, vec3 power, u32 depth, u32 max_bounces, f32 curr_ior);

    template <class LightList, class SampleFn>
    void emit_light_group(RngState &rng, const LightList &lights, f32 total_light_power, u32 total_photons,
                          u32 max_bounces, u32 n_threads, ProgressScope &progress, SampleFn &&sample);
};

#endif // PHOSPHOR_SCENE_HPP
