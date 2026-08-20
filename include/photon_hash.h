#ifndef PHOSPHOR_PHOTON_HASH_H
#define PHOSPHOR_PHOTON_HASH_H

#include "bounding_box.h"
#include "photon.h"
#include "typedefs.h"

// https://courses.csail.mit.edu/18.337/2012/projects/sherry_wu_paper.pdf
typedef struct PhotonHashInfo {
    float4 origin;
    float4 cell_sizes;
    u32 k;
    // 2 * 4 * 4 + 4 = 36

    // total: 36
    u8 _padding[12];
} PhotonHashInfo __attribute__((aligned(16)));

inline u32 photon_hash(const float4 pos, const PhotonHashInfo info) {
    u32 k = info.k;
    f32 fx = (pos.x - info.origin.x) / info.cell_sizes.x;
    f32 fy = (pos.y - info.origin.y) / info.cell_sizes.y;
    f32 fz = (pos.z - info.origin.z) / info.cell_sizes.z;

    if (fx < 0.0f || fy < 0.0f || fz < 0.0f || fx >= (f32)k || fy >= (f32)k || fz >= (f32)k) {
        return 0;
    }

    u32 x = (u32)fx;
    u32 y = (u32)fy;
    u32 z = (u32)fz;

    // return k * k * (u32)(x + k) + k * (u32)(y + k) + (u32)(z + k); // remember to change bucket_count if reverted
    return k * k * (u32)(x) + k * (u32)(y) + (u32)(z + 1);
    // +1 leaves as 0 as special, empty value
}

#ifdef __OPENCL_C_VERSION__

inline void try_insert_photon(u32 pidx, f32 d2, u32 k, u32 *result, f32 *dist2, u32 *count) {
    if (*count < k) {
        result[*count] = pidx;
        dist2[*count] = d2;
        (*count)++;
        return;
    }

    u32 worst = 0;
    for (u32 i = 1; i < k; i++)
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

inline void locate_bucket_knn(__global const u32 *tree_index, u32 offset, u32 tree_size, __global const Photon *photons,
                              float4 pos, u32 k, f32 max_dist2, u32 *result, f32 *dist2, u32 *count) {
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

        Photon ph = photons[pidx - 1];
        float4 diff = pos - ph.pos;
        f32 d2 = dot(diff.xyz, diff.xyz);

        if (d2 < max_dist2)
            try_insert_photon(pidx - 1, d2, k, result, dist2, count);

        u32 axis = ph.axis;
        f32 delta = get_axis(pos, axis) - get_axis(ph.pos, axis);
        f32 delta_sq = delta * delta;

        u32 near = (delta < 0.0f) ? index * 2 : index * 2 + 1;
        u32 far = (delta < 0.0f) ? index * 2 + 1 : index * 2;

        f32 current_max = max_dist2;
        if (*count >= k) {
            current_max = dist2[0];
            for (u32 i = 1; i < k; i++)
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
                               __global const Photon *photons, f32 max_dist2, float4 *flux, f32 *out_max_dist2) {
    u32 result[PHOTON_K];
    f32 dist2[PHOTON_K];
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

        locate_bucket_knn(tree_index, bucket_tree_offset[h], size, photons, pos, PHOTON_K, max_dist2, result, dist2,
                          &count);
    }

    f32 worst = 0.0f;
    for (u32 i = 0; i < count; i++) {
        flux[0] += photons[result[i]].power;
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

inline PhotonHashInfo build_hash_info(BoundingBox bbox, u32 k) {
    PhotonHashInfo info;
    info.k = k;
    info.origin = (float4){bbox.bbox_min.x, bbox.bbox_min.y, bbox.bbox_min.z, 0.0f};
    info.cell_sizes.x = (bbox.bbox_max.x - bbox.bbox_min.x) / k;
    info.cell_sizes.y = (bbox.bbox_max.y - bbox.bbox_min.y) / k;
    info.cell_sizes.z = (bbox.bbox_max.z - bbox.bbox_min.z) / k;
    return info;
}

inline u32 choose_axis(std::vector<Photon> &photons, usize start, usize end) {
    float4 minp = photons[start].pos;
    float4 maxp = photons[start].pos;
    for (usize i = start + 1; i < end; i++) {
        minp.x = min_f32(minp.x, photons[i].pos.x);
        minp.y = min_f32(minp.y, photons[i].pos.y);
        minp.z = min_f32(minp.z, photons[i].pos.z);
        maxp.x = max_f32(maxp.x, photons[i].pos.x);
        maxp.y = max_f32(maxp.y, photons[i].pos.y);
        maxp.z = max_f32(maxp.z, photons[i].pos.z);
    }
    f32 ex = maxp.x - minp.x, ey = maxp.y - minp.y, ez = maxp.z - minp.z;
    if (ex > ey && ex > ez)
        return 0;
    if (ey > ez)
        return 1;
    return 2;
}

inline void balance(std::vector<Photon> &photons, usize index, usize start, usize end, std::vector<u32> &tree_index,
                    u32 offset, u32 tree_size) {
    if (start >= end || index >= tree_size)
        return;

    u32 axis = choose_axis(photons, start, end);
    usize median = (start + end) / 2;

    std::nth_element(photons.begin() + start, photons.begin() + median, photons.begin() + end,
                     [axis](const Photon &a, const Photon &b) { return a.pos.s[axis] < b.pos.s[axis]; });

    photons[median].axis = axis;
    tree_index[offset + index] = static_cast<u32>(median) + 1; // 0 left blank

    balance(photons, index * 2, start, median, tree_index, offset, tree_size);
    balance(photons, index * 2 + 1, median + 1, end, tree_index, offset, tree_size);
}

inline PhotonHash build_hash(std::vector<Photon> &photons, PhotonHashInfo info) {
    u32 n_photons = photons.size();
    PhotonHash grid;
    grid.bucket_count = (info.k * (info.k * (info.k + 1) + 1)) + 1; // Horner for efficiency

    std::vector<u32> hashes(n_photons);
    std::unordered_map<u32, u32> hashes_count;
    hashes_count.reserve(n_photons);

    for (u32 i = 0; i < n_photons; i++) {
        u32 hash = photon_hash(photons[i].pos, info);
        hashes[i] = hash;
        hashes_count[hash]++;
    }

    // https://stackoverflow.com/questions/37368787/c-sort-one-vector-based-on-another-one
    std::vector<int> indices(photons.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&hashes](u32 a, u32 b) { return hashes[a] < hashes[b]; });

    std::vector<Photon> sorted_photons(n_photons);
    for (u32 i = 0; i < n_photons; i++) {
        sorted_photons[i] = photons[indices[i]];
    }
    photons = std::move(sorted_photons);

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
        u32 size = count == 0 ? 0 : pow2roundup(count + 1);
        grid.bucket_tree_size[b] = size;
        grid.bucket_tree_offset[b] = tree_total;
        tree_total += size;
    }

    grid.tree_index.assign(tree_total, 0);
    for (u32 b = 0; b < grid.bucket_count; b++) {
        u32 count = grid.cell_end[b] - grid.cell_start[b];
        if (count == 0)
            continue;
        balance(photons, 1, grid.cell_start[b], grid.cell_end[b], grid.tree_index, grid.bucket_tree_offset[b],
                grid.bucket_tree_size[b]);
    }

    return grid;
}
#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_PHOTON_HASH_H
