#ifndef PHOSPHOR_PHOTON_HASH_H
#define PHOSPHOR_PHOTON_HASH_H

#include "common/bounding_box.h"
#include "common/constants.h"
#include "common/typedefs.h"
#include "common/utils.h"
#include "device/encoding.h"

// https://courses.csail.mit.edu/18.337/2012/projects/sherry_wu_paper.pdf
typedef struct GPU_ALIGN PhotonHashData {
    float4 origin;
    float4 cell_sizes;
    u32 grid_res;
    // 2 * 4 * 4 + 4 = 36

    // total: 36
    u8 _padding[12];
} PhotonHashData;

inline u32 get_hash(const float4 pos, const PhotonHashData data) {
    u32 grid_res = data.grid_res;
    f32 fx = (pos.x - data.origin.x) / data.cell_sizes.x;
    f32 fy = (pos.y - data.origin.y) / data.cell_sizes.y;
    f32 fz = (pos.z - data.origin.z) / data.cell_sizes.z;

    if (fx < 0.0f || fy < 0.0f || fz < 0.0f || fx >= (f32)grid_res || fy >= (f32)grid_res || fz >= (f32)grid_res) {
        return 0;
    }

    u32 x = (u32)fx;
    u32 y = (u32)fy;
    u32 z = (u32)fz;

    // +1 leaves as 0 as special, empty value
    return grid_res * grid_res * x + grid_res * y + z + 1;
}

#ifndef __OPENCL_C_VERSION__

inline PhotonHashData build_photon_hash_info(const BoundingBox &bbox, u32 grid_res) {
    PhotonHashData data{};
    data.grid_res = grid_res;
    data.origin = (float4){bbox.min.x, bbox.min.y, bbox.min.z, 0.0f};
    data.cell_sizes.x = glm::max((bbox.max.x - bbox.min.x) / grid_res, MIN_CELL_SIZE);
    data.cell_sizes.y = glm::max((bbox.max.y - bbox.min.y) / grid_res, MIN_CELL_SIZE);
    data.cell_sizes.z = glm::max((bbox.max.z - bbox.min.z) / grid_res, MIN_CELL_SIZE);

    return data;
}

#endif // __OPENCL_C_VERSION__

#ifdef __OPENCL_C_VERSION__

inline void gather_photon_flux(const float4 pos, const PhotonHashData data, __global const u32 *cell_start,
                               __global const u32 *cell_end, __global const float4 *photon_pos,
                               __global const u32 *photon_power, __global const u32 *photon_dir, f32 search_r_sq,
                               const float4 surf_hit_normal, float4 *flux, f32 *out_max_dist_sq, f32 *out_found) {
    f32 max_dist_sq = 0.0f;
    f32 found = 0.0f;

    for (u32 i = 0; i < 27; i++) {
        i32 cell_x = (i32)(i % 3) - 1;
        i32 cell_y = (i32)((i / 3) % 3) - 1;
        i32 cell_z = (i32)((i / 9) % 3) - 1;

        float4 pos2 = pos;
        pos2.x += (f32)cell_x * data.cell_sizes.x;
        pos2.y += (f32)cell_y * data.cell_sizes.y;
        pos2.z += (f32)cell_z * data.cell_sizes.z;

        u32 h = get_hash(pos2, data);
        if (h == 0)
            continue;

        u32 start = cell_start[h];
        u32 end = cell_end[h];
        if (start == end)
            continue;

        for (u32 p_idx = start; p_idx < end; p_idx++) {
            float4 d = pos - photon_pos[p_idx];
            f32 d_sq = dot(d, d);
            if (d_sq >= search_r_sq)
                continue;

            float4 p_dir = decode_oct(photon_dir[p_idx]);
            if (dot(p_dir, surf_hit_normal) > 0.0f)
                continue;

            max_dist_sq = fmax(max_dist_sq, d_sq);
            // gaussian filter, see constants definition for reference
            f32 w = GAUSS_ALPHA * (1.0f - (1.0f - native_exp(-GAUSS_BETA * d_sq / (2.0f * max_dist_sq))) / GAUSS_DENOM);
            *flux += w * decode_rgbe(photon_power[p_idx]);
            found += w;
        }
    }

    *out_max_dist_sq = max_dist_sq;
    *out_found = found;
}
#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_PHOTON_HASH_H
