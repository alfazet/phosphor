#!/usr/bin/env python

import sys
import numpy as np
from PyQt6 import uic
from PyQt6.QtWidgets import QApplication, QMainWindow, QFileDialog
from PyQt6.QtGui import QPixmap, QImage, qRgb, QAction
from PyQt6.QtCore import Qt

def open_file():
    filename, _ = QFileDialog.getOpenFileName(
        window, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp)"
    )
    if filename:
        window.left_image.setPixmap(QPixmap(filename))

def save_file():
    filename, _ = QFileDialog.getSaveFileName(
        window, "Save Image", "", "PNG (*.png);;JPEG (*.jpg *.jpeg);;Bitmap (*.bmp)"
    )
    if filename:
        pixmap = window.right_image.pixmap()
        if pixmap:
            pixmap.save(filename)

def qimage_to_array(image):
    image = image.convertToFormat(QImage.Format.Format_RGB32)
    width = image.width()
    height = image.height()
    ptr = image.bits()
    ptr.setsize(height * width * 4)
    arr = np.frombuffer(ptr, dtype=np.uint8).reshape((height, width, 4))
    return arr[:, :, [2, 1, 0]].astype(np.int32)

def array_to_qimage(arr):
    arr = np.clip(arr, 0, 255).astype(np.uint8)
    height, width, _ = arr.shape
    bgra = np.zeros((height, width, 4), dtype=np.uint8)
    bgra[:, :, 0] = arr[:, :, 2]
    bgra[:, :, 1] = arr[:, :, 1]
    bgra[:, :, 2] = arr[:, :, 0]
    bgra[:, :, 3] = 255
    image = QImage(bgra.data, width, height, QImage.Format.Format_RGB32)
    return image.copy()

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
    result /= (kernel_size * kernel_size)
    return result

def process_pixels(func):
    pixmap = window.left_image.pixmap()
    if pixmap is None or pixmap.isNull():
        return

    arr = qimage_to_array(pixmap.toImage())
    result = func(arr)
    window.right_image.setPixmap(QPixmap.fromImage(array_to_qimage(result)))

algs = {
    "Mean3": lambda arr: mean_filter(arr, 1),
    "Mean5": lambda arr: mean_filter(arr, 2),
    "Median3": lambda arr: median_filter(arr, 1),
    "Median5": lambda arr: median_filter(arr, 2),
    "Gaussian3_3": lambda arr: gaussian_filter(arr, 1, 1),
    "Gaussian3_5": lambda arr: gaussian_filter(arr, 2, 2),
    "Gaussian5_3": lambda arr: gaussian_filter(arr, 1, 1),
    "Gaussian5_5": lambda arr: gaussian_filter(arr, 2, 2),
}

app = QApplication(sys.argv)

window = uic.loadUi("window.ui")
menubar = window.menubar
filters_menu = menubar.addMenu("Algorithms")

for name, func in algs.items():
    action = QAction(name, window)
    action.triggered.connect(lambda checked, f=func: process_pixels(f))
    filters_menu.addAction(action)

window.actionOpen.triggered.connect(open_file)
window.actionSave.triggered.connect(save_file)

window.show()
sys.exit(app.exec())
