#include "buffers/photon.hpp"

void PhotonBuffers::alloc(ClContext &ctx, u32 max_photons, u32 bucket_count) {
    d_photon_pos = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, max_photons * sizeof(float4));
    d_photon_power = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, max_photons * sizeof(u32));
    d_photon_dir = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, max_photons * sizeof(u32));
    d_photon_hashes = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, max_photons * sizeof(u32));
    d_cell_counts = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, bucket_count * sizeof(u32));
    d_cell_start = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, bucket_count * sizeof(u32));
    d_cell_end = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, bucket_count * sizeof(u32));
}
