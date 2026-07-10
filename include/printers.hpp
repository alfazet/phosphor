#ifndef PHOSPHOR_PRINTERS_HPP
#define PHOSPHOR_PRINTERS_HPP

#include "common.hpp"
#include "camera.hpp"

inline void print_camera(const Camera &camera) {
    printf("Camera {\n");
    printf("  position : (%f, %f, %f)\n", camera.position_.x, camera.position_.y, camera.position_.z);
    printf("  target   : (%f, %f, %f)\n", camera.target_.x,   camera.target_.y,   camera.target_.z);
    printf("  up       : (%f, %f, %f)\n", camera.up_.x,       camera.up_.y,       camera.up_.z);
    printf("  hfov     : %f deg\n", camera.hfov_);
    printf("  aspect   : %f\n", camera.aspect_ratio_);
    printf("  u        : (%f, %f, %f)\n", camera.u_.x, camera.u_.y, camera.u_.z);
    printf("  v        : (%f, %f, %f)\n", camera.v_.x, camera.v_.y, camera.v_.z);
    printf("  w        : (%f, %f, %f)\n", camera.w_.x, camera.w_.y, camera.w_.z);
    printf("  lower_left_corner : (%f, %f, %f)\n", camera.lower_left_corner_.x, camera.lower_left_corner_.y, camera.lower_left_corner_.z);
    printf("  horizontal        : (%f, %f, %f)\n", camera.horizontal_.x, camera.horizontal_.y, camera.horizontal_.z);
    printf("  vertical          : (%f, %f, %f)\n", camera.vertical_.x, camera.vertical_.y, camera.vertical_.z);
    printf("}\n");
}
#endif // PHOSPHOR_PRINTERS_HPP
