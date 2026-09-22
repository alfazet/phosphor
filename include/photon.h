#ifndef PHOSPHOR_PHOTON_H
#define PHOSPHOR_PHOTON_H

#include "constants.h"
#include "typedefs.h"

// this struct should be unused (we're using SoA), but keep it for documentation
typedef struct GPU_ALIGN Photon {
    float4 pos; // .xyz - position, .w - axis for kd-tree (use as_float/as_uint)
    u32 power;  // RGBE
    u32 dir;    // incoming direction normalized, (octahedral encoding)
    // 4 * 4 + 4 + 4 = 24

    // total: 24
    // u8 _padding[8];
} Photon;

#ifdef __OPENCL_C_VERSION__

// https://www.graphics.cornell.edu/%7Ebjw/rgbe/rgbe.c
u32 encode_rgbe(float4 power) {
    f32 m = fmax(power.x, fmax(power.y, power.z));
    if (m <= 0.0f)
        return 0u;

    i32 e;
    frexp(m, &e);               // m = frac * 2^e, frac in [0.5, 1)
    f32 scale = ldexp(1.0f, e); // 2^e

    u32 r = (uint)(power.x / scale * 256.0f);
    u32 g = (uint)(power.y / scale * 256.0f);
    u32 b = (uint)(power.z / scale * 256.0f);
    u32 E = (uint)(e + 128);

    r = min(r, 255u);
    g = min(g, 255u);
    b = min(b, 255u);

    return r | (g << 8) | (b << 16) | (E << 24);
}

float4 decode_rgbe(u32 rgbe) {
    u32 E = rgbe >> 24;
    if (E == 0)
        return (float4)(0.0f);

    f32 scale = ldexp(1.0f, (i32)E - 128 - 8); // 2^(E-128) / 256

    return (float4)((rgbe & 0xFF) * scale, ((rgbe >> 8) & 0xFF) * scale, ((rgbe >> 16) & 0xFF) * scale, 0.0f);
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/

float2 oct_wrap(float2 v) {
    return (1.0f - fabs(v.yx)) * (float2)(v.x >= 0.0f ? 1.0f : -1.0f, v.y >= 0.0f ? 1.0f : -1.0f);
}

u32 encode_oct(float4 dir) {
    dir = normalize(dir);

    f32 l = fabs(dir.x) + fabs(dir.y) + fabs(dir.z);
    float2 n = dir.xy / l;

    n = (dir.z >= 0.0f) ? n : oct_wrap(n);

    u32 ux = (u32)(clamp(n.x * 0.5f + 0.5f, 0.0f, 1.0f) * 65535.0f);
    u32 uy = (u32)(clamp(n.y * 0.5f + 0.5f, 0.0f, 1.0f) * 65535.0f);

    return ux | (uy << 16);
}

float4 decode_oct(u32 packed) {
    float2 f;
    f.x = (packed & 0xFFFF) / 65535.0f;
    f.y = ((packed >> 16) & 0xFFFF) / 65535.0f;

    f = f * 2.0f - 1.0f;

    float4 n = (float4)(f.x, f.y, 1.0f - fabs(f.x) - fabs(f.y), 0.0f);

    f32 t = clamp(-n.z, 0.0f, 1.0f);
    n.xy += (float2)(n.x >= 0.0f ? -t : t, n.y >= 0.0f ? -t : t);

    return normalize(n);
}

#endif // __OPENCL_C_VERSION__

#endif // PHOSPHOR_PHOTON_H
