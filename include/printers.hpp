#ifndef PHOSPHOR_PRINTERS_HPP
#define PHOSPHOR_PRINTERS_HPP

#include "camera.hpp"
#include "common.hpp"
#include "scene.hpp"

inline void print_camera(const Camera &camera) {
    printf("Camera {\n");
    printf("  position : (%f, %f, %f)\n", camera.position_.x, camera.position_.y, camera.position_.z);
    printf("  target   : (%f, %f, %f)\n", camera.target_.x, camera.target_.y, camera.target_.z);
    printf("  up       : (%f, %f, %f)\n", camera.up_.x, camera.up_.y, camera.up_.z);
    printf("  hfov     : %f deg\n", camera.hfov_);
    printf("  aspect   : %f\n", camera.aspect_ratio_);
    printf("  u        : (%f, %f, %f)\n", camera.u_.x, camera.u_.y, camera.u_.z);
    printf("  v        : (%f, %f, %f)\n", camera.v_.x, camera.v_.y, camera.v_.z);
    printf("  w        : (%f, %f, %f)\n", camera.w_.x, camera.w_.y, camera.w_.z);
    printf("  lower_left_corner : (%f, %f, %f)\n", camera.lower_left_corner_.x, camera.lower_left_corner_.y,
           camera.lower_left_corner_.z);
    printf("  horizontal        : (%f, %f, %f)\n", camera.horizontal_.x, camera.horizontal_.y, camera.horizontal_.z);
    printf("  vertical          : (%f, %f, %f)\n", camera.vertical_.x, camera.vertical_.y, camera.vertical_.z);
    printf("}\n");
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

    printf("scene bounding box {\n");
    printf("  min      : (%f, %f, %f)\n", minp.x, minp.y, minp.z);
    printf("  max      : (%f, %f, %f)\n", maxp.x, maxp.y, maxp.z);
    printf("}\n");
}
#endif // PHOSPHOR_PRINTERS_HPP
