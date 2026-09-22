#ifndef PHOSPHOR_IMAGE_OUTPUT_HPP
#define PHOSPHOR_IMAGE_OUTPUT_HPP

#include "sppm_pixel.h"
#include "typedefs.h"

#include <vector>
#include <filesystem>

void write_png(const std::filesystem::path &path, u32 width, u32 height, const std::vector<SppmPixel> &sppm_pixels,
               const std::vector<float4> &total_irradiance, u64 total_photons, u32 sppm_rounds);

#endif // PHOSPHOR_IMAGE_OUTPUT_HPP
