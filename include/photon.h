#ifndef PHOSPHOR_PHOTON_H
#define PHOSPHOR_PHOTON_H

#include "constants.h"
#include "typedefs.h"

// this struct should be unused (we're using SoA), but keep it for documentation
typedef struct GPU_ALIGN Photon {
    float4 pos; // .xyz - position, .w - axis for kd-tree (use as_float/as_uint)
    u32 power; // RGBE
    u32 dir; // incoming direction normalized, (octahedral encoding)
    // 4 * 4 + 4 + 4 = 24

    // total: 24
    // u8 _padding[8];
} Photon;

#endif // PHOSPHOR_PHOTON_H
