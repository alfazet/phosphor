#ifndef PHOSPHOR_BVH_NODE_H
#define PHOSPHOR_BVH_NODE_H

#include "typedefs.h"

typedef struct BvhNode {
    float4 bbox_min;
    float4 bbox_max;
    // 2 * 4 * 4 = 32

    u32 left;
    u32 right;
    u32 tri_start; // index into bvh_tri_indices
    u32 tri_count;
    // 4 * 4 = 16

    // total: 48
} BvhNode __attribute__((aligned(16)));

#endif // PHOSPHOR_BVH_NODE_H
