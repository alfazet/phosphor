#ifndef PHOSPHOR_SCENE_READER_HPP
#define PHOSPHOR_SCENE_READER_HPP

#include "scene.hpp"

std::vector<Scene> read_file(const char *file_name, RngState rng);

#endif // PHOSPHOR_SCENE_READER_HPP
