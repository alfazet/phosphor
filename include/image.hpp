#ifndef PHOSPHOR_IMAGE_HPP
#define PHOSPHOR_IMAGE_HPP

#include "common.hpp"
#include <algorithm>
#include <vector>

class Image {
  public:
    Image(u32 width, u32 height) : width_(width), height_(height), pixels_(width * height * 3) {}

    void set_pixel(u32 x, u32 y, const vec3 &color) {
        const usize idx = (y * width_ + x) * 3;
        pixels_[idx + 0] = to_byte(color.r);
        pixels_[idx + 1] = to_byte(color.g);
        pixels_[idx + 2] = to_byte(color.b);
    }

    void write_png(const char *path) const;

    u32 width() const { return width_; }
    u32 height() const { return height_; }

  private:
    static u8 to_byte(f32 c) {
        c = std::clamp(c, 0.0f, 1.0f);
        c = glm::pow(c, 1.0f / 2.2f);

        return static_cast<u8>(c * 255.0f + 0.5f);
    }

    u32 width_, height_;
    std::vector<u8> pixels_;
};

#endif // PHOSPHOR_IMAGE_HPP
