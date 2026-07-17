#ifndef PHOSPHOR_MATERIAL_HPP
#define PHOSPHOR_MATERIAL_HPP

#include "common.hpp"
#include <optional>

struct Material {
    vec3 diff;
    vec3 spec;

    vec3 emissive;

    std::optional<usize> diff_index;
    std::optional<usize> emis_index;
    std::optional<usize> norm_index;
    f32 norm_scale = 1;
};

#endif // PHOSPHOR_MATERIAL_HPP
