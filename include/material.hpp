#ifndef PHOSPHOR_MATERIAL_HPP
#define PHOSPHOR_MATERIAL_HPP

#include "common.hpp"
#include <optional>

struct Material {

    vec4 base_color;
    f32 metallic;
    f32 roughness;
    vec3 emissive;

    std::optional<usize> diff_index;
    std::optional<usize> emis_index;
    std::optional<usize> norm_index;
    std::optional<usize> occlusion_index;
    std::optional<usize> metal_rough_index;
    f32 norm_scale = 1;
};

#endif // PHOSPHOR_MATERIAL_HPP
