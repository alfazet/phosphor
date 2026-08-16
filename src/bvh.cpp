#include "bounding_box.h"
#include "bvh_node.h"
#include "constants.h"
#include "host_triangle.hpp"

#include <algorithm>
#include <vector>

void split(std::vector<HostTriangle> &triangles, u32 start, u32 end, std::vector<BvhNode> &tree, u32 p = 1);

// https://stackoverflow.com/questions/364985/algorithm-for-finding-the-smallest-power-of-two-thats-greater-or-equal-to-a-giv
/// Round up to next higher power of 2 (return x if it's already a power
/// of 2).
inline int pow2roundup(int x) {
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

bool box_compare(HostTriangle &a, HostTriangle &b, u32 index) {
    return get_bounding_box(a).bbox_min.s[index] < get_bounding_box(b).bbox_min.s[index];
}

bool box_compare0(HostTriangle &a, HostTriangle &b) { return box_compare(a, b, 0); }

bool box_compare1(HostTriangle &a, HostTriangle &b) { return box_compare(a, b, 1); }

bool box_compare2(HostTriangle &a, HostTriangle &b) { return box_compare(a, b, 2); }

std::vector<BvhNode> create_tree(std::vector<HostTriangle> &triangles) {
    std::vector<BvhNode> res;
    res.emplace_back(BvhNode()); // 0 intentionally left blank so binary tree aligns well
    res.resize(2 * pow2roundup(triangles.size()) + 1);

    split(triangles, 0, triangles.size(), res);
    return res;
}

void split(std::vector<HostTriangle> &triangles, u32 start, u32 end, std::vector<BvhNode> &tree, u32 p) {
    u32 span = end - start;
    if (span == 1) {
        BvhNode node;
        node.triangle_index = start; // i sure hope we dont have 2,147,483,647 triangles
        node.bbox = get_bounding_box(triangles[start]);
        tree.at(p) = node;
        return;
    }
    BvhNode node;
    BoundingBox bbox = get_bounding_box(triangles[start]);
    for (u32 i = start + 1; i < end; i++) {
        bbox = merge(bbox, get_bounding_box(triangles[i]));
    }
    node.bbox = bbox;
    node.triangle_index = -1;
    tree.at(p) = node;

    u32 axis = longest_axis(bbox);

    auto comparator = (axis == 0) ? box_compare0 : (axis == 1) ? box_compare1 : box_compare2;

    std::sort(std::begin(triangles) + start, std::begin(triangles) + end, comparator);
    auto mid = start + (end - start) / 2;

    split(triangles, start, mid, tree, 2 * p);
    split(triangles, mid, end, tree, 2 * p + 1);
}