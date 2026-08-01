#ifndef PHOSPHOR_SCENE_READER_HPP
#define PHOSPHOR_SCENE_READER_HPP

#include "scene.hpp"

constexpr f32 LUMINOUS_EFF = 683.0f;

std::vector<Scene> read_file(const char *file_name);

#endif // PHOSPHOR_SCENE_READER_HPP
