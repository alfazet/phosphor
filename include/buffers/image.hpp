#ifndef PHOSPHOR_IMAGE_BUFFERS_HPP
#define PHOSPHOR_IMAGE_BUFFERS_HPP

#include "common/camera.h"
#include "common/typedefs.h"
#include "host/opencl_ctx.hpp"

struct ImageBuffers {
    cl::Buffer d_hp_pos;
    cl::Buffer d_hp_normal;
    cl::Buffer d_hp_throughput;
    cl::Buffer d_hp_base_color;
    cl::Buffer d_hp_direct;
    cl::Buffer d_hp_emission;
    cl::Buffer d_flux; // .w contains the radius of the sphere in which the photons were located
    cl::Buffer d_photon_count;
    cl::Buffer d_total_radiance;

    CameraParams camera{};
    u32 image_width = 0;
    u32 image_height = 0;
    u32 n_pixels = 0;

    void alloc(ClContext &ctx);
};

#endif // PHOSPHOR_IMAGE_BUFFERS_HPP
