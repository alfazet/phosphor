#include "constants.h"
#include "hitpoint.h"
#include "photon_hash.h"
#include "sppm_pixel.h"
#include "typedefs.h"

__kernel void gather_pass(
    // hit point and per-pixel state
    __global const HitPoint *hit_points, __global SppmPixel *sppm_pixels, __global float4 *total_irradiance,
    const u32 n_pixels,

    // photon map buffers
    __global const float4 *photon_pos, __global const float4 *photon_power, __global const float4 *photon_dir,
    __global const float4 *photon_normal, const u32 n_photons,

    // spatial hash
    __global const u32 *tree_index, __global const u32 *bucket_tree_offset, __global const u32 *bucket_tree_size,
    const PhotonHashInfo info,

    const f32 sppm_alpha) {

    u32 tid = get_global_id(0);
    if (tid >= n_pixels)
        return;

    HitPoint hp = hit_points[tid];
    SppmPixel sp = sppm_pixels[tid];
    if (hp.is_valid == 0) {
        sppm_pixels[tid] = sp;
        return;
    }

    float4 irradiance;
    irradiance.x = hp.direct.x + hp.emission.x;
    irradiance.y = hp.direct.y + hp.emission.y;
    irradiance.z = hp.direct.z + hp.emission.z;
    irradiance.w = 0.0f;
    total_irradiance[tid] += irradiance;

    f32 r_sq = sp.radius_sq;
    f32 photon_count = sp.photon_count;
    // first round has the initial search radius taken from cell size
    if (r_sq < EPS) {
        f32 half_cx = info.cell_sizes.x * 0.5f;
        f32 half_cy = info.cell_sizes.y * 0.5f;
        f32 half_cz = info.cell_sizes.z * 0.5f;
        r_sq = half_cx * half_cx + half_cy * half_cy + half_cz * half_cz;
    }

    float4 new_flux = BLACK;
    f32 max_dist_sq = 0.0f;
    f32 found = 0.0f; // called `M` in the paper

    gather_photon_flux(hp.position, info, tree_index, bucket_tree_offset, bucket_tree_size, photon_pos, photon_power,
                       photon_dir, photon_normal, r_sq, hp.normal, &new_flux, &max_dist_sq, &found);

    // SPPM update
    if (found > 0.0f) {
        f32 new_photon_count = photon_count + sppm_alpha * found;
        f32 ratio = new_photon_count / (photon_count + found);
        f32 new_radius_sq = r_sq * ratio;

        float4 albedo = hp.base_color * (1.0f - hp.metallic);
        float4 contribution = hp.throughput * (albedo / PI) * new_flux;

        sp.flux = (sp.flux + contribution) * ratio;
        sp.radius_sq = new_radius_sq;
        sp.photon_count = new_photon_count;
    }
    sppm_pixels[tid] = sp;
}
