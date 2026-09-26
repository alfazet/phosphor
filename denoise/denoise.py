#!/usr/bin/env python

import sys
import numpy as np
from matplotlib.image import imread, imsave
from PyQt6 import uic
from PyQt6.QtWidgets import QApplication, QMainWindow, QFileDialog
from PyQt6.QtGui import QPixmap, QImage, qRgb, QAction
from PyQt6.QtCore import Qt

from filters_cpu import mean_filter, median_filter, gaussian_filter
from filters_gpu import mean_filter_gpu, median_filter_gpu, gaussian_filter_gpu

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

def process_pixels(func):
    pixmap = window.left_image.pixmap()
    if pixmap is None or pixmap.isNull():
        return

    arr = qimage_to_array(pixmap.toImage())
    result = func(arr)
    window.right_image.setPixmap(QPixmap.fromImage(array_to_qimage(result)))

algs_cpu = {
    "Mean3 (CPU)": lambda arr: mean_filter(arr, 1),
    "Mean5 (CPU)": lambda arr: mean_filter(arr, 2),
    "Median3 (CPU)": lambda arr: median_filter(arr, 1),
    "Median5 (CPU)": lambda arr: median_filter(arr, 2),
    "Gaussian3_1 (CPU)": lambda arr: gaussian_filter(arr, 1, 1),
    "Gaussian3_2 (CPU)": lambda arr: gaussian_filter(arr, 1, 2),
    "Gaussian5_1 (CPU)": lambda arr: gaussian_filter(arr, 2, 1),
    "Gaussian5_2 (CPU)": lambda arr: gaussian_filter(arr, 2, 2),
}

algs_gpu = {
    "Mean3 (GPU)": lambda arr: mean_filter_gpu(arr, 1),
    "Mean5 (GPU)": lambda arr: mean_filter_gpu(arr, 2),
    "Median3 (GPU)": lambda arr: median_filter_gpu(arr, 1),
    "Median5 (GPU)": lambda arr: median_filter_gpu(arr, 2),
    "Gaussian3_1 (GPU)": lambda arr: gaussian_filter_gpu(arr, 1, 1),
    "Gaussian3_2 (GPU)": lambda arr: gaussian_filter_gpu(arr, 1, 2),
    "Gaussian5_1 (GPU)": lambda arr: gaussian_filter_gpu(arr, 2, 1),
    "Gaussian5_2 (GPU)": lambda arr: gaussian_filter_gpu(arr, 2, 2),
}

algs = {**algs_cpu, **algs_gpu}

if len(sys.argv) > 1:
    image_path = sys.argv[1]
    image = imread(image_path)
    if image.dtype != np.uint8:
        image = (image * 255).round()
    for name, func in algs_gpu.items():
        result = func(image)
        imsave(f"{name}.png", np.clip(result, 0, 255).astype(np.uint8))
    sys.exit(0)


if len(sys.argv) > 1:
    image_path = sys.argv[1]
    image = imread(image_path)
    for name, func in algs_gpu.items():
        result = func(image)
        imsave(f"{name}.png", result)
    sys.exit(0)

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
