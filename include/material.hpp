#ifndef PHOSPHOR_MATERIAL_HPP
#define PHOSPHOR_MATERIAL_HPP

#include "common.hpp"

struct Material {
    vec3 diff;
    vec3 spec;

    vec3 emissive;
};

#endif // PHOSPHOR_MATERIAL_HPP
