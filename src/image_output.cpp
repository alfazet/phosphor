#include "image_output.hpp"
#include "constants.h"
#include "logger.hpp"
#include "stb_image_write.h"

#include <cmath>

inline f32 tone_map(f32 x) {
    f32 mapped = x / (1.0f + x);
    f32 out = std::pow(mapped, 1.0f / 2.2f);

    return std::clamp(out, 0.0f, 1.0f);
}

void write_png(const std::filesystem::path &path, u32 width, u32 height, const std::vector<SppmPixel> &sppm_pixels,
               const std::vector<float4> &total_irradiance, u64 total_photons, u32 sppm_rounds) {
    u32 n_pixels = width * height;
    std::vector<u8> ldr(n_pixels * 3);

    f32 inv_photons = (total_photons > 0) ? (1.0f / static_cast<f32>(total_photons)) : 0.0f;
    f32 inv_rounds = (sppm_rounds > 0) ? (1.0f / static_cast<f32>(sppm_rounds)) : 0.0f;

    for (u32 p = 0; p < n_pixels; p++) {
        const SppmPixel &sp = sppm_pixels[p];
        f32 r_sq = sp.radius_sq;
        float4 indirect = {0.0f, 0.0f, 0.0f, 0.0f};
        if (r_sq > 0.0f) {
            f32 inv_area = inv_photons / (PI * r_sq);
            indirect.x = sp.flux.x * inv_area;
            indirect.y = sp.flux.y * inv_area;
            indirect.z = sp.flux.z * inv_area;
        }

        const float4 &x = total_irradiance[p];
        f32 r = x.x * inv_rounds + indirect.x;
        f32 g = x.y * inv_rounds + indirect.y;
        f32 b = x.z * inv_rounds + indirect.z;
        ldr[p * 3 + 0] = static_cast<u8>(tone_map(r) * 255.0f);
        ldr[p * 3 + 1] = static_cast<u8>(tone_map(g) * 255.0f);
        ldr[p * 3 + 2] = static_cast<u8>(tone_map(b) * 255.0f);
    }

    stbi_write_png(path.c_str(), static_cast<i32>(width), static_cast<i32>(height), 3, ldr.data(),
                   static_cast<i32>(width) * 3);
}
