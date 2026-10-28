#include "common/constants.h"
#include "common/hit_point.h"
#include "common/photon_hash.h"
#include "common/pixel.h"
#include "common/typedefs.h"

__kernel void gather_flux(
    // hit point and per-pixel state
    __global const HitPoint *hit_points, __global Pixel *pixels, __global float4 *total_radiance, const u32 n_pixels,

    // photon map buffers
    __global const float4 *photon_pos, __global const u32 *photon_power, __global const u32 *photon_dir,
    const u32 n_photons,

    // spatial hash
    __global const u32 *cell_start, __global const u32 *cell_end, const PhotonHashData data,

    // sppm params
    const f32 sppm_alpha) {

    u32 tid = get_global_id(0);
    if (tid >= n_pixels)
        return;

    __global const HitPoint *hp = &hit_points[tid];
    if (hp->is_valid == 0) {
        float4 emission = hp->emission;
        float4 radiance;
        radiance.x = emission.x;
        radiance.y = emission.y;
        radiance.z = emission.z;
        radiance.w = 0.0f;
        total_radiance[tid] += radiance;
        return;
    }

    float4 direct = hp->direct;
    float4 emission = hp->emission;
    float4 radiance;
    radiance.x = direct.x + emission.x;
    radiance.y = direct.y + emission.y;
    radiance.z = direct.z + emission.z;
    radiance.w = 0.0f;
    total_radiance[tid] += radiance;

    Pixel px = pixels[tid];
    f32 r_sq = px.radius_sq;
    f32 photon_count = px.photon_count;
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

    gather_photon_flux(hp->position, data, cell_start, cell_end, photon_pos, photon_power, photon_dir, r_sq, hp->normal,
                       &new_flux, &max_dist_sq, &found);

    // SPPM update
    if (found > 0.0f) {
        f32 new_photon_count = photon_count + sppm_alpha * found;
        f32 ratio = new_photon_count / (photon_count + found);
        f32 new_radius_sq = r_sq * ratio;

        float4 albedo = hp->base_color;
        float4 contribution = hp->throughput * (albedo / PI) * new_flux;

        px.flux = (px.flux + contribution) * ratio;
        px.radius_sq = new_radius_sq;
        px.photon_count = new_photon_count;
        pixels[tid] = px;
    }
}
