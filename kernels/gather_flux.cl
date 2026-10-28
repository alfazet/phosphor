#include "common/constants.h"
#include "common/photon_hash.h"
#include "common/typedefs.h"

__kernel void gather_flux(
    // hit point and per-pixel state
    __global const float4 *hp_pos, __global const float4 *hp_normal, __global const float4 *hp_throughput,
    __global const float4 *hp_base_color, __global const float4 *hp_direct, __global const float4 *hp_emission,
    const u32 n_pixels,

    // photon map buffers
    __global const float4 *photon_pos, __global const u32 *photon_power, __global const u32 *photon_dir,
    const u32 n_photons,

    // spatial hash
    __global const u32 *cell_start, __global const u32 *cell_end, const PhotonHashData data,

    // sppm params
    const f32 sppm_alpha,

    // output
    __global float4 *flux, __global f32 *photon_weight, __global float4 *radiance) {

    u32 tid = get_global_id(0);
    if (tid >= n_pixels)
        return;

    float4 gathered_emission = hp_emission[tid];
    float4 radiance_so_far = radiance[tid];

    float4 r;
    r.x = gathered_emission.x;
    r.y = gathered_emission.y;
    r.z = gathered_emission.z;
    r.w = 0.0f;
    if (gathered_emission.w == 0.0f) {
        radiance[tid] = radiance_so_far + r;
        return;
    }

    float4 direct = hp_direct[tid];
    r.x += direct.x;
    r.y += direct.y;
    r.z += direct.z;
    radiance[tid] = radiance_so_far + r;

    float4 flux_so_far = flux[tid];
    f32 photon_weight_so_far = photon_weight[tid];
    f32 r_sq = flux_so_far.w;
    // first round has the initial search radius taken from cell size
    if (r_sq < EPS) {
        f32 half_cx = data.cell_sizes.x * 0.5f;
        f32 half_cy = data.cell_sizes.y * 0.5f;
        f32 half_cz = data.cell_sizes.z * 0.5f;
        r_sq = half_cx * half_cx + half_cy * half_cy + half_cz * half_cz;
    }

    float4 new_flux = BLACK;
    f32 max_dist_sq = 0.0f;
    f32 found = 0.0f; // called `M` in the paper

    gather_photon_flux(hp_pos[tid], hp_normal[tid], data, cell_start, cell_end, photon_pos, photon_power, photon_dir,
                       r_sq, &new_flux, &max_dist_sq, &found);

    // SPPM update
    if (found > 0.0f) {
        f32 new_photon_count = photon_weight_so_far + sppm_alpha * found;
        f32 ratio = new_photon_count / (photon_weight_so_far + found);
        f32 new_radius_sq = r_sq * ratio;

        float4 albedo = hp_base_color[tid];
        float4 contribution = hp_throughput[tid] * (albedo / PI) * new_flux;

        float4 temp = (flux_so_far + contribution) * ratio;
        flux[tid] = (float4)(temp.xyz, new_radius_sq);
        photon_weight[tid] = new_photon_count;
    }
}
