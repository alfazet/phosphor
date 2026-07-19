#ifndef PHOSPHOR_PRINTERS_HPP
#define PHOSPHOR_PRINTERS_HPP

#include "camera.hpp"
#include "common.hpp"
#include "scene.hpp"

#include <format>

template <glm::length_t L, typename T, glm::qualifier Q> struct std::formatter<glm::vec<L, T, Q>> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const glm::vec<L, T, Q> &v, std::format_context &ctx) const {
        auto out = ctx.out();
        *out++ = '(';
        for (glm::length_t i = 0; i < L; ++i) {
            if (i > 0) {
                *out++ = ',';
                *out++ = ' ';
            }
            out = std::format_to(out, "{}", v[i]);
        }
        *out++ = ')';
        return out;
    }
};

inline void print_camera(const Camera &camera) {
    LOG_INFO("camera {{");
    LOG_INFO("  position : {}", camera.position_);
    LOG_INFO("  target   : {}", camera.target_);
    LOG_INFO("  up       : {}", camera.up_);
    LOG_INFO("  hfov     : {} deg", camera.hfov_);
    LOG_INFO("  aspect   : {}", camera.aspect_ratio_);
    LOG_INFO("  u        : {}", camera.u_);
    LOG_INFO("  v        : {}", camera.v_);
    LOG_INFO("  w        : {}", camera.w_);
    LOG_INFO("  lower_left_corner : {}", camera.lower_left_corner_);
    LOG_INFO("  horizontal        : {}", camera.horizontal_);
    LOG_INFO("  vertical          : {}", camera.vertical_);
    LOG_INFO("}}");
}

inline void print_spanning_box(const Scene &scene) {
    vec3 minp(INF);
    vec3 maxp(-INF);

    for (const auto &tri : scene.triangles_) {
        minp = glm::min(minp, tri.v0_);
        minp = glm::min(minp, tri.v1_);
        minp = glm::min(minp, tri.v2_);

        maxp = glm::max(maxp, tri.v0_);
        maxp = glm::max(maxp, tri.v1_);
        maxp = glm::max(maxp, tri.v2_);
    }

    LOG_INFO("scene bounding box {{");
    LOG_INFO("  min : {}", minp);
    LOG_INFO("  max : {}", maxp);
    LOG_INFO("}}");
}

#endif // PHOSPHOR_PRINTERS_HPP
