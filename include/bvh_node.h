#ifndef PHOSPHOR_BVH_NODE_H
#define PHOSPHOR_BVH_NODE_H

#include "bounding_box.h"
#include "typedefs.h"

typedef struct BvhNode {
    BoundingBox bbox;
    // 2 * 4 * 4 = 32

    i32 triangle_index;
    // 4

    // total: 36
    u8 _padding[12];
} BvhNode __attribute__((aligned(16)));

#endif // PHOSPHOR_BVH_NODE_H
