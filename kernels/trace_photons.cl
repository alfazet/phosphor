#include "light.h"
#include "photon.h"
#include "random.h"
#include "ray.h"
#include "mock_scene.h"
#include "typedefs.h"

__kernel void trace_photons(__global Photon *photons, __global usize *photon_count, __global const Light *lights,
                            const usize n_lights, const usize photons_count, const RngState base_rng) {
    usize gid = get_global_id(0);
    if (gid >= photons_count)
        return;

    RngState rng = make_thread_rng(base_rng, gid);

    Light light = lights[0]; // TODO

    float4 origin = light.position;
    float4 dir = random_unit_vector(&rng);

    float4 power = light.power;

    f32 t;
    float4 normal;
    usize mat_id;
    if (intersect_scene(origin, dir, &t, &normal, &mat_id)) {
        float4 hit_pos = origin + dir * t;

        usize idx = atomic_inc(photon_count);
        if (idx < photons_count) {
            photons[idx].pos = hit_pos;
            photons[idx].power = power;
            photons[idx].dir = -dir;
        }
    }
}
