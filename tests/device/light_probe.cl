#include "common/material.h"
#include "common/random.h"
#include "common/texture_meta.h"
#include "common/triangle.h"
#include "common/typedefs.h"
#include "device/light_sampling.h"

__kernel void probe_sample_light(__global const Light *lights, u32 n_lights, __global const f32 *lum_pref_sum,
                                 f32 total_lum, float4 scene_center, f32 scene_radius,
                                 __global const TriangleVertices *etri_verts,
                                 __global const TriangleNormals *etri_normals, __global const TriangleUvs *etri_uvs,
                                 __global const u32 *etri_mat_index, __global const Material *materials,
                                 __global const UvTransformBlock *uv_transforms, __global const TextureMeta *tex_meta,
                                 __global const u8 *tex_atlas, u32 seed, u32 n_samples, __global float4 *out_origin,
                                 __global float4 *out_dir, __global float4 *out_power) {
    const u32 tid = get_global_id(0);
    if (tid >= n_samples)
        return;
    RngState rng = pcg_seed(seed + tid);

    float4 o = (float4)(0.0f);
    float4 d = (float4)(0.0f);
    float4 p = (float4)(0.0f);

    sample_light(&rng, lights, n_lights, lum_pref_sum, total_lum, scene_center, scene_radius, etri_verts, etri_normals,
                 etri_uvs, etri_mat_index, materials, uv_transforms, tex_meta, tex_atlas, &o, &d, &p);

    out_origin[tid] = o;
    out_dir[tid] = d;
    out_power[tid] = p;
}
