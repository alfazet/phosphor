#ifndef PHOSPHOR_PHOTON_H
#define PHOSPHOR_PHOTON_H

#include "constants.h"
#include "typedefs.h"

typedef struct GPU_ALIGN Photon {
    float4 pos;
    float4 power;
    float4 dir;    // incoming direction normalized
    float4 normal; // normal of the surface hit
    u32 axis;      // for kd-tree
    // 4 * 4 * 4 + 1 * 4 = 68

    // total: 68
    u8 _padding[12];
} Photon;

#endif // PHOSPHOR_PHOTON_H
