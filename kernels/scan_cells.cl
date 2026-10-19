#include "typedefs.h"

// very naive (uses only one thread)
// but for a two-digit grid_res it's probably not worth it to bother with a proper scan implementation
__kernel void scan_cells(__global const u32 *cell_counts, __global u32 *cell_start, const u32 n) {
    u32 sum = 0;
    for (u32 i = 0; i < n; i++) {
        cell_start[i] = sum;
        sum += cell_counts[i];
    }
}
