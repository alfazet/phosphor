#ifndef PHOSPHOR_TYPES_H
#define PHOSPHOR_TYPES_H

// types accessible in both in C++ code and in kernels
#include <CL/cl_platform.h>

// not using float3 since there's a padding mismatch
// OpenCL compiler pads it to 16 bytes, meanwhile the host compiler
// can do whatever - so let's just stick to float4

typedef cl_float f32;
typedef cl_float2 float2;
typedef cl_float4 float4;

typedef cl_uchar u8;
typedef cl_int i32;
typedef cl_uint u32;
typedef cl_ulong usize;

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

struct alignas(16) Triangle {
    float4 *v0, *v1, *v2;
    float4 *uv0, *uv1, *uv2;
    float4 *n0, *n1, *n2;
    float4 *t0, *t1, *t2;
    Material *mat;
};

struct alignas(16) Photon {
    float4 *pos;
    float4 *power;
    f32 *phi;
    f32 *theta;
    u8 *plane;
};

#endif // PHOSPHOR_TYPES_H
