#ifndef PHOSPHOR_GLTFREADER_HPP
#define PHOSPHOR_GLTFREADER_HPP

#include "common.hpp"
#include "scene.hpp"
#include "vector"

std::vector<Scene> read_file(const char *file_name);

#endif // PHOSPHOR_GLTFREADER_HPP
