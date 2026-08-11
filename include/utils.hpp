#ifndef PHOSPHOR_UTILS_HPP
#define PHOSPHOR_UTILS_HPP

#include "common.hpp"

inline vec2 compute_bary(vec2 bary, vec2 v0, vec2 v1, vec2 v2) {
    return (1.0f - bary.x - bary.y) * v0 + bary.x * v1 + bary.y * v2;
}

inline vec3 compute_bary(vec2 bary, vec3 v0, vec3 v1, vec3 v2) {
    return (1.0f - bary.x - bary.y) * v0 + bary.x * v1 + bary.y * v2;
}

#endif // PHOSPHOR_UTILS_HPP
