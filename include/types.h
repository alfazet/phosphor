#ifndef PHOSPHOR_TYPES_H
#define PHOSPHOR_TYPES_H

// this header is meant to be shared between host and device,
// so don't write any functions with pointer arguments here
// (value arguments are fine)

// mark all functions as `static inline`

// structs that will be used on both host and device
// need to be annotated with alignas(16)

// not using float3 in because there would be a padding mismatch
// OpenCL compiler pads it to 16 bytes, meanwhile the host compiler
// can do whatever - so let's just stick to float4

#ifndef __OPENCL_C_VERSION__

#include <CL/cl_platform.h>

typedef cl_float f32;
typedef cl_float2 float2;
typedef cl_float4 float4;

typedef cl_uchar u8;
typedef cl_int i32;
typedef cl_uint u32;
typedef cl_ulong usize;

#else

typedef float f32;
typedef uchar u8;
typedef int i32;
typedef uint u32;
typedef ulong usize;

#endif // __OPENCL_C_VERSION__

struct alignas(16) Material {
    float4 base_color;
    float4 emissive;

    usize diff_index;
    usize emis_index;
    usize norm_index;
    usize occlusion_index;
    usize metal_rough_index;

    f32 metallic;
    f32 roughness;
    f32 transmission;
    f32 ior;
};

// to be used when loading the mesh
struct Triangle {
    float4 v0, v1, v2;
    float4 uv0, uv1, uv2;
    float4 n0, n1, n2;
    float4 t0, t1, t2;
    Material mat;
};

// to be used in computation
struct alignas(16) ClTriangle {
    float4 *v0, *v1, *v2;
    float4 *uv0, *uv1, *uv2;
    float4 *n0, *n1, *n2;
    float4 *t0, *t1, *t2;
    Material *mat;
};

struct alignas(16) ClPhoton {
    float4 *pos;
    float4 *power;
    f32 *phi;
    f32 *theta;
    u8 *plane;
};

#endif // PHOSPHOR_TYPES_H
