#ifndef PHOSPHOR_PHOTON_H
#define PHOSPHOR_PHOTON_H

#include "constants.h"
#include "typedefs.h"

// this struct should be unused (we're using SoA), but keep it for documentation
typedef struct GPU_ALIGN Photon {
    float4 pos; // .xyz - position, .w - axis for kd-tree (use as_float/as_uint)
    float4 power;
    float4 dir;    // incoming direction normalized
    float4 normal; // normal of the surface hit
    // 4 * 4 * 4 + 1 * 4 = 68

    // total: 64
    // u8 _padding[0];
} Photon;

#endif // PHOSPHOR_PHOTON_H
