#ifndef PHOSPHOR_BOUNDING_BOX_H
#define PHOSPHOR_BOUNDING_BOX_H

#include "common/constants.h"
#include "common/typedefs.h"

typedef struct BoundingBox {
    float4 bbox_min;
    float4 bbox_max;
} BoundingBox;

#ifndef __OPENCL_C_VERSION__

#include "common/triangle.h"
#include "host/logger.hpp"

static inline bool check_inf(f32 value) { return (value >= INF || value <= -INF); }

static inline void check_bbox_inf(const BoundingBox bbox) {
    if (check_inf(bbox.bbox_min.x) || check_inf(bbox.bbox_min.y) || check_inf(bbox.bbox_min.z) ||
        check_inf(bbox.bbox_max.x) || check_inf(bbox.bbox_max.y) || check_inf(bbox.bbox_max.z))
        LOG_FATAL("bounding box contains an infinity");
}

static inline void check_bbox_width(const BoundingBox bbox) {
    if (bbox.bbox_min.x == bbox.bbox_max.x || bbox.bbox_min.y == bbox.bbox_max.y || bbox.bbox_min.z == bbox.bbox_max.z)
        LOG_FATAL("object has no width");
}

static inline void expand(BoundingBox &bbox) {
    bbox.bbox_min.x -= BBOX_EPS;
    bbox.bbox_min.y -= BBOX_EPS;
    bbox.bbox_min.z -= BBOX_EPS;
    bbox.bbox_max.x += BBOX_EPS;
    bbox.bbox_max.y += BBOX_EPS;
    bbox.bbox_max.z += BBOX_EPS;
}

static inline void expand_aabb(BoundingBox &bbox, const float4 p) {
    bbox.bbox_min.x = std::min(bbox.bbox_min.x, p.x);
    bbox.bbox_min.y = std::min(bbox.bbox_min.y, p.y);
    bbox.bbox_min.z = std::min(bbox.bbox_min.z, p.z);
    bbox.bbox_max.x = std::max(bbox.bbox_max.x, p.x);
    bbox.bbox_max.y = std::max(bbox.bbox_max.y, p.y);
    bbox.bbox_max.z = std::max(bbox.bbox_max.z, p.z);
}

inline BoundingBox get_bounding_box(const Triangle &tri) {
    float4 bbox_min = {{INF, INF, INF, 0.0f}};
    float4 bbox_max = {{-INF, -INF, -INF, 0.0f}};
    BoundingBox res = {bbox_min, bbox_max};

    expand_aabb(res, tri.v0);
    expand_aabb(res, tri.v1);
    expand_aabb(res, tri.v2);
    check_bbox_inf(res);
    expand(res);
    check_bbox_width(res);

    return res;
}

inline u32 get_longest_axis(const BoundingBox bbox) {
    f32 x_size = bbox.bbox_max.x - bbox.bbox_min.x;
    f32 y_size = bbox.bbox_max.y - bbox.bbox_min.y;
    f32 z_size = bbox.bbox_max.z - bbox.bbox_min.z;
    if (x_size > y_size) {
        if (x_size > z_size) {
            return 0;
        }
    } else {
        if (y_size > z_size) {
            return 1;
        }
    }

    return 2;
}

inline BoundingBox merge(const BoundingBox &a, const BoundingBox &b) {
    BoundingBox result;
    result.bbox_min.x = std::min(a.bbox_min.x, b.bbox_min.x);
    result.bbox_min.y = std::min(a.bbox_min.y, b.bbox_min.y);
    result.bbox_min.z = std::min(a.bbox_min.z, b.bbox_min.z);
    result.bbox_max.x = std::max(a.bbox_max.x, b.bbox_max.x);
    result.bbox_max.y = std::max(a.bbox_max.y, b.bbox_max.y);
    result.bbox_max.z = std::max(a.bbox_max.z, b.bbox_max.z);

    return result;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_BOUNDING_BOX_H
