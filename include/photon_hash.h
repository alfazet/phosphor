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
    u32 x = (u32)((pos.x - info.origin.x) / info.cell_sizes.x);
    u32 y = (u32)((pos.y - info.origin.y) / info.cell_sizes.y);
    u32 z = (u32)((pos.z - info.origin.z) / info.cell_sizes.z);
    // return k * k * (u32)(x + k) + k * (u32)(y + k) + (u32)(z + k); // remember to change bucket_count if reverted
    return k * k * (u32)(x) + k * (u32)(y) + (u32)(z);
}

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
    grid.bucket_count = (info.k * (info.k * (info.k + 1) + 1)); // Horner for efficiency

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
