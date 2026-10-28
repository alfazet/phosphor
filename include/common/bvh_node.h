#ifndef PHOSPHOR_BVH_NODE_H
#define PHOSPHOR_BVH_NODE_H

#include "common/bounding_box.h"

typedef struct GPU_ALIGN BvhNode {
    BoundingBox bbox; // bbox.min.w = triangle_index, bbox.max.w = right_child
} BvhNode;

#endif // PHOSPHOR_BVH_NODE_H
