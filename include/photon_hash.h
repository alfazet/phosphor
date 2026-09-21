#ifndef PHOSPHOR_PHOTON_HASH_H
#define PHOSPHOR_PHOTON_HASH_H

#include "bounding_box.h"
#include "constants.h"
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

inline void try_insert_photon(u32 pidx, f32 d2, u32 samples, u32 *result, f32 *dist2, u32 *count) {
    if (*count < samples) {
        result[*count] = pidx;
        dist2[*count] = d2;
        (*count)++;
        return;
    }

    u32 worst = 0;
    for (u32 i = 1; i < samples; i++)
        if (dist2[i] > dist2[worst])
            worst = i;

    if (d2 < dist2[worst]) {
        result[worst] = pidx;
        dist2[worst] = d2;
    }
}

inline f32 get_axis(float4 v, u32 axis) {
    if (axis == 0)
        return v.x;
    if (axis == 1)
        return v.y;
    return v.z;
}

inline void locate_bucket_knn(__global const u32 *tree_index, u32 offset, u32 tree_size,
                              __global const float4 *photon_pos, float4 pos, u32 samples, f32 max_dist2, u32 *result,
                              f32 *dist2, u32 *count) {
    u32 stack[KD_STACK_SIZE];
    i32 sp = 0;
    stack[0] = 1;

    while (sp >= 0) {
        u32 index = stack[sp--];
        if (index == 0 || index >= tree_size)
            continue;

        u32 pidx = tree_index[offset + index];
        if (pidx == 0)
            continue;

        float4 ph = photon_pos[pidx - 1];
        float4 diff = pos - ph;
        f32 d2 = dot(diff, diff);

        if (d2 < max_dist2)
            try_insert_photon(pidx - 1, d2, samples, result, dist2, count);

        u32 axis = as_uint(ph.w);
        f32 delta = get_axis(pos, axis) - get_axis(ph, axis);
        f32 delta_sq = delta * delta;

        u32 near = (delta < 0.0f) ? index * 2 : index * 2 + 1;
        u32 far = (delta < 0.0f) ? index * 2 + 1 : index * 2;

        f32 current_max = max_dist2;
        if (*count >= samples) {
            current_max = dist2[0];
            for (u32 i = 1; i < samples; i++)
                current_max = fmax(current_max, dist2[i]);
        }

        if (delta_sq < current_max && far < tree_size && sp < KD_STACK_SIZE - 1)
            stack[++sp] = far;
        if (near < tree_size && sp < KD_STACK_SIZE - 1)
            stack[++sp] = near;
    }
}

inline void gather_photon_flux(const float4 pos, const PhotonHashInfo info, __global const u32 *tree_index,
                               __global const u32 *bucket_tree_offset, __global const u32 *bucket_tree_size,
                               __global const float4 *photon_pos, __global const u32 *photon_power,
                               __global const float4 *photon_dir, u32 samples, f32 max_dist2,
                               const float4 surf_hit_normal, float4 *flux, f32 *out_max_dist2, f32 *out_count) {
    u32 k = min(samples, (u32)MAX_PHOTON_SAMPLES);
    u32 result[MAX_PHOTON_SAMPLES];
    f32 dist2[MAX_PHOTON_SAMPLES];
    u32 count = 0;

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

        u32 size = bucket_tree_size[h];
        if (size == 0)
            continue;

        locate_bucket_knn(tree_index, bucket_tree_offset[h], size, photon_pos, pos, k, max_dist2, result, dist2,
                          &count);
    }

    f32 worst = 0.0f;
    for (u32 i = 0; i < count; i++) {
        const float4 p_dir = photon_dir[result[i]];

        if (dot(p_dir, surf_hit_normal) > 0.0f)
            continue;

        *flux += decodeRGBE(photon_power[result[i]]);
        worst = fmax(worst, dist2[i]);
    }

    *out_max_dist2 = worst;
    *out_count = count;
}
#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_PHOTON_HASH_H
