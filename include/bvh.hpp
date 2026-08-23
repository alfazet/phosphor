#ifndef PHOSPHOR_GPU_BVH_H
#define PHOSPHOR_GPU_BVH_H

#include "bvh_node.h"
#include "triangle.h"

#include <vector>

struct Bvh {
    std::vector<BvhNode> nodes;

    Bvh(const std::vector<Triangle> &triangles);

    const BoundingBox& get_bbox() const;
};

#endif // PHOSPHOR_GPU_BVH_H
