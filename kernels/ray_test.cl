#include "random.h"
#include "ray.h"
#include "typedefs.h"

__kernel void ray_test(__global const float4 *v_origin, __global const float4 *v_dir, __global float4 *res,
                       const usize n, const RngState global_rng) {
    usize i = get_global_id(0);
    if (i >= n) return;

    float4 origin = v_origin[i], dir = v_dir[i];

    RngState rng = make_thread_rng(global_rng, i);
    f32 t = random_float(&rng);

    res[i] = ray_at(origin, dir, t);
}
