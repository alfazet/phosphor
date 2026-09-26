import numpy as np

def median_filter(arr, k=1):
    kernel_size = 2 * k + 1
    padded = np.pad(arr, ((k, k), (k, k), (0, 0)), mode='edge')
    h, w, c = arr.shape
    result = np.zeros_like(arr, dtype=np.float64)

    for i in range(h):
        for j in range(w):
            window = padded[i:i + kernel_size, j:j + kernel_size, :]
            result[i, j, :] = np.median(window, axis=(0, 1))

    return result

def mean_filter(arr, k=1):
    kernel_size = 2 * k + 1
    padded = np.pad(arr, ((k, k), (k, k), (0, 0)), mode='edge')
    result = np.zeros_like(padded, dtype=np.float64)
    for dy in range(-k, k + 1):
        for dx in range(-k, k + 1):
            shifted = np.roll(padded, shift=(dy, dx), axis=(0, 1))
            result += shifted
    result = result[k:-k, k:-k, :]
    result /= (kernel_size * kernel_size)
    return result

def gaussian(x, y, sd):
    return 1/np.sqrt(2*(np.pi))/sd*np.exp(-(x**2+y**2)/(2*sd**2))

def gaussian_filter(arr, k=1, sd=1):
    kernel_size = 2 * k + 1

    padded = np.pad(arr, ((k, k), (k, k), (0, 0)), mode='edge')
    result = np.zeros_like(padded, dtype=np.float64)
    kernel = np.zeros(shape=(kernel_size, kernel_size), dtype=np.float64)
    for dy in range(-k, k + 1):
        for dx in range(-k, k + 1):
            kernel[dy + k, dx + k] = gaussian(dx, dy, sd)

    kernel /= kernel.sum()

    for dy in range(-k, k + 1):
        for dx in range(-k, k + 1):
            shifted = np.roll(padded, shift=(dy, dx), axis=(0, 1))
            result += shifted * kernel[dy + k, dx + k]
    result = result[k:-k, k:-k, :]
    return result
