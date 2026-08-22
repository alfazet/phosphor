#include "bounding_box.h"
#include "bvh_node.h"
#include "constants.h"
#include "helpers.h"
#include "triangle.h"

#include <algorithm>
#include <vector>

void split(std::vector<Triangle> &triangles, u32 start, u32 end, std::vector<BvhNode> &tree, u32 p);

bool box_compare(Triangle &a, Triangle &b, u32 index) {
    return get_bounding_box(a).bbox_min.s[index] < get_bounding_box(b).bbox_min.s[index];
}

bool box_compare0(Triangle &a, Triangle &b) { return box_compare(a, b, 0); }

bool box_compare1(Triangle &a, Triangle &b) { return box_compare(a, b, 1); }

bool box_compare2(Triangle &a, Triangle &b) { return box_compare(a, b, 2); }

std::vector<BvhNode> create_tree(std::vector<Triangle> &triangles) {
    std::vector<BvhNode> res;
    res.emplace_back(BvhNode()); // 0 intentionally left blank so binary tree aligns well
    res.resize(2 * pow2roundup(triangles.size()) + 1);

    split(triangles, 0, triangles.size(), res, 1);
    return res;
}

void split(std::vector<Triangle> &triangles, u32 start, u32 end, std::vector<BvhNode> &tree, u32 p) {
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
