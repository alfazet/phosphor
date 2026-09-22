#ifndef PHOSPHOR_PHOTON_HASH_H
#define PHOSPHOR_PHOTON_HASH_H

#include "bounding_box.h"
#include "constants.h"
#include "photon.h"
#include "typedefs.h"
#include "utils.h"

// https://courses.csail.mit.edu/18.337/2012/projects/sherry_wu_paper.pdf
typedef struct GPU_ALIGN PhotonHashInfo {
    float4 origin;
    float4 cell_sizes;
    u32 grid_res;
    // 2 * 4 * 4 + 4 = 36

    // total: 36
    u8 _padding[12];
} PhotonHashInfo;

inline u32 get_hash(const float4 pos, const PhotonHashInfo info) {
    u32 grid_res = info.grid_res;
    f32 fx = (pos.x - info.origin.x) / info.cell_sizes.x;
    f32 fy = (pos.y - info.origin.y) / info.cell_sizes.y;
    f32 fz = (pos.z - info.origin.z) / info.cell_sizes.z;

    if (fx < 0.0f || fy < 0.0f || fz < 0.0f || fx >= (f32)grid_res || fy >= (f32)grid_res || fz >= (f32)grid_res) {
        return 0;
    }

    u32 x = (u32)fx;
    u32 y = (u32)fy;
    u32 z = (u32)fz;

    // return k * k * (u32)(x + k) + k * (u32)(y + k) + (u32)(z + k); // remember to change bucket_count if reverted
    return grid_res * grid_res * x + grid_res * y + z + 1;
    // +1 leaves as 0 as special, empty value
}

#ifndef __OPENCL_C_VERSION__

inline PhotonHashInfo build_photon_hash_info(const BoundingBox &bbox, u32 grid_res) {
    PhotonHashInfo info;
    info.grid_res = grid_res;
    info.origin = (float4){bbox.bbox_min.x, bbox.bbox_min.y, bbox.bbox_min.z, 0.0f};
    info.cell_sizes.x = glm::max((bbox.bbox_max.x - bbox.bbox_min.x) / grid_res, MIN_CELL_SIZE);
    info.cell_sizes.y = glm::max((bbox.bbox_max.y - bbox.bbox_min.y) / grid_res, MIN_CELL_SIZE);
    info.cell_sizes.z = glm::max((bbox.bbox_max.z - bbox.bbox_min.z) / grid_res, MIN_CELL_SIZE);

    return info;
}

#endif // __OPENCL_C_VERSION__

#ifdef __OPENCL_C_VERSION__

inline void gather_photon_flux(const float4 pos, const PhotonHashInfo info, __global const u32 *tree_index,
                               __global const u32 *bucket_tree_offset, __global const u32 *bucket_tree_size,
                               __global const float4 *photon_pos, __global const u32 *photon_power,
                               __global const u32 *photon_dir, f32 max_dist2, const float4 surf_hit_normal,
                               float4 *flux, f32 *out_max_dist2, f32 *out_found) {
    f32 worst = 0.0f;
    f32 found = 0.0f;

    for (u32 i = 0; i < 27; i++) {
        i32 dx = (i32)(i % 3) - 1;
        i32 dy = (i32)((i / 3) % 3) - 1;
        i32 dz = (i32)((i / 9) % 3) - 1;

        float4 pos2 = pos;
        pos2.x += (f32)dx * info.cell_sizes.x;
        pos2.y += (f32)dy * info.cell_sizes.y;
        pos2.z += (f32)dz * info.cell_sizes.z;

        u32 h = get_hash(pos2, info);
        if (h == 0)
            continue;

        u32 offset = bucket_tree_offset[h];
        u32 size = bucket_tree_size[h];
        if (size == 0)
            continue;

        for (u32 j = 0; j < size; j++) {
            u32 pidx = tree_index[offset + j];
            if (pidx == 0)
                continue;
            pidx -= 1;

            float4 diff = pos - photon_pos[pidx];
            f32 d2 = dot(diff, diff);
            if (d2 >= max_dist2)
                continue;

            float4 p_dir = decode_oct(photon_dir[pidx]);
            if (dot(p_dir, surf_hit_normal) > 0.0f)
                continue;

            worst = fmax(worst, d2);

            // gaussian filter, see constants definition for reference
            f32 w = GAUSS_ALPHA * (1.0f - (1.0f - exp(-GAUSS_BETA * d2 / (2.0f * max_dist2))) / (1 - exp(-GAUSS_BETA)));
            *flux += w * decode_rgbe(photon_power[pidx]);
            found += w;
        }
    }

    *out_max_dist2 = worst;
    *out_found = found;
}
#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_PHOTON_HASH_H
