#include "bvh.hpp"
#include "bounding_box.h"
#include "bvh_node.h"
#include "triangle.h"

#include <algorithm>
#include <numeric>
#include <vector>

struct TmpNode {
    BoundingBox bbox;
    i32 triangle_index;
    u32 left_child;
    u32 right_child;
};

static u32 build_tmp(const std::vector<Triangle> &triangles, std::vector<u32> &indices, u32 start, u32 end,
                      std::vector<TmpNode> &tmp) {
    u32 idx = static_cast<u32>(tmp.size());
    tmp.push_back({});

    // leaf node
    if (end == start + 1) {
        TmpNode &node = tmp[idx];
        node.triangle_index = static_cast<i32>(indices[start]);
        node.bbox = get_bounding_box(triangles[indices[start]]);
        node.left_child = 0;
        node.right_child = 0;
        return idx;
    }

    BoundingBox bbox = get_bounding_box(triangles[indices[start]]);
    for (u32 i = start + 1; i < end; i++) {
        bbox = merge(bbox, get_bounding_box(triangles[indices[i]]));
    }

    TmpNode &node = tmp[idx];
    node.bbox = bbox;
    node.triangle_index = -1;

    u32 axis = longest_axis(bbox);
    auto cmp = [&](u32 i, u32 j) {
        return get_bounding_box(triangles[i]).bbox_min.s[axis] < get_bounding_box(triangles[j]).bbox_min.s[axis];
    };
    std::sort(indices.begin() + start, indices.begin() + end, cmp);
    u32 mid = start + (end - start) / 2;

    u32 left_idx = build_tmp(triangles, indices, start, mid, tmp);
    u32 right_idx = build_tmp(triangles, indices, mid, end, tmp);
    tmp[idx].left_child = left_idx;
    tmp[idx].right_child = right_idx;

    return idx;
}

// left child is always placed right after the parent
// right child is placed somewhere after the parent and is pointed to by a "skip pointer"
static void flatten(const std::vector<TmpNode> &tmp, u32 tmp_idx, std::vector<BvhNode> &bvh) {
    u32 idx = static_cast<u32>(bvh.size());
    bvh.push_back({});
    bvh[idx].bbox = tmp[tmp_idx].bbox;
    bvh[idx].triangle_index = tmp[tmp_idx].triangle_index;
    bvh[idx].right_child = 0;

    // non-leaf node
    if (tmp[tmp_idx].triangle_index == -1) {
        flatten(tmp, tmp[tmp_idx].left_child, bvh);
        bvh[idx].right_child = static_cast<u32>(bvh.size());
        flatten(tmp, tmp[tmp_idx].right_child, bvh);
    }
}

Bvh::Bvh(const std::vector<Triangle> &triangles) {
    std::vector<u32> indices(triangles.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::vector<TmpNode> tmp;
    tmp.reserve(2 * triangles.size());
    build_tmp(triangles, indices, 0, triangles.size(), tmp);

    this->nodes.reserve(2 * triangles.size() - 1);
    flatten(tmp, 0, this->nodes);
}

const BoundingBox &Bvh::get_bbox() const { return this->nodes[0].bbox; }
