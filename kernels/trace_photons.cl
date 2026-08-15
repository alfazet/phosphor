#include "hit.h"
#include "light.h"
#include "material.h"
#include "photon.h"
#include "random.h"
#include "ray.h"
#include "typedefs.h"

__kernel void trace_photons(__global Photon *photons, __global u32 *photon_count, __global const Light *lights,
                            const u32 n_lights, const u32 max_photons, const u32 seed, __global const float4 *tri_v0,
                            __global const float4 *tri_v1, __global const float4 *tri_v2,
                            __global const u32 *tri_mat_index, const usize n_tris, __global const Material *materials) {
    usize gid = get_global_id(0);
    if (gid >= max_photons)
        return;

    RngState rng = pcg_seed(seed + (u32)gid);

    // pick light 0 for now
    Light light = lights[0];

    float4 origin = light.position;
    float4 dir = random_unit_vector(&rng);

    float4 power = light.power;

    HitRecord rec;
    if (intersect_scene(origin, dir, tri_v0, tri_v1, tri_v2, tri_mat_index, n_tris, &rec)) {
        float4 hit_pos = origin + dir * rec.t;

        u32 idx = atomic_inc(photon_count);
        if (idx < max_photons) {
            photons[idx].pos = hit_pos;
            photons[idx].power = power;
            photons[idx].dir = -dir;
        }
    }
}
