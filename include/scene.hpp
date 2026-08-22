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

struct SceneData {
    std::vector<Triangle> triangles;
    std::vector<Material> materials;
    std::vector<Light> lights;
    std::vector<f32> light_area_pref_sum;
    std::vector<Camera> cameras;
    std::vector<Texture> textures;
    std::optional<u32> chosen_camera{};

    const Camera& get_camera() const;
};

SceneData read_gltf_scene(const char *path);

#endif // PHOSPHOR_SCENE_HPP
