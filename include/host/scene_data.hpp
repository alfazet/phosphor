#ifndef PHOSPHOR_SCENE_DATA_HPP
#define PHOSPHOR_SCENE_DATA_HPP

#include "common/material.h"
#include "common/triangle.h"
#include "common/typedefs.h"
#include "host/camera.hpp"
#include "host/light.hpp"
#include "host/parsers/texture.hpp"

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
    std::vector<UvTransformBlock> uv_transforms;
    std::vector<Light> lights;
    std::vector<f32> lum_pref_sum;
    std::vector<Camera> cameras;
    std::vector<Texture> textures;
    std::optional<u32> chosen_camera{};
    EnvMap envmap;

    bool is_empty() const;

    bool has_camera() const;

    Camera &get_camera();

    void build_lum_pref_sum();

    void load_envmap(const char *path);
};

#endif // PHOSPHOR_SCENE_DATA_HPP
