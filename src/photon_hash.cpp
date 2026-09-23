#include "photon_hash.hpp"
#include "bounding_box.h"
#include "photon_hash.h"
#include "utils.h"

#include <algorithm>
#include <numeric>
#include <thread>
#include <unordered_map>

u32 choose_axis(const std::vector<float4> &photon_pos, const std::vector<u32> &indices, u32 start, u32 end) {
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

void balance(std::vector<float4> &photon_pos, std::vector<u32> &indices, u32 index, u32 start, u32 end,
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

PhotonHash::PhotonHash(std::vector<float4> &photon_pos, std::vector<u32> &photon_power,
                       std::vector<u32> &photon_dir, PhotonHashInfo info) {
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

    this->bucket_tree_offset.assign(this->bucket_count, 0);
    this->bucket_tree_size.assign(this->bucket_count, 0);

    u32 tree_total = 0;
    for (u32 b = 0; b < this->bucket_count; b++) {
        u32 count = this->cell_end[b] - this->cell_start[b];
        u32 size = count == 0 ? 0 : round_up_to_pow2(count + 1);
        this->bucket_tree_size[b] = size;
        this->bucket_tree_offset[b] = tree_total;
        tree_total += size;
    }

    std::vector<u32> kd_indices(n_photons);
    std::iota(kd_indices.begin(), kd_indices.end(), 0);
    this->tree_index.assign(tree_total, 0);

    std::vector<u32> active_buckets;
    for (u32 b = 0; b < this->bucket_count; b++) {
        if (this->cell_end[b] > this->cell_start[b])
            active_buckets.push_back(b);
    }
    u32 n_buckets = active_buckets.size();

    u32 n_threads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (u32 tid = 0; tid < n_threads; tid++) {
        u32 part_start = (tid * n_buckets) / n_threads;
        u32 part_end = ((tid + 1) * n_buckets) / n_threads;
        threads.emplace_back([&, part_start, part_end]() {
            for (u32 i = part_start; i < part_end; i++) {
                u32 b = active_buckets[i];
                balance(photon_pos, kd_indices, 1, this->cell_start[b], this->cell_end[b], this->tree_index,
                        this->bucket_tree_offset[b], this->bucket_tree_size[b]);
            }
        });
    }
    for (auto &t : threads)
        t.join();
}
