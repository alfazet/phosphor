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
inline u32 get_photon_nei(const float4 pos, const PhotonHashInfo info, __global const u32 *cell_start,
                          __global const u32 *cell_end, u32 starts[27], u32 ends[27]) {
    u32 p = 0;
    for (u32 i = 0; i < 27; i++) {
        i32 dx = (i32)(i % 3) - 1;
        i32 dy = (i32)((i / 3) % 3) - 1;
        i32 dz = (i32)((i / 9) % 3) - 1;

        float4 pos2 = pos;
        pos2.x += (f32)dx * info.cell_sizes.x;
        pos2.y += (f32)dy * info.cell_sizes.y;
        pos2.z += (f32)dz * info.cell_sizes.z;

        u32 h = photon_hash(pos2, info);
        if (h != 0) {
            starts[p] = cell_start[h];
            ends[p] = cell_end[h];
            p++;
        }
    }
    return p;
}
#endif // __OPENCL_C_VERSION__

#ifndef __OPENCL_C_VERSION__
#include <algorithm>
#include <numeric>
#include <vector>

typedef struct PhotonHash {
    std::vector<u32> cell_start;
    std::vector<u32> cell_end;
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
inline PhotonHash build_hash(std::vector<Photon> &photons, PhotonHashInfo info) {
    u32 n_photons = photons.size();
    PhotonHash grid;
    grid.bucket_count = (info.k * (info.k * (info.k + 1) + 1)) + 1; // Horner for efficiency

    std::vector<u32> hashes(n_photons);
    std::vector<u32> hashes_count(grid.bucket_count);

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
        grid.cell_end[i] = accum + hashes_count[i];
        accum += hashes_count[i];
    }

    return grid;
}
#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_PHOTON_HASH_H
