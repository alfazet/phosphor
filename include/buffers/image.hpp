#ifndef PHOSPHOR_IMAGE_BUFFERS_HPP
#define PHOSPHOR_IMAGE_BUFFERS_HPP

#include "common/camera.h"
#include "common/typedefs.h"
#include "host/opencl_ctx.hpp"

struct ImageBuffers {
    cl::Buffer d_hit_points;
    cl::Buffer d_pixels;
    cl::Buffer d_total_radiance;

    CameraParams camera{};
    u32 image_width = 0;
    u32 image_height = 0;
    u32 n_pixels = 0;

    void alloc(ClContext &ctx);
};

#endif // PHOSPHOR_IMAGE_BUFFERS_HPP
