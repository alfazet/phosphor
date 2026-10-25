#include "buffers/image.hpp"
#include "common/hit_point.h"
#include "common/pixel.h"

static cl::Buffer alloc_and_memset(ClContext &ctx, u8 data, u32 count, u32 item_size) {
    cl::Buffer buf(ctx.context, CL_MEM_READ_WRITE, count * item_size);
    ctx.queue.enqueueFillBuffer(buf, data, 0, count * item_size);
    ctx.queue.finish();

    return buf;
}

void ImageBuffers::alloc(ClContext &ctx) {
    this->d_hit_points = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(HitPoint));
    this->d_pixels = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(Pixel));
    this->d_total_radiance = alloc_and_memset(ctx, 0, this->n_pixels, sizeof(float4));
}
