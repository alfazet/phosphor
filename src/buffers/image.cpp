#include "buffers/image.hpp"

static cl::Buffer alloc_and_memset(ClContext &ctx, u8 data, u32 count, u32 item_size) {
    cl::Buffer buf(ctx.context, CL_MEM_READ_WRITE, count * item_size);
    ctx.queue.enqueueFillBuffer(buf, data, 0, count * item_size);
    ctx.queue.finish();

    return buf;
}

void ImageBuffers::alloc(ClContext &ctx) {
    this->d_hp_pos = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
    this->d_hp_normal = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
    this->d_hp_throughput = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
    this->d_hp_base_color = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
    this->d_hp_direct = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
    this->d_hp_emission = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
    this->d_flux = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
    this->d_photon_count = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(f32));
    this->d_total_radiance = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
}
