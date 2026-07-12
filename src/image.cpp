#include "image.hpp"
#include "stb_image_write.h"

void Image::write_png(const char *path) const {
    constexpr i32 channels = 3;
    const i32 stride = static_cast<i32>(width_) * channels;

    if (!stbi_write_png(path, static_cast<i32>(width_), static_cast<i32>(height_), channels, pixels_.data(), stride)) {
        throw std::runtime_error("failed to write PNG");
    }
}
