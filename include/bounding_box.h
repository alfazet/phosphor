#ifndef PHOSPHOR_GPU_BOUNDING_BOX_H
#define PHOSPHOR_GPU_BOUNDING_BOX_H

#include "constants.h"
#include "typedefs.h"

typedef struct BoundingBox {
    float4 bbox_min;
    float4 bbox_max;
} BoundingBox;

#ifndef __OPENCL_C_VERSION__
#include "host_triangle.hpp"
inline f32 min_f32(f32 a, f32 b) { return a < b ? a : b; }
inline f32 max_f32(f32 a, f32 b) { return a > b ? a : b; }

inline void expand_aabb(float4 bbox_min, float4 bbox_max, const float4 p) {
    bbox_min.x = min_f32(bbox_min.x, p.x);
    bbox_min.y = min_f32(bbox_min.y, p.y);
    bbox_min.z = min_f32(bbox_min.z, p.z);

    bbox_max.x = max_f32(bbox_max.x, p.x);
    bbox_max.y = max_f32(bbox_max.y, p.y);
    bbox_max.z = max_f32(bbox_max.z, p.z);
}

inline BoundingBox get_bounding_box(const HostTriangle &tri) {
    float4 bbox_min{{INF, INF, INF, 0.0f}};
    float4 bbox_max{{-INF, -INF, -INF, 0.0f}};

    expand_aabb(bbox_min, bbox_max, tri.v0);
    expand_aabb(bbox_min, bbox_max, tri.v1);
    expand_aabb(bbox_min, bbox_max, tri.v2);

    return {bbox_min, bbox_max};
}

inline u32 longest_axis(const BoundingBox bbox) {
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

    result.bbox_min.x = min_f32(a.bbox_min.x, b.bbox_min.x);
    result.bbox_min.y = min_f32(a.bbox_min.y, b.bbox_min.y);
    result.bbox_min.z = min_f32(a.bbox_min.z, b.bbox_min.z);

    result.bbox_max.x = max_f32(a.bbox_max.x, b.bbox_max.x);
    result.bbox_max.y = max_f32(a.bbox_max.y, b.bbox_max.y);
    result.bbox_max.z = max_f32(a.bbox_max.z, b.bbox_max.z);

    return result;
}
#endif // __OPENCL_C_VERSION__
#endif // PHOSPHOR_GPU_BOUNDING_BOX_H
