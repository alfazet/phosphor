#include "photon_hash.hpp"
#include "photon_hash.h"

#include <algorithm>
#include <numeric>
#include <unordered_map>

PhotonHash::PhotonHash(std::vector<float4> &photon_pos, std::vector<u32> &photon_power, std::vector<u32> &photon_dir,
                       PhotonHashInfo info) {
    u32 n_photons = photon_pos.size();
    this->bucket_count = info.grid_res * info.grid_res * info.grid_res + 1;

    std::vector<u32> hashes(n_photons);
    std::unordered_map<u32, u32> hashes_count;
    hashes_count.reserve(n_photons);

    for (u32 i = 0; i < n_photons; i++) {
        u32 hash = get_hash(photon_pos[i], info);
        hashes[i] = hash;
        hashes_count[hash]++;
    }

    std::vector<u32> indices(n_photons);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&hashes](u32 a, u32 b) { return hashes[a] < hashes[b]; });

    std::vector<float4> sorted_pos(n_photons);
    std::vector<u32> sorted_power(n_photons);
    std::vector<u32> sorted_dir(n_photons);
    for (u32 i = 0; i < n_photons; i++) {
        u32 src = indices[i];
        sorted_pos[i] = photon_pos[src];
        sorted_power[i] = photon_power[src];
        sorted_dir[i] = photon_dir[src];
    }
    photon_pos = std::move(sorted_pos);
    photon_power = std::move(sorted_power);
    photon_dir = std::move(sorted_dir);

    this->cell_start.assign(this->bucket_count, 0);
    this->cell_end.assign(this->bucket_count, 0);

    u32 accum = 0;
    for (u32 i = 0; i < this->bucket_count; i++) {
        this->cell_start[i] = accum;
        auto it = hashes_count.find(i);
        u32 count = (it != hashes_count.end()) ? it->second : 0;
        this->cell_end[i] = accum + count;
        accum += count;
    }
}
