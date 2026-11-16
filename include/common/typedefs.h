#ifndef PHOSPHOR_TYPEDEFS_H
#define PHOSPHOR_TYPEDEFS_H

#ifndef __OPENCL_C_VERSION__

#include <CL/cl_platform.h>

typedef cl_float f32;
typedef cl_float2 float2;
typedef cl_float4 float4;

typedef cl_uchar u8;
typedef cl_int i32;
typedef cl_uint u32;
typedef cl_ulong u64;
typedef cl_ulong usize;

#else

typedef float f32;

typedef uchar u8;
typedef int i32;
typedef uint u32;
typedef ulong usize;

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_TYPEDEFS_H
