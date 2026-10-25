#ifndef PHOSPHOR_PIXEL_H
#define PHOSPHOR_PIXEL_H

#include "common/constants.h"
#include "common/typedefs.h"

typedef struct GPU_ALIGN Pixel {
    float4 flux;
    // 1 * 16 = 16

    f32 radius_sq;
    f32 photon_count;
    // 2 * 4 = 8

    // total: 24
    u8 _padding[8];
} Pixel;

#endif // PHOSPHOR_PIXEL_H
