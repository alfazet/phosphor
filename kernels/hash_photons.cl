#include "photon_hash.h"
#include "typedefs.h"

__kernel void hash_photons(__global const float4 *photon_pos, __global u32 *photon_hashes, __global u32 *cell_counts,
                           const u32 n_photons, const PhotonHashInfo info) {
    u32 tid = get_global_id(0);
    if (tid >= n_photons)
        return;

    u32 h = get_hash(photon_pos[tid], info);
    photon_hashes[tid] = h;
    atomic_inc(&cell_counts[h]);
}
