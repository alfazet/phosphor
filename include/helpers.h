#include "typedefs.h"

// https://stackoverflow.com/questions/364985/algorithm-for-finding-the-smallest-power-of-two-thats-greater-or-equal-to-a-giv
/// Round up to next higher power of 2 (return x if it's already a power
/// of 2).
inline u32 pow2roundup(u32 x) {
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

inline float4 compute_bary(float bary_x, float bary_y, float4 v0, float4 v1, float4 v2) {
    float w0 = 1.0f - bary_x - bary_y;
    float w1 = bary_x;
    float w2 = bary_y;

    float4 result;
    result.x = w0 * v0.x + w1 * v1.x + w2 * v2.x;
    result.y = w0 * v0.y + w1 * v1.y + w2 * v2.y;
    result.z = w0 * v0.z + w1 * v1.z + w2 * v2.z;
    result.w = w0 * v0.w + w1 * v1.w + w2 * v2.w;
    return result;
}