#ifndef PHOSPHOR_BVH_NODE_H
#define PHOSPHOR_BVH_NODE_H

#include "bounding_box.h"
#include "typedefs.h"

typedef struct GPU_ALIGN BvhNode {
    BoundingBox bbox;
    // 2 * 4 * 4 = 32

    i32 triangle_index;
    // 4

    u32 right_child;
    // 4

    // total: 40
    u8 _padding[8];
} BvhNode;

#endif // PHOSPHOR_BVH_NODE_H
