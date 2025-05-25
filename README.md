# Image Processing in C

## Project Overview

This project implements a command-line based image processing tool in C. It focuses on reading, manipulating, and saving BMP (Bitmap) image files, specifically 8-bit grayscale and 24-bit color images.

## Features

The project is divided into three main parts:

1.  **Part 1: 8-bit Grayscale Image Processing**
    *   Loading and saving 8-bit grayscale BMP images (`.bmp`).
    *   Reading and parsing BMP headers and color tables.
    *   Displaying image information.
    *   Basic image operations:
        *   Negative
        *   Brightness adjustment
        *   Thresholding (binary image)
        *   Image filtering using convolution kernels:
        *   Box Blur
        *   Gaussian Blur
        *   Outline
        *   Emboss
        *   Sharpen

2.  **Part 2: 24-bit Color Image Processing**
    *   Loading and saving 24-bit color BMP images.
    *   Handling 3-channel (RGB) pixel data.
    *   Basic image operations:
        *   Negative
        *   Conversion to Grayscale
        *   Brightness adjustment
        *   Image filtering using convolution kernels (applied to color images).

3.  **Part 3: Histogram Equalization**
    *   Calculating image histograms.
    *   Performing histogram equalization on 8-bit grayscale images.
    *   Performing histogram equalization on 24-bit color images (by converting to YUV color space and equalizing the Y/luminance channel).

## Core Functionality

*   **BMP File Handling:** Reading BMP file headers, info headers, and pixel data. Writing modified image data back to BMP files.
*   **Data Structures:** Custom C structs to represent image metadata and pixel data for both 8-bit and 24-bit images.
*   **Memory Management:** Dynamic allocation and deallocation of memory for image data.
*   **Command-Line Interface (CLI):** A menu-driven interface to allow users to select images and apply various processing operations.
