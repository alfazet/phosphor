#include "image_output.hpp"
#include "constants.h"
#include "logger.hpp"
#include "stb_image_write.h"

#include <cmath>
//
// inline f32 tone_map(f32 x, f32 exposure) {
//     x *= exposure;
//     f32 mapped = x / (1.0f + x);
//     f32 out = std::pow(mapped, 1.0f / 2.2f);
//
//     return std::clamp(out, 0.0f, 1.0f);
// }

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve
inline f32 tone_map(f32 x, f32 exposure) {
    x *= exposure;
    f32 a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    f32 out = (x * (a * x + b)) / (x * (c * x + d) + e);

    return std::clamp(out, 0.0f, 1.0f);
}

RenderSize resolve_render_size(const ArgsList &args, f32 base_aspect) {
    ASSERT(base_aspect > 0.0f, "base aspect must be positive");

    const bool has_w = args.was_provided("-w");
    const bool has_h = args.was_provided("-h");

    u32 width = args.width;
    u32 height = args.height;

    if ((has_w && has_h) || (!has_w && !has_h)) {
        f32 aspect = static_cast<f32>(width) / static_cast<f32>(height);
        f32 rel_diff = std::abs(aspect - base_aspect) / base_aspect;
        if (rel_diff > 0.1f)
            LOG_WARN("output aspect {:.3f} ({}x{}) differs from the camera's aspect {:.3f}", aspect, width, height,
                     base_aspect);
        return {
            args.width,
            args.height,
            aspect,
        };
    }

    if (has_w) {
        height = std::max(1u, static_cast<u32>(std::lround(width / base_aspect)));
        return {width, height, base_aspect};
    }

    width = std::max(1u, static_cast<u32>(std::lround(height * base_aspect)));
    return {width, height, base_aspect};
}

void write_png(const std::filesystem::path &path, u32 width, u32 height, f32 exposure,
               const std::vector<SppmPixel> &sppm_pixels, const std::vector<float4> &total_irradiance,
               u64 total_photons, u32 sppm_rounds, const std::string &metadata) {
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
        ldr[p * 3 + 0] = static_cast<u8>(tone_map(r, exposure) * 255.0f);
        ldr[p * 3 + 1] = static_cast<u8>(tone_map(g, exposure) * 255.0f);
        ldr[p * 3 + 2] = static_cast<u8>(tone_map(b, exposure) * 255.0f);
    }

    stbi_write_png(path.c_str(), static_cast<i32>(width), static_cast<i32>(height), 3, ldr.data(),
                   static_cast<i32>(width) * 3);

    std::ostringstream cmd;
    cmd << "exiftool -q -overwrite_original "
        << "-Comment=\"" << metadata << "\" "
        << "\'" << path.string() << "\'";

    u32 ret = std::system(cmd.str().c_str());
    if (ret != 0)
        LOG_ERROR("exiftool failed to write metadata for {} (exit code {})", path.c_str(), ret);
}
