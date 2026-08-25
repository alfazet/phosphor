#ifndef PHOSPHOR_TYPEDEFS_H
#define PHOSPHOR_TYPEDEFS_H

// PSA: this and all other .h headers are meant to be shared between host and device, so a couple of rules apply:
// - no C++ features
// - no pointers in structs (in short: when passing structs to kernels, a memcpy happens and a
// cl::Buffer isn't a device pointer but some special wrapper where the actual
// address gets resolved only at kernel dispatch time and only when passed in through `kernel.setArg`, I guess
// that's just the price you pay for no vendor-lock...)
// - structs that will be used on both host and device need to be annotated with GPU_ALIGN
// and padded to have their size be multiple of 16 (the compilers would insert that padding by themselves, but then
// we couldn't be sure if they both did in the same way, so it's better to do it manually)
// - don't use float3 because there's a size mismatch, just use float4 instead and ignore the last field

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

#endif // PHOSPHOR_TYPEDEFS_H
