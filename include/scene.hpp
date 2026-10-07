#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.hpp"
#include "light.hpp"
#include "material.h"
#include "texture.hpp"
#include "triangle.h"
#include "typedefs.h"

#include <optional>
#include <vector>

struct EnvMap {
    std::vector<f32> pixels{};
    u32 width = 0;
    u32 height = 0;
    bool loaded = false;
};

struct SceneData {
    std::vector<Triangle> triangles;
    std::vector<Triangle> emissive_triangles;
    std::vector<Material> materials;
    std::vector<Light> lights;
    std::vector<f32> luminance_pref_sum;
    std::vector<Camera> cameras;
    std::vector<Texture> textures;
    std::optional<u32> chosen_camera{};
    EnvMap envmap;

    Camera &get_camera();

    void build_luminance_pref_sum();

    void load_envmap(const char *path);
};

SceneData read_gltf_scene(const char *path);

#endif // PHOSPHOR_SCENE_HPP
