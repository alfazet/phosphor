#ifndef PHOSPHOR_BVH_HPP
#define PHOSPHOR_BVH_HPP

#include "common/bvh_node.h"
#include "common/triangle.h"

#include <vector>

struct Bvh {
    std::vector<BvhNode> nodes;

    Bvh(const std::vector<Triangle> &triangles);

    const BoundingBox &get_bbox() const;
};

#endif // PHOSPHOR_BVH_HPP
