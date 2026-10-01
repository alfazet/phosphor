import numpy as np
import pyopencl as cl

_ctx = None
_queue = None
_kernels = None

_KERNEL_SOURCE = """
__kernel void mean_filter(__global const int *input, __global float *output, const int width, const int height, const int k)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;

    for (int c = 0; c < 3; c++) {
        float sum = 0.0f;
        int i = 0;
        for (int xx = clamp(x-k, 0, width - 1); xx <= clamp(x+k, 0, width - 1); xx++) {
            for (int yy = clamp(y-k, 0, height - 1); yy <= clamp(y+k, 0, height - 1); yy++) {
                sum += (float)input[(yy * width + xx) * 3 + c];
                i++;
            }
        }
        output[(y * width + x) * 3 + c] = sum / (float)i;
    }
}

#define MAX_WINDOW 49
__kernel void median_filter(__global const int *input, __global float *output, const int width, const int height, const int k)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;

    for (int c = 0; c < 3; c++) {

        // gathering
        int window[MAX_WINDOW];
        int idx = 0;
        for (int xx = clamp(x-k, 0, width - 1); xx <= clamp(x+k, 0, width - 1); xx++) {
            for (int yy = clamp(y-k, 0, height - 1); yy <= clamp(y+k, 0, height - 1); yy++) {
                window[idx++] = input[(yy * width + xx) * 3 + c];
            }
        }

        // insertion sort
        for (int i = 1; i < idx; i++) {
            int key = window[i];
            int j = i - 1;
            while (j >= 0 && window[j] > key) {
                window[j + 1] = window[j];
                j--;
            }
            window[j + 1] = key;
        }

        // getting median
        float med;
        if (idx % 2 == 0) {
            med = (window[idx / 2 - 1] + window[idx / 2]) / 2.0f;
        } else {
            med = window[idx / 2];
        }
        output[(y * width + x) * 3 + c] = med;
    }
}

__kernel void weighted_filter(__global const int *input, __global float *output, __global const float *weights, const int width, const int height, const int k)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;

    int kernel_size = 2 * k + 1;

    for (int c = 0; c < 3; c++) {
        float sum = 0.0f;
        for (int xx = clamp(x-k, 0, width - 1); xx <= clamp(x+k, 0, width - 1); xx++) {
            for (int yy = clamp(y-k, 0, height - 1); yy <= clamp(y+k, 0, height - 1); yy++) {
                int widx = (yy - y + k) * kernel_size + (xx - x + k);
                sum += (float)input[(yy * width + xx) * 3 + c] * weights[widx];
            }
        }
        output[(y * width + x) * 3 + c] = sum;
    }
}
"""


def _get_context():
    global _ctx, _queue, _kernels
    if _ctx is None:
        device = None
        for platform in cl.get_platforms():
            gpu_devices = platform.get_devices(device_type=cl.device_type.GPU)
            if gpu_devices:
                device = gpu_devices[0]
                break
        if device is None:
            device = cl.get_platforms()[0].get_devices()[0]
        _ctx = cl.Context([device])
        _queue = cl.CommandQueue(_ctx)
        _program = cl.Program(_ctx, _KERNEL_SOURCE).build()
        _kernels = {
            "mean_filter": cl.Kernel(_program, "mean_filter"),
            "median_filter": cl.Kernel(_program, "median_filter"),
            "weighted_filter": cl.Kernel(_program, "weighted_filter"),
        }

    return _ctx, _queue, _kernels


def _run_kernel(kernel, arr, k, extra_args=()):
    ctx, queue, _ = _get_context()
    h, w, c = arr.shape
    if c != 3:
        raise ValueError("expected an (H, W, 3) array")

    input_arr = np.ascontiguousarray(arr.astype(np.int32))
    output_arr = np.empty((h, w, 3), dtype=np.float32)

    mf = cl.mem_flags
    input_buf = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=input_arr)
    output_buf = cl.Buffer(ctx, mf.WRITE_ONLY, output_arr.nbytes)

    args = [input_buf, output_buf] + list(extra_args) + [np.int32(w), np.int32(h), np.int32(k)]
    kernel(queue, (w, h), None, *args)
    cl.enqueue_copy(queue, output_arr, output_buf)
    queue.finish()
    return output_arr.astype(np.float64)


def mean_filter_gpu(arr, k=1):
    ctx, _, kernels = _get_context()
    return _run_kernel(kernels["mean_filter"], arr, k)


def median_filter_gpu(arr, k=1):
    ctx, _, kernels = _get_context()
    if k > 2:
        raise ValueError("median filter k is > 3")
    return _run_kernel(kernels["median_filter"], arr, k)


def gaussian(x, y, sd):
    return 1 / np.sqrt(2 * np.pi) / sd * np.exp(-(x ** 2 + y ** 2) / (2 * sd ** 2))


def gaussian_filter_gpu(arr, k=1, sd=1):
    ctx, _, kernels = _get_context()
    kernel_size = 2 * k + 1
    weights = np.zeros((kernel_size, kernel_size), dtype=np.float32)
    for dy in range(-k, k + 1):
        for dx in range(-k, k + 1):
            weights[dy + k, dx + k] = gaussian(dx, dy, sd)
    weights /= weights.sum()
    weights_flat = np.ascontiguousarray(weights.flatten())

    mf = cl.mem_flags
    weights_buf = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=weights_flat)
    return _run_kernel(kernels["weighted_filter"], arr, k, extra_args=[weights_buf])
