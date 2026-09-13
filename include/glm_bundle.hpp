#ifndef PHOSPHOR_GLM_BUNDLE_HPP
#define PHOSPHOR_GLM_BUNDLE_HPP

#include "typedefs.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

using glm::mat3, glm::mat4, glm::vec2, glm::vec3, glm::vec4;

inline float4 vec3_to_float4(const vec3 &v) { return float4{{v.x, v.y, v.z, 0.0f}}; }
inline vec3 float4_to_vec3(const float4 &v) { return {v.x, v.y, v.z}; }
inline float2 vec2_to_float2(const vec2 &v) { return float2{{v.x, v.y}}; }

#endif // PHOSPHOR_GLM_BUNDLE_HPP
