#include "light.hpp"

inline f32 bits_as_float(u32 x) { return *reinterpret_cast<f32 *>(&x); }

Light make_point_light(vec3 position, vec3 power) {
    Light l{};
    l.kind = LIGHT_POINT;
    l.position = vec3_to_float4(position);
    l.power = vec3_to_float4(power);
    return l;
}

Light make_spot_light(vec3 position, vec3 power, vec3 direction, f32 inner_rad, f32 outer_rad) {
    Light l{};
    l.kind = LIGHT_SPOT;
    l.position = vec3_to_float4(position);
    l.power = vec3_to_float4(power);
    l.direction = vec3_to_float4(direction);
    l.aux = float4{{inner_rad, outer_rad, 0.0f, 0.0f}};
    return l;
}

Light make_directional_light(vec3 direction, vec3 power, f32 radius) {
    Light l{};
    l.kind = LIGHT_DIRECTIONAL;
    l.direction = vec3_to_float4(direction);
    l.power = vec3_to_float4(power);
    l.aux = float4{{radius, 0.0f, 0.0f, 0.0f}};
    return l;
}

Light make_textured_light(u32 tex_index, u32 tri_start, u32 tri_count, vec3 power) {
    Light l{};
    l.kind = LIGHT_TEXTURED;
    l.power = vec3_to_float4(power);
    l.aux = float4{{bits_as_float(tex_index), bits_as_float(tri_start), bits_as_float(tri_count), 0.0f}};
    return l;
}
