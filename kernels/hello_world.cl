// `ulong` is always 64 bits, just like size_t on modern PCs so let's use that for sizes
// (no, you can't pass a size_t, it won't compile because it wants to be cross-platform)
__kernel void hello_world(__global const float *a, __global const float *b, __global float *c, const ulong n) {
    // get_local_size(0/1/2) = blockDim.x/y/z
    // get_group_id(0/1/2) = blockIdx.x/y/z
    // get_local_id(0/1/2) = threadIdx.x/y/z
    ulong i = get_local_size(0) * get_group_id(0) + get_local_id(0);
    if (i < n)
        c[i] = a[i] + b[i];
}
