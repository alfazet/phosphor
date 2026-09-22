#ifndef PHOSPHOR_SPPM_PIXEL_H
#define PHOSPHOR_SPPM_PIXEL_H

#include "constants.h"
#include "typedefs.h"

typedef struct GPU_ALIGN SppmPixel {
    float4 flux;
    // 1 * 16 = 16

    f32 radius_sq;
    f32 photon_count;
    // 2 * 4 = 8

    // total: 24
    u8 _padding[8];
} SppmPixel;

#endif // PHOSPHOR_SPPM_PIXEL_H
