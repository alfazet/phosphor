#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.h"
#include "host_triangle.hpp"
#include "light.h"
#include "material.h"
#include "texture.hpp"
#include "typedefs.h"

#include <optional>
#include <vector>

struct SceneData {
    std::vector<HostTriangle> triangles;
    std::vector<Material> materials;
    std::vector<Light> lights;
    std::vector<f32> light_area_pref_sum;
    std::vector<Camera> cameras;
    std::vector<Texture> textures;
    std::optional<Camera> chosen_camera{};
};

#endif // PHOSPHOR_SCENE_HPP
