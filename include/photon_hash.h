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

inline u32 photon_hash(const float4 pos, const PhotonHashInfo info) {
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

#ifdef __OPENCL_C_VERSION__

inline void try_insert_photon(u32 pidx, f32 d2, u32 grid_res, u32 *result, f32 *dist2, u32 *count) {
    if (*count < grid_res) {
        result[*count] = pidx;
        dist2[*count] = d2;
        (*count)++;
        return;
    }

    u32 worst = 0;
    for (u32 i = 1; i < grid_res; i++)
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
                              __global const float4 *photon_pos, float4 pos, u32 grid_res, f32 max_dist2, u32 *result,
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
        f32 d2 = dot(diff.xyz, diff.xyz);

        if (d2 < max_dist2)
            try_insert_photon(pidx - 1, d2, grid_res, result, dist2, count);

        u32 axis = as_uint(ph.w);
        f32 delta = get_axis(pos, axis) - get_axis(ph, axis);
        f32 delta_sq = delta * delta;

        u32 near = (delta < 0.0f) ? index * 2 : index * 2 + 1;
        u32 far = (delta < 0.0f) ? index * 2 + 1 : index * 2;

        f32 current_max = max_dist2;
        if (*count >= grid_res) {
            current_max = dist2[0];
            for (u32 i = 1; i < grid_res; i++)
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
                               __global const float4 *photon_pos, __global const float4 *photon_power, u32 samples,
                               f32 max_dist2, float4 *flux, f32 *out_max_dist2) {
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

        u32 h = photon_hash(pos2, info);
        if (h == 0)
            continue;

        u32 size = bucket_tree_size[h];
        if (size == 0)
            continue;

        locate_bucket_knn(tree_index, bucket_tree_offset[h], size, photon_pos, pos, samples, max_dist2, result, dist2,
                          &count);
    }

    f32 worst = 0.0f;
    for (u32 i = 0; i < count; i++) {
        flux[0] += photon_power[result[i]];
        worst = fmax(worst, dist2[i]);
    }
    *out_max_dist2 = worst;
}
#endif // __OPENCL_C_VERSION__

#ifndef __OPENCL_C_VERSION__
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>

typedef struct PhotonHash {
    std::vector<u32> cell_start;
    std::vector<u32> cell_end;
    std::vector<u32> tree_index;
    std::vector<u32> bucket_tree_offset;
    std::vector<u32> bucket_tree_size;
    u32 bucket_count;
} PhotonHash;

inline PhotonHashInfo build_hash_info(const BoundingBox &bbox, u32 grid_res) {
    PhotonHashInfo info;
    info.grid_res = grid_res;
    info.origin = (float4){bbox.bbox_min.x, bbox.bbox_min.y, bbox.bbox_min.z, 0.0f};
    info.cell_sizes.x = glm::max((bbox.bbox_max.x - bbox.bbox_min.x) / grid_res, MIN_CELL_SIZE);
    info.cell_sizes.y = glm::max((bbox.bbox_max.y - bbox.bbox_min.y) / grid_res, MIN_CELL_SIZE);
    info.cell_sizes.z = glm::max((bbox.bbox_max.z - bbox.bbox_min.z) / grid_res, MIN_CELL_SIZE);
    return info;
}

inline u32 choose_axis(const std::vector<float4> &photon_pos, const std::vector<u32> &indices, u32 start, u32 end) {
    float4 minp = photon_pos[indices[start]];
    float4 maxp = photon_pos[indices[start]];
    for (u32 i = start + 1; i < end; i++) {
        float4 p = photon_pos[indices[i]];
        minp.x = min_f32(minp.x, p.x);
        minp.y = min_f32(minp.y, p.y);
        minp.z = min_f32(minp.z, p.z);
        maxp.x = max_f32(maxp.x, p.x);
        maxp.y = max_f32(maxp.y, p.y);
        maxp.z = max_f32(maxp.z, p.z);
    }
    f32 ex = maxp.x - minp.x, ey = maxp.y - minp.y, ez = maxp.z - minp.z;
    if (ex > ey && ex > ez)
        return 0;
    if (ey > ez)
        return 1;
    return 2;
}

inline void balance(std::vector<float4> &photon_pos, std::vector<u32> &indices, u32 index, u32 start, u32 end,
                    std::vector<u32> &tree_index, u32 offset, u32 tree_size) {
    if (start >= end || index >= tree_size)
        return;

    u32 axis = choose_axis(photon_pos, indices, start, end);
    u32 median = (start + end) / 2;

    std::nth_element(indices.begin() + start, indices.begin() + median, indices.begin() + end,
                     [&photon_pos, axis](u32 a, u32 b) { return photon_pos[a].s[axis] < photon_pos[b].s[axis]; });

    photon_pos[indices[median]].w = as_float(axis);
    tree_index[offset + index] = indices[median] + 1; // 0 left blank

    balance(photon_pos, indices, index * 2, start, median, tree_index, offset, tree_size);
    balance(photon_pos, indices, index * 2 + 1, median + 1, end, tree_index, offset, tree_size);
}

inline PhotonHash build_hash(std::vector<float4> &photon_pos, std::vector<float4> &photon_power,
                             std::vector<float4> &photon_dir, std::vector<float4> &photon_normal, PhotonHashInfo info) {
    u32 n_photons = photon_pos.size();
    PhotonHash grid;
    grid.bucket_count = info.grid_res * info.grid_res * info.grid_res + 1;

    std::vector<u32> hashes(n_photons);
    std::unordered_map<u32, u32> hashes_count;
    hashes_count.reserve(n_photons);

    for (u32 i = 0; i < n_photons; i++) {
        u32 hash = photon_hash(photon_pos[i], info);
        hashes[i] = hash;
        hashes_count[hash]++;
    }

    // https://stackoverflow.com/questions/37368787/c-sort-one-vector-based-on-another-one
    std::vector<u32> indices(n_photons);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&hashes](u32 a, u32 b) { return hashes[a] < hashes[b]; });

    std::vector<float4> sorted_pos(n_photons);
    std::vector<float4> sorted_power(n_photons);
    std::vector<float4> sorted_dir(n_photons);
    std::vector<float4> sorted_normal(n_photons);
    for (u32 i = 0; i < n_photons; i++) {
        u32 src = indices[i];
        sorted_pos[i] = photon_pos[src];
        sorted_power[i] = photon_power[src];
        sorted_dir[i] = photon_dir[src];
        sorted_normal[i] = photon_normal[src];
    }
    photon_pos = std::move(sorted_pos);
    photon_power = std::move(sorted_power);
    photon_dir = std::move(sorted_dir);
    photon_normal = std::move(sorted_normal);

    grid.cell_start.assign(grid.bucket_count, 0);
    grid.cell_end.assign(grid.bucket_count, 0);

    u32 accum = 0;
    for (u32 i = 0; i < grid.bucket_count; i++) {
        grid.cell_start[i] = accum;
        auto it = hashes_count.find(i);
        u32 count = (it != hashes_count.end()) ? it->second : 0;
        grid.cell_end[i] = accum + count;
        accum += count;
    }

    grid.bucket_tree_offset.assign(grid.bucket_count, 0);
    grid.bucket_tree_size.assign(grid.bucket_count, 0);

    u32 tree_total = 0;
    for (u32 b = 0; b < grid.bucket_count; b++) {
        u32 count = grid.cell_end[b] - grid.cell_start[b];
        u32 size = count == 0 ? 0 : round_up_to_pow2(count + 1);
        grid.bucket_tree_size[b] = size;
        grid.bucket_tree_offset[b] = tree_total;
        tree_total += size;
    }

    std::vector<u32> kd_indices(n_photons);
    std::iota(kd_indices.begin(), kd_indices.end(), 0);
    grid.tree_index.assign(tree_total, 0);
    for (u32 b = 0; b < grid.bucket_count; b++) {
        u32 count = grid.cell_end[b] - grid.cell_start[b];
        if (count == 0)
            continue;
        balance(photon_pos, kd_indices, 1, grid.cell_start[b],
                grid.cell_end[b], grid.tree_index, grid.bucket_tree_offset[b], grid.bucket_tree_size[b]);
    }

    return grid;
}
#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_PHOTON_HASH_H
