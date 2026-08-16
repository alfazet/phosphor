#ifndef PHOSPHOR_GPU_BVH_H
#define PHOSPHOR_GPU_BVH_H

#include "bvh_node.h"
#include "host_triangle.hpp"
#include <vector>

std::vector<BvhNode> create_tree(std::vector<HostTriangle> &triangles);

#endif // PHOSPHOR_GPU_BVH_H
