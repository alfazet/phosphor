#include "bvh.hpp"
#include "bounding_box.h"
#include "bvh_node.h"
#include "triangle.h"
#include "utils.h"

#include <algorithm>
#include <numeric>
#include <vector>

void split(const std::vector<Triangle> &triangles, std::vector<u32> &indices, u32 start, u32 end,
           std::vector<BvhNode> &tree, u32 p) {
    u32 span = end - start;
    if (span == 1) {
        BvhNode node;
        node.triangle_index = static_cast<i32>(indices[start]);
        node.bbox = get_bounding_box(triangles[indices[start]]);
        tree.at(p) = node;
        return;
    }
    BvhNode node;
    BoundingBox bbox = get_bounding_box(triangles[indices[start]]);
    for (u32 i = start + 1; i < end; i++) {
        bbox = merge(bbox, get_bounding_box(triangles[indices[i]]));
    }
    node.bbox = bbox;
    node.triangle_index = -1;
    tree.at(p) = node;

    u32 axis = longest_axis(bbox);

    auto cmp = [&](u32 i, u32 j) {
        return get_bounding_box(triangles[i]).bbox_min.s[axis] < get_bounding_box(triangles[j]).bbox_min.s[axis];
    };

    std::sort(indices.begin() + start, indices.begin() + end, cmp);
    auto mid = start + (end - start) / 2;

    split(triangles, indices, start, mid, tree, 2 * p);
    split(triangles, indices, mid, end, tree, 2 * p + 1);
}

Bvh::Bvh(const std::vector<Triangle> &triangles) {
    std::vector<BvhNode> nodes;
    nodes.emplace_back(BvhNode()); // 0 intentionally left blank so binary tree aligns well
    nodes.resize(2 * round_up_to_pow2(triangles.size()) + 1);

    std::vector<u32> indices(triangles.size());
    std::iota(indices.begin(), indices.end(), 0);
    split(triangles, indices, 0, triangles.size(), nodes, 1);

    this->nodes = nodes;
}

const BoundingBox &Bvh::get_bbox() const { return this->nodes[1].bbox; }
