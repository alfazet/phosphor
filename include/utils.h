#ifndef PHOSPHOR_UTILS_H
#define PHOSPHOR_UTILS_H

#include "typedefs.h"

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline u32 round_up_to_pow2(u32 x) {
    if (x == 0)
        return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
}

// reinterprets the bits of a 32 bit unsigned int as a float
inline f32 bits_as_float(u32 x) { return *(f32 *)(&x); }

#endif // PHOSPHOR_UTILS_H
