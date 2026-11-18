#ifndef PHOSPHOR_TEST_UTILS
#define PHOSPHOR_TEST_UTILS

#include "common/bounding_box.h"
#include "common/triangle.h"
#include "common/typedefs.h"

namespace ts {

inline float4 f4(f32 x, f32 y, f32 z, f32 w = 0.0f) {
    float4 v{};
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

inline BoundingBox make_bbox(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1) {
    BoundingBox b{};
    b.min = f4(x0, y0, z0);
    b.max = f4(x1, y1, z1);
    return b;
}

inline Triangle make_triangle(const float4 &a, const float4 &b, const float4 &c) {
    Triangle t{};
    t.v0 = a;
    t.v1 = b;
    t.v2 = c;
    return t;
}

inline bool same_box(const BoundingBox &a, const BoundingBox &b) {
    return a.min.x == b.min.x && a.min.y == b.min.y && a.min.z == b.min.z && a.max.x == b.max.x && a.max.y == b.max.y &&
           a.max.z == b.max.z;
}

} // namespace ts

#endif // PHOSPHOR_TEST_UTILS
