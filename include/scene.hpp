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
#include <glm/glm.hpp>

struct SceneData {
    std::vector<HostTriangle> triangles;
    std::vector<Material> materials;
    std::vector<Light> lights;
    std::vector<f32> light_area_pref_sum;
    std::vector<Camera> cameras;
    std::vector<Texture> textures;
    std::optional<u32> chosen_camera{};
};

SceneData read_file(const char *file_name);

Light make_point_light(glm::vec3 position, glm::vec3 power);

#endif // PHOSPHOR_SCENE_HPP
