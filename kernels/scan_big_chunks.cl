#include "common/constants.h"
#include "common/typedefs.h"

// similar to scan_small_chunks, but one level higher
// scans "small" chunk sums produced by scan_small_chunks into big_chunk_sums
__kernel __attribute__((reqd_work_group_size(SCAN_WG_SIZE, 1, 1))) void
scan_big_chunks(__global const u32 *small_chunk_sums, __global u32 *big_chunk_sums, const u32 n_small_chunks) {
    __local u32 sh_mem[SCAN_CHUNK_SIZE];
    __local u32 sums[SCAN_WG_SIZE];

    u32 local_tid = get_local_id(0);

    for (u32 i = 0; i < SCAN_ELEMENTS_PER_THREAD; i++) {
        u32 idx = local_tid + i * SCAN_WG_SIZE;
        sh_mem[idx] = (idx < n_small_chunks) ? small_chunk_sums[idx] : 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

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

    for (u32 i = 0; i < SCAN_ELEMENTS_PER_THREAD; i++) {
        u32 idx = local_tid + i * SCAN_WG_SIZE;
        if (idx < n_small_chunks)
            big_chunk_sums[idx] = sh_mem[idx];
    }
}
