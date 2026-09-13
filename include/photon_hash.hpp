#ifndef PHOSPHOR_PHOTON_HASH_HPP
#define PHOSPHOR_PHOTON_HASH_HPP

#include "photon_hash.h"
#include "typedefs.h"

#include <vector>

typedef struct PhotonHash {
    std::vector<u32> cell_start;
    std::vector<u32> cell_end;
    std::vector<u32> tree_index;
    std::vector<u32> bucket_tree_offset;
    std::vector<u32> bucket_tree_size;
    u32 bucket_count;

    PhotonHash(std::vector<float4> &photon_pos, std::vector<float4> &photon_power, std::vector<float4> &photon_dir,
               std::vector<float4> &photon_normal, PhotonHashInfo info);
} PhotonHash;

#endif // PHOSPHOR_PHOTON_HASH_HPP
