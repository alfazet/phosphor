#include "image_output.hpp"
#include "logger.hpp"
#include "stb_image_write.h"

#include <cmath>
#include <vector>

inline f32 tone_map(f32 x) {
    f32 mapped = x / (1.0f + x);
    return std::pow(mapped, 1.0f / 2.2f);
}

void write_png(const std::string &path, u32 width, u32 height, u32 iters, const std::vector<float4> &raw) {
    u32 n_pixels = width * height;
    std::vector<u8> ldr(n_pixels * 3);

    for (u32 p = 0; p < n_pixels; p++) {
        f32 sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
        for (u32 j = 0; j < iters; j++) {
            const float4 &c = raw[p * iters + j];
            sum_r += c.x;
            sum_g += c.y;
            sum_b += c.z;
        }
        sum_r /= iters;
        sum_g /= iters;
        sum_b /= iters;

        ldr[p * 3 + 0] = static_cast<u8>(tone_map(sum_r) * 255.0f);
        ldr[p * 3 + 1] = static_cast<u8>(tone_map(sum_g) * 255.0f);
        ldr[p * 3 + 2] = static_cast<u8>(tone_map(sum_b) * 255.0f);
    }

    stbi_write_png(path.c_str(), static_cast<i32>(width), static_cast<i32>(height), 3, ldr.data(),
                   static_cast<i32>(width) * 3);
    LOG_INFO("rendered image written to {}", path);
}
