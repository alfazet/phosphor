#ifndef PHOSPHOR_BOUNDING_BOX_H
#define PHOSPHOR_BOUNDING_BOX_H

#include "common/constants.h"
#include "common/typedefs.h"

typedef struct GPU_ALIGN BoundingBox {
    float4 min;
    float4 max;
} BoundingBox;

#ifndef __OPENCL_C_VERSION__

#include "common/triangle.h"
#include "host/logger.hpp"

static inline bool check_inf(f32 value) { return (value >= INF || value <= -INF); }

static inline void check_bbox_inf(const BoundingBox bbox) {
    if (check_inf(bbox.min.x) || check_inf(bbox.min.y) || check_inf(bbox.min.z) ||
        check_inf(bbox.max.x) || check_inf(bbox.max.y) || check_inf(bbox.max.z))
        LOG_FATAL("bounding box contains an infinity");
}

static inline void check_bbox_width(const BoundingBox bbox) {
    if (bbox.min.x == bbox.max.x || bbox.min.y == bbox.max.y || bbox.min.z == bbox.max.z)
        LOG_FATAL("object has no width");
}

static inline void expand(BoundingBox &bbox) {
    bbox.min.x -= BBOX_EPS;
    bbox.min.y -= BBOX_EPS;
    bbox.min.z -= BBOX_EPS;
    bbox.max.x += BBOX_EPS;
    bbox.max.y += BBOX_EPS;
    bbox.max.z += BBOX_EPS;
}

static inline void expand_aabb(BoundingBox &bbox, const float4 p) {
    bbox.min.x = std::min(bbox.min.x, p.x);
    bbox.min.y = std::min(bbox.min.y, p.y);
    bbox.min.z = std::min(bbox.min.z, p.z);
    bbox.max.x = std::max(bbox.max.x, p.x);
    bbox.max.y = std::max(bbox.max.y, p.y);
    bbox.max.z = std::max(bbox.max.z, p.z);
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
    f32 x_size = bbox.max.x - bbox.min.x;
    f32 y_size = bbox.max.y - bbox.min.y;
    f32 z_size = bbox.max.z - bbox.min.z;
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
    result.min.x = std::min(a.min.x, b.min.x);
    result.min.y = std::min(a.min.y, b.min.y);
    result.min.z = std::min(a.min.z, b.min.z);
    result.max.x = std::max(a.max.x, b.max.x);
    result.max.y = std::max(a.max.y, b.max.y);
    result.max.z = std::max(a.max.z, b.max.z);

    return result;
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_BOUNDING_BOX_H
