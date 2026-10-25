#include "common/constants.h"
#include "common/typedefs.h"

// adds big chunk offsets back to cell_start to finalize the scan
__kernel __attribute__((reqd_work_group_size(SCAN_WG_SIZE, 1, 1))) void
scan_add_offsets(__global u32 *cell_start, __global const u32 *chunk_offsets, const u32 n_cells) {
    u32 local_tid = get_local_id(0);
    u32 wg_id = get_group_id(0);
    u32 offset = chunk_offsets[wg_id];
    u32 chunk_start = wg_id * SCAN_CHUNK_SIZE;

    for (u32 i = 0; i < SCAN_ELEMENTS_PER_THREAD; i++) {
        u32 idx = chunk_start + local_tid + i * SCAN_WG_SIZE;
        if (idx < n_cells) {
            cell_start[idx] += offset;
        }
    }
}
