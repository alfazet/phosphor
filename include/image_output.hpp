#ifndef PHOSPHOR_IMAGE_OUTPUT_HPP
#define PHOSPHOR_IMAGE_OUTPUT_HPP

#include "typedefs.h"

#include <string>
#include <vector>

void write_png(const std::string &path, u32 width, u32 height, u32 iters, const std::vector<float4> &raw);

#endif // PHOSPHOR_IMAGE_OUTPUT_HPP
