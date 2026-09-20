#ifndef PHOSPHOR_PHOTON_H
#define PHOSPHOR_PHOTON_H

#include "constants.h"
#include "typedefs.h"

// this struct should be unused (we're using SoA), but keep it for documentation
typedef struct GPU_ALIGN Photon {
    float4 pos; // .xyz - position, .w - axis for kd-tree (use as_float/as_uint)
    float4 power;
    float4 dir; // incoming direction normalized
    // 4 * 4 * 3 = 48

    // total: 48
    // u8 _padding[0];
} Photon;

#endif // PHOSPHOR_PHOTON_H
