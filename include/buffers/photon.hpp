#ifndef PHOSPHOR_PHOTON_BUFFERS_HPP
#define PHOSPHOR_PHOTON_BUFFERS_HPP

#include "host/opencl_ctx.hpp"
#include "common/typedefs.h"

struct PhotonBuffers {
    cl::Buffer d_photon_pos;
    cl::Buffer d_photon_power; // RGBE encoded
    cl::Buffer d_photon_dir; // octahedral encoded
    cl::Buffer d_photon_hashes;

    cl::Buffer d_cell_counts;
    cl::Buffer d_cell_start;
    cl::Buffer d_cell_end;

    u32 n_photons = 0;

    void alloc(ClContext &ctx, u32 max_photons, u32 bucket_count);
};

#endif // PHOSPHOR_PHOTON_BUFFERS_HPP
