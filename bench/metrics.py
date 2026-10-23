import numpy as np
from numpy.lib.stride_tricks import sliding_window_view

def _gauss(size=11, sigma=1.5):
    x = np.arange(size) - (size - 1) / 2
    g = np.exp(-x ** 2 / (2 * sigma ** 2))
    return g / g.sum()

def _blur(img, g):
    k = len(g)
    v = sliding_window_view(img, k, axis=0) @ g
    return sliding_window_view(v, k, axis=1) @ g

# original paper: https://ece.uwaterloo.ca/~z70wang/publications/ssim.pdf
# https://github.com/zhenglab/IQA/tree/master/Codes/OriginalVersion/SSIM
def ssim_channel(a, b, data_range=1.0):
    C1, C2 = (0.01 * data_range) ** 2, (0.03 * data_range) ** 2
    g = _gauss()
    ma, mb = _blur(a, g), _blur(b, g)
    saa = _blur(a * a, g) - ma * ma
    sbb = _blur(b * b, g) - mb * mb
    sab = _blur(a * b, g) - ma * mb
    m = ((2 * ma * mb + C1) * (2 * sab + C2)) / ((ma * ma + mb * mb + C1) * (saa + sbb + C2))
    return float(m.mean())

def ssim(img, ref):
    return float(np.mean([ssim_channel(img[..., c], ref[..., c]) for c in range(3)]))

def mse(img, ref):
    return float(np.mean((img - ref) ** 2))
