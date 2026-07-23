#include "image.hpp"
#include "logger.hpp"
#include "stb_image_write.h"

void Image::set_pixel(u32 x, u32 y, const vec3 &color) {
    const usize idx = (y * width_ + x) * 3;
    pixels_[idx + 0] = to_byte(color.r);
    pixels_[idx + 1] = to_byte(color.g);
    pixels_[idx + 2] = to_byte(color.b);
}

void Image::write_png(const char *path) const {
    constexpr i32 channels = 3;
    const i32 stride = static_cast<i32>(width_) * channels;

    if (!stbi_write_png(path, static_cast<i32>(width_), static_cast<i32>(height_), channels, pixels_.data(), stride))
        LOG_ERROR("failed to write PNG");
}
