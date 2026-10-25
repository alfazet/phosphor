#include "common/constants.h"
#include "common/typedefs.h"

// the work group with id wg_id scans cell_counts[wg_id * SCAN_CHUNK_SIZE .. (wg_id + 1) * SCAN_CHUNK_SIZE] into
// cell_start, the sum of the entire chunk is also written to chunk_sums[wg_id] in preparation for scan_big_chunks
__kernel __attribute__((reqd_work_group_size(SCAN_WG_SIZE, 1, 1))) void
scan_small_chunks(__global const u32 *cell_counts, __global u32 *cell_start, __global u32 *chunk_sums,
                  const u32 n_cells) {
    __local u32 sh_mem[SCAN_CHUNK_SIZE];
    __local u32 sums[SCAN_WG_SIZE];

    u32 local_tid = get_local_id(0); // id of this thread within this work group
    u32 wg_id = get_group_id(0);     // id of this work group
    u32 chunk_start = wg_id * SCAN_CHUNK_SIZE;

    for (u32 i = 0; i < SCAN_ELEMENTS_PER_THREAD; i++) {
        u32 idx = chunk_start + local_tid + i * SCAN_WG_SIZE;
        sh_mem[local_tid + i * SCAN_WG_SIZE] = (idx < n_cells) ? cell_counts[idx] : 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE); // wait for memory writes to finish

    u32 start_idx = local_tid * SCAN_ELEMENTS_PER_THREAD;
    u32 sum = 0;
    for (u32 i = 0; i < SCAN_ELEMENTS_PER_THREAD; i++) {
        u32 prev = sh_mem[start_idx + i];
        sh_mem[start_idx + i] = sum;
        sum += prev;
    }
    sums[local_tid] = sum;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (u32 stride = 1; stride < SCAN_WG_SIZE; stride *= 2) {
        u32 prev = (local_tid >= stride) ? sums[local_tid - stride] : 0;
        barrier(CLK_LOCAL_MEM_FENCE);
        sums[local_tid] += prev;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    u32 offset = (local_tid > 0) ? sums[local_tid - 1] : 0;
    for (u32 i = 0; i < SCAN_ELEMENTS_PER_THREAD; i++)
        sh_mem[start_idx + i] += offset;
    barrier(CLK_LOCAL_MEM_FENCE);
    if (local_tid == 0)
        chunk_sums[wg_id] = sums[SCAN_WG_SIZE - 1];

    for (u32 i = 0; i < SCAN_ELEMENTS_PER_THREAD; i++) {
        u32 idx = chunk_start + local_tid + i * SCAN_WG_SIZE;
        if (idx < n_cells)
            cell_start[idx] = sh_mem[local_tid + i * SCAN_WG_SIZE];
    }
}
