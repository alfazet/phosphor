#ifndef PHOSPHOR_LIGHT_HPP
#define PHOSPHOR_LIGHT_HPP

#include "common/light.h"
#include "common/typedefs.h"
#include "host/glm_bundle.hpp"

Light make_point_light(vec3 position, vec3 power);

Light make_spot_light(vec3 position, vec3 power, vec3 direction, f32 inner_rad, f32 outer_rad);

Light make_directional_light(vec3 direction, vec3 power);

Light make_textured_light(u32 tex_index, u32 tri_start, u32 tri_count, vec3 power, f32 total_area);

#endif // PHOSPHOR_LIGHT_HPP
