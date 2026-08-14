#ifndef PHOSPHOR_PIXEL_H
#define PHOSPHOR_PIXEL_H

#include "typedefs.h"

typedef struct Pixel {
    float4 color; // .xyz = non-clamped color, .w = sample count
} Pixel __attribute__((aligned(16)));

#endif // PHOSPHOR_PIXEL_H
