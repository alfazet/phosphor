#ifndef PHOSPHOR_IMAGE_OUTPUT_HPP
#define PHOSPHOR_IMAGE_OUTPUT_HPP

#include "cmd_args.hpp"
#include "sppm_pixel.h"
#include "typedefs.h"

#include <filesystem>
#include <vector>

struct RenderSize {
    u32 width;
    u32 height;
    f32 aspect;
};

RenderSize resolve_render_size(const ArgsList &args, f32 base_aspect);

void write_png(const std::filesystem::path &path, u32 width, u32 height, f32 exposure,
               const std::vector<SppmPixel> &sppm_pixels, const std::vector<float4> &total_irradiance,
               u64 total_photons, u32 sppm_rounds, const std::string &metadata);

#endif // PHOSPHOR_IMAGE_OUTPUT_HPP
