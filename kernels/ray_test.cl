#include "random.h"
#include "ray.h"
#include "typedefs.h"

__kernel void ray_test(__global const float4 *v_origin, __global const float4 *v_dir, __global float4 *res,
                       const usize n, const RngState global_rng) {
    usize i = get_local_size(0) * get_group_id(0) + get_local_id(0);
    float4 origin = v_origin[i], dir = v_dir[i];

    RngState rng = make_thread_rng(global_rng, i);
    f32 t = random_float(&rng);

    if (i < n)
        res[i] = ray_at(origin, dir, t);
}
