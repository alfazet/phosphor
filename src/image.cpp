#include "image.hpp"
#include "logger.hpp"
#include "stb_image_write.h"

static u8 to_byte(f32 c) {
    // tone mapping
    c = c / (c + 1);
    // gamma correction
    c = glm::pow(c, 1.0f / 2.2f);

    return static_cast<u8>(c * 255.0f + 0.5f);
}

Image::Image(u32 width, u32 height) : width(width), height(height), pixels(width * height * 3) {}

void Image::set_pixel(u32 x, u32 y, const vec3 &color) {
    const usize idx = (y * this->width + x) * 3;
    this->pixels[idx + 0] = to_byte(color.r);
    this->pixels[idx + 1] = to_byte(color.g);
    this->pixels[idx + 2] = to_byte(color.b);
}

void Image::write_png(const char *path) const {
    constexpr i32 channels = 3;
    const i32 stride = static_cast<i32>(width) * channels;

    if (!stbi_write_png(path, static_cast<i32>(width), static_cast<i32>(height), channels, pixels.data(), stride))
        LOG_ERROR("failed to write PNG");
}
