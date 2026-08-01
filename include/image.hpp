#ifndef PHOSPHOR_IMAGE_HPP
#define PHOSPHOR_IMAGE_HPP

#include "common.hpp"

#include <vector>

struct Image {
    u32 width;
    u32 height;
    std::vector<u8> pixels;

    Image(u32 width, u32 height);

    void set_pixel(u32 x, u32 y, const vec3 &color);
    void write_png(const char *path) const;
};

#endif // PHOSPHOR_IMAGE_HPP
