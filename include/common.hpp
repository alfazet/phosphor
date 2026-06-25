#ifndef PHOSPHOR_COMMON_HPP
#define PHOSPHOR_COMMON_HPP

#include <cstdint>
#include <cstdio>
#include <stdexcept>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/scalar_constants.hpp"
#include "glm/glm.hpp"

using glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4, glm::quat;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef std::size_t usize;

typedef float f32;
typedef double f64;

constexpr f32 EPS = 1e-9;
constexpr f32 INF = 1e9;

#endif // PHOSPHOR_COMMON_HPP
