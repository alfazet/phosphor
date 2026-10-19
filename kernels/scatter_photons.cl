#include "typedefs.h"

// scatters photons into an order sorted by the spatial hash
__kernel void scatter_photons(__global const float4 *photon_pos_in, __global const u32 *photon_power_in,
                              __global const u32 *photon_dir_in, __global const u32 *photon_hashes,
                              __global float4 *photon_pos_out, __global u32 *photon_power_out,
                              __global u32 *photon_dir_out, __global u32 *cell_end, const u32 n_photons) {
    u32 tid = get_global_id(0);
    if (tid >= n_photons)
        return;

    u32 h = photon_hashes[tid];
    u32 dst = atomic_inc(&cell_end[h]);

    photon_pos_out[dst] = photon_pos_in[tid];
    photon_power_out[dst] = photon_power_in[tid];
    photon_dir_out[dst] = photon_dir_in[tid];
}
