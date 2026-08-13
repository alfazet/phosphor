#ifndef PHOSPHOR_SCENE_HPP
#define PHOSPHOR_SCENE_HPP

#include "camera.h"
#include "triangle.hpp"
#include "typedefs.h"

#include <vector>

struct SceneData {
    std::vector<Camera> cameras;

    std::vector<Triangle> triangles;
    std::vector<u8> tex_atlas; // all mipmap levels one after another
    std::vector<usize> tex_offsets;
    std::vector<usize> tex_widths;
    std::vector<usize> tex_heights;

    // TODO: port remaining fields

    // TODO: port scene loading
};

#endif // PHOSPHOR_SCENE_HPP
