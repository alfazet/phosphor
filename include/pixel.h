#ifndef PHOSPHOR_PIXEL_H
#define PHOSPHOR_PIXEL_H

#include "constants.h"
#include "typedefs.h"

typedef struct GPU_ALIGN Pixel {
    float4 color; // .xyz = non-clamped color, .w = sample count
} Pixel;

#endif // PHOSPHOR_PIXEL_H
