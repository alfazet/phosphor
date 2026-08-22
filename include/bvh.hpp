#ifndef PHOSPHOR_GPU_BVH_H
#define PHOSPHOR_GPU_BVH_H

#include "bvh_node.h"
#include "triangle.h"

#include <vector>

std::vector<BvhNode> create_tree(std::vector<Triangle> &triangles);

#endif // PHOSPHOR_GPU_BVH_H
