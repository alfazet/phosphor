#ifndef PHOSPHOR_COMMON_HPP
#define PHOSPHOR_COMMON_HPP

#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/glm.hpp"

using glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4, glm::quat;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef bool i1;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef std::size_t usize;

typedef float f32;
typedef double f64;

constexpr f32 EPS = 1e-9;
constexpr f32 INF = 1e9;
constexpr f32 PI = glm::pi<f32>();
constexpr vec3 ZERO_VEC = vec3(0.0f);
constexpr vec3 BLACK = vec3(0.0f);
constexpr vec3 WHITE = vec3(1.0f);
constexpr f32 AIR_IOR = 1.0f;
constexpr f32 DEFAULT_IOR = 1.5f;
constexpr f32 DEFAULT_TRANSMISSION = 0.0f;
constexpr u32 MAX_PHOTON_BOUNCES = 50;
constexpr u32 MIN_RAY_BOUNCES = 4;
constexpr u32 MAX_RAY_BOUNCES = 50;

#endif // PHOSPHOR_COMMON_HPP
