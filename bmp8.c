#include "bmp8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For memcpy, calloc
#include <math.h>   // For round

// Helper function to extract unsigned int from header
static unsigned int read_uint_le(const unsigned char *buffer, int offset) {
    return buffer[offset] | (buffer[offset + 1] << 8) | (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
}

// Helper function to extract unsigned short from header
static unsigned short read_ushort_le(const unsigned char *buffer, int offset) {
    return buffer[offset] | (buffer[offset + 1] << 8);
}

t_bmp8 *bmp8_loadImage(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error opening file for reading");
        return NULL;
    }

    t_bmp8 *img = (t_bmp8 *)malloc(sizeof(t_bmp8));
    if (!img) {
        fprintf(stderr, "Error: Failed to allocate memory for t_bmp8 structure.\n");
        fclose(file);
        return NULL;
    }

    if (fread(img->header, sizeof(unsigned char), 54, file) != 54) {
        fprintf(stderr, "Error: Failed to read BMP header.\n");
        free(img);
        fclose(file);
        return NULL;
    }

    if (img->header[0] != 'B' || img->header[1] != 'M') {
        fprintf(stderr, "Error: Not a BMP file (invalid signature).\n");
        free(img);
        fclose(file);
        return NULL;
    }

    // Extract data from header
    img->width = read_uint_le(img->header, 18);
    img->height = read_uint_le(img->header, 22);
    img->colorDepth = read_ushort_le(img->header, 28);
    img->dataSize = read_uint_le(img->header, 34);

    if (img->colorDepth != 8) {
        fprintf(stderr, "Error: Image is not 8-bit (colorDepth = %u).\n", img->colorDepth);
        free(img);
        fclose(file);
        return NULL;
    }

    if (img->dataSize == 0) {
        img->dataSize = img->width * img->height;
    }


    // Read color table (256 entries * 4 bytes/entry = 1024 bytes for 8-bit BMP)
    if (fread(img->colorTable, sizeof(unsigned char), 1024, file) != 1024) {
        fprintf(stderr, "Error: Failed to read color table.\n");
        free(img);
        fclose(file);
        return NULL;
    }

    img->data = (unsigned char *)malloc(img->dataSize);
    if (!img->data) {
        fprintf(stderr, "Error: Failed to allocate memory for pixel data.\n");
        free(img);
        fclose(file);
        return NULL;
    }

    if (fread(img->data, sizeof(unsigned char), img->dataSize, file) != img->dataSize) {
        fprintf(stderr, "Error: Failed to read pixel data (read %ld, expected %u).\n", ftell(file), img->dataSize);
        free(img->data);
        free(img);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return img;
}

void bmp8_saveImage(const char *filename, t_bmp8 *img) {
    if (!img) {
        fprintf(stderr, "Error: Cannot save NULL image.\n");
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Error opening file for writing");
        return;
    }

    if (fwrite(img->header, sizeof(unsigned char), 54, file) != 54) {
        fprintf(stderr, "Error: Failed to write BMP header.\n");
        fclose(file);
        return;
    }

    if (fwrite(img->colorTable, sizeof(unsigned char), 1024, file) != 1024) {
        fprintf(stderr, "Error: Failed to write color table.\n");
        fclose(file);
        return;
    }

    if (fwrite(img->data, sizeof(unsigned char), img->dataSize, file) != img->dataSize) {
        fprintf(stderr, "Error: Failed to write pixel data.\n");
        fclose(file);
        return;
    }

    fclose(file);
}

void bmp8_free(t_bmp8 *img) {
    if (img) {
        if (img->data) {
            free(img->data);
        }
        free(img);
    }
}

void bmp8_printInfo(t_bmp8 *img) {
    if (!img) {
        printf("Image Info: NULL image\n");
        return;
    }
    printf("Image Info:\n");
    printf("  Width: %u\n", img->width);
    printf("  Height: %u\n", img->height);
    printf("  Color Depth: %u\n", img->colorDepth);
    printf("  Data Size: %u\n", img->dataSize);
}

void bmp8_negative(t_bmp8 *img) {
    if (!img || !img->data) return;
    for (unsigned int i = 0; i < img->dataSize; i++) {
        img->data[i] = 255 - img->data[i];
    }
}

void bmp8_brightness(t_bmp8 *img, int value) {
    if (!img || !img->data) return;
    for (unsigned int i = 0; i < img->dataSize; i++) {
        int new_val = (int)img->data[i] + value;
        if (new_val < 0) new_val = 0;
        if (new_val > 255) new_val = 255;
        img->data[i] = (unsigned char)new_val;
    }
}

void bmp8_threshold(t_bmp8 *img, int threshold_val) {
    if (!img || !img->data) return;
    for (unsigned int i = 0; i < img->dataSize; i++) {
        if (img->data[i] >= threshold_val) {
            img->data[i] = 255;
        } else {
            img->data[i] = 0;
        }
    }
}

void bmp8_applyFilter(t_bmp8 *img, float **kernel, int kernelSize) {
    if (!img || !img->data || !kernel || kernelSize % 2 == 0 || kernelSize < 1) {
        fprintf(stderr, "Error: Invalid parameters for bmp8_applyFilter.\n");
        return;
    }

    unsigned char *original_data = (unsigned char *)malloc(img->dataSize);
    if (!original_data) {
        fprintf(stderr, "Error: Failed to allocate memory for temporary data in applyFilter.\n");
        return;
    }
    memcpy(original_data, img->data, img->dataSize);

    int n = kernelSize / 2;
    unsigned int width = img->width;
    unsigned int height = img->height;

    for (unsigned int y_center = n; y_center < height - n; y_center++) {
        for (unsigned int x_center = n; x_center < width - n; x_center++) {
            float sum = 0.0f;
            for (int i_offset = -n; i_offset <= n; i_offset++) {
                for (int j_offset = -n; j_offset <= n; j_offset++) {
                    // Image pixel coordinates based on convolution formula I(x-i, y-j)
                    unsigned int img_y = y_center - i_offset;
                    unsigned int img_x = x_center - j_offset;

                    float kernel_val = kernel[i_offset + n][j_offset + n];

                    sum += (float)original_data[img_y * width + img_x] * kernel_val;
                }
            }
            int val = (int)round(sum);
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            img->data[y_center * width + x_center] = (unsigned char)val;
        }
    }
    free(original_data);
}

unsigned int *bmp8_computeHistogram(t_bmp8 *img) {
    if (!img || !img->data) return NULL;

    // Use calloc to initialize histogram to zeros
    unsigned int *hist = (unsigned int *)calloc(256, sizeof(unsigned int));
    if (!hist) {
        fprintf(stderr, "Error: Failed to allocate memory for histogram.\n");
        return NULL;
    }

    for (unsigned int i = 0; i < img->dataSize; i++) {
        hist[img->data[i]]++;
    }
    return hist;
}

unsigned int *bmp8_computeCDF(const unsigned int *hist) {
    if (!hist) return NULL;

    unsigned int *hist_eq_map = (unsigned int *)malloc(256 * sizeof(unsigned int));
    if (!hist_eq_map) {
        fprintf(stderr, "Error: Failed to allocate memory for CDF/hist_eq_map.\n");
        return NULL;
    }

    unsigned int cdf[256];
    unsigned int N = 0; // Total number of pixels

    // Calculate CDF and total number of pixels
    cdf[0] = hist[0];
    N = hist[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i - 1] + hist[i];
        N += hist[i];
    }

    if (N == 0) { // Empty image or all hist entries are 0
        for (int i = 0; i < 256; i++) hist_eq_map[i] = i;
        return hist_eq_map;
    }

    // Find cdf_min (smallest non-zero CDF value)
    unsigned int cdf_min_val = N;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] > 0) {
            cdf_min_val = cdf[i];
            break;
        }
    }

    double denominator = (double)N - cdf_min_val;

    for (int i = 0; i < 256; i++) {
        if (denominator == 0.0) {
            hist_eq_map[i] = i;
        } else {
            double val = round(((double)cdf[i] - cdf_min_val) / denominator * 255.0);
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            hist_eq_map[i] = (unsigned int)val;
        }
    }
    return hist_eq_map;
}

void bmp8_equalize(t_bmp8 *img, const unsigned int *hist_eq_map) {
    if (!img || !img->data || !hist_eq_map) return;

    for (unsigned int i = 0; i < img->dataSize; i++) {
        img->data[i] = (unsigned char)hist_eq_map[img->data[i]];
    }
}