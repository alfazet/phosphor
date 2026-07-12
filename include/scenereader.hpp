#ifndef PHOSPHOR_GLTFREADER_HPP
#define PHOSPHOR_GLTFREADER_HPP

#include "common.hpp"
#include "scene.hpp"
#include "vector"

std::vector<Scene> ReadFile(const char *fileName);

#endif // PHOSPHOR_GLTFREADER_HPP
