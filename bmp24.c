#include "bmp24.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// To help for uint8_t
static uint8_t float_to_uint8_clamp(float val) {
    if (val < 0.0f) return 0;
    if (val > 255.0f) return 255;
    return (uint8_t)roundf(val);
}

t_pixel **bmp24_allocateDataPixels(int width, int height) {
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: Invalid dimensions for pixel data allocation (%d x %d).\n", width, height);
        return NULL;
    }
    t_pixel **pixels = (t_pixel **)malloc(height * sizeof(t_pixel *));
    if (!pixels) {
        fprintf(stderr, "Error: Failed to allocate memory for pixel rows.\n");
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        pixels[i] = (t_pixel *)malloc(width * sizeof(t_pixel));
        if (!pixels[i]) {
            fprintf(stderr, "Error: Failed to allocate memory for pixel row %d.\n", i);
            for (int j = 0; j < i; j++) {
                free(pixels[j]);
            }
            free(pixels);
            return NULL;
        }
        // Initialize pixels to black
        memset(pixels[i], 0, width * sizeof(t_pixel));
    }
    return pixels;
}

void bmp24_freeDataPixels(t_pixel **pixels, int height) {
    if (!pixels) return;
    // height is expected to be positive and to match allocation
    if (height <= 0) return;

    for (int i = 0; i < height; i++) {
        free(pixels[i]);
    }
    free(pixels);
}

t_bmp24 *bmp24_allocate(int width, int signed_height, int colorDepth) {
    int actual_height = (signed_height > 0) ? signed_height : -signed_height;

    if (width <= 0 || actual_height <= 0) {
        fprintf(stderr, "Error: Invalid dimensions for bmp24 allocation (width: %d, height: %d).\n", width, actual_height);
        return NULL;
    }
    t_bmp24 *img = (t_bmp24 *)malloc(sizeof(t_bmp24));
    if (!img) {
        fprintf(stderr, "Error: Failed to allocate memory for t_bmp24 structure.\n");
        return NULL;
    }

    img->data = bmp24_allocateDataPixels(width, actual_height);
    if (!img->data) {
        free(img);
        return NULL;
    }

    img->width = width;
    img->height = actual_height;
    img->colorDepth = colorDepth;

    memset(&img->header, 0, sizeof(t_bmp_header));
    memset(&img->header_info, 0, sizeof(t_bmp_info));

    img->header.type = BMP_TYPE;
    img->header.offset = sizeof(t_bmp_header) + sizeof(t_bmp_info);

    img->header_info.size = sizeof(t_bmp_info);
    img->header_info.width = width;
    img->header_info.height = signed_height;
    img->header_info.planes = 1;
    img->header_info.bits = (uint16_t)colorDepth;
    img->header_info.compression = 0;

    uint32_t row_size_bytes = ((uint32_t)width * (uint32_t)img->header_info.bits + 31u) / 32u * 4u;

    img->header_info.imagesize = row_size_bytes * (uint32_t)actual_height;
    img->header.size = img->header.offset + img->header_info.imagesize;

    img->header_info.xresolution = 0;
    img->header_info.yresolution = 0;
    img->header_info.ncolors = 0;
    img->header_info.importantcolors = 0;

    return img;
}

void bmp24_free(t_bmp24 *img) {
    if (!img) return;
    if (img->data) {
        bmp24_freeDataPixels(img->data, img->height);
    }
    free(img);
}

void file_rawRead (uint32_t position, void * buffer, uint32_t size, size_t n, FILE * file) {
    fseek(file, position, SEEK_SET);
    fread(buffer, size, n, file);
}

void file_rawWrite (uint32_t position, void * buffer, uint32_t size, size_t n, FILE * file) {
    fseek(file, position, SEEK_SET);
    fwrite(buffer, size, n, file);
}

void bmp24_readPixelValue(const t_bmp24 *image, int x, int y, const FILE *file) {
    (void)image; (void)x; (void)y; (void)file;
}

void bmp24_writePixelValue(const t_bmp24 *image, int x, int y, const FILE *file) {
    (void)image; (void)x; (void)y; (void)file;
}

void bmp24_readPixelData(t_bmp24 *image, FILE *file) {
    if (!image || !image->data || !file) {
        fprintf(stderr, "Error: NULL image, image data, or file pointer in bmp24_readPixelData.\n");
        return;
    }

    if (fseek(file, image->header.offset, SEEK_SET) != 0) {
        printf("Error: Failed to seek to pixel data offset in bmp24_readPixelData");
        return;
    }

    uint32_t bytes_per_pixel = image->header_info.bits / 8;
    uint32_t row_pitch = ((uint32_t)image->width * bytes_per_pixel + 3u) & ~3u;
    uint32_t padding_per_row = row_pitch - (uint32_t)image->width * bytes_per_pixel;

    for (int y = image->height - 1; y >= 0; y--) {
        for (int x = 0; x < image->width; x++) {
            if (fread(&image->data[y][x].blue, sizeof(uint8_t), 1, file) != 1 ||
                fread(&image->data[y][x].green, sizeof(uint8_t), 1, file) != 1 ||
                fread(&image->data[y][x].red, sizeof(uint8_t), 1, file) != 1) {
                fprintf(stderr, "Error: Failed to read pixel data for (%d, %d).\n", x, y);
                if(feof(file)) fprintf(stderr, "EOF reached prematurely.\n");
                if(ferror(file)) printf("File error during read");
                return;
            }
        }
        if (padding_per_row > 0) {
            if (fseek(file, padding_per_row, SEEK_CUR) != 0) {
                printf("Error: Failed to seek past padding bytes during read");
                return;
            }
        }
    }
}

void bmp24_writePixelData(t_bmp24 *image, FILE *file) {
    if (!image || !image->data || !file) {
        fprintf(stderr, "Error: NULL image, image data, or file pointer in bmp24_writePixelData.\n");
        return;
    }

    if (fseek(file, image->header.offset, SEEK_SET) != 0) {
         printf("Error: Failed to seek to pixel data offset in bmp24_writePixelData");
        return;
    }

    uint32_t bytes_per_pixel = image->header_info.bits / 8;
    uint32_t row_pitch = ((uint32_t)image->width * bytes_per_pixel + 3u) & ~3u;
    uint32_t padding_per_row = row_pitch - (uint32_t)image->width * bytes_per_pixel;
    uint8_t pad_byte = 0;

    for (int y = image->height - 1; y >= 0; y--) {
        for (int x = 0; x < image->width; x++) {
            if (fwrite(&image->data[y][x].blue, sizeof(uint8_t), 1, file) != 1 ||
                fwrite(&image->data[y][x].green, sizeof(uint8_t), 1, file) != 1 ||
                fwrite(&image->data[y][x].red, sizeof(uint8_t), 1, file) != 1) {
                fprintf(stderr, "Error: Failed to write pixel data for (%d, %d).\n", x, y);
                return;
            }
        }
        if (padding_per_row > 0) {
            for (uint32_t k = 0; k < padding_per_row; k++) {
                if (fwrite(&pad_byte, sizeof(uint8_t), 1, file) != 1) {
                    fprintf(stderr, "Error: Failed to write padding byte.\n");
                    return;
                }
            }
        }
    }
}

t_bmp24 *bmp24_loadImage(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file for reading");
        return NULL;
    }

    t_bmp_header bmpHeader;
    t_bmp_info bmpInfoHeader;

    if (fread(&bmpHeader, sizeof(t_bmp_header), 1, file) != 1) {
        fprintf(stderr, "Error reading BMP file header.\n"); fclose(file); return NULL;
    }
    if (fread(&bmpInfoHeader, sizeof(t_bmp_info), 1, file) != 1) {
        fprintf(stderr, "Error reading BMP info header.\n"); fclose(file); return NULL;
    }


    if (bmpHeader.type != BMP_TYPE) {
        fprintf(stderr, "Error: Not a BMP file. Signature is %04X.\n", bmpHeader.type);
        fclose(file);
        return NULL;
    }

    if (bmpInfoHeader.bits != 24) {
        fprintf(stderr, "Error: Not a 24-bit BMP file. Bits per pixel: %d.\n", bmpInfoHeader.bits);
        fclose(file);
        return NULL;
    }

    if (bmpInfoHeader.compression != 0) {
        fprintf(stderr, "Error: Compressed BMP files are not supported. Compression type: %u.\n", bmpInfoHeader.compression);
        fclose(file);
        return NULL;
    }

    if (bmpInfoHeader.width % 4 != 0 || abs(bmpInfoHeader.height) % 4 != 0) {
         fprintf(stderr, "Warning: Image width (%d) or height (%d) is not a multiple of 4, as expected by problem constraints for simplified padding.\n", bmpInfoHeader.width, abs(bmpInfoHeader.height));
    }

    t_bmp24 *img = bmp24_allocate(bmpInfoHeader.width, bmpInfoHeader.height, bmpInfoHeader.bits);
    if (!img) {
        fclose(file);
        return NULL;
    }

    img->header = bmpHeader; // Copy loaded main header
    img->header_info = bmpInfoHeader;

    bmp24_readPixelData(img, file);

    fclose(file);
    return img;
}

void bmp24_saveImage(t_bmp24 *img, const char *filename) {
    if (!img) {
        fprintf(stderr, "Error: Cannot save NULL image.\n");
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Error: Cannot open file for writing");
        return;
    }

    // Ensure headers are correctly set before writing
    img->header.type = BMP_TYPE;
    img->header.offset = sizeof(t_bmp_header) + sizeof(t_bmp_info);

    img->header_info.size = sizeof(t_bmp_info);
    img->header_info.width = img->width;
    img->header_info.height = img->height;
    img->header_info.planes = 1;
    img->header_info.bits = (uint16_t)img->colorDepth;
    img->header_info.compression = 0;

    uint32_t bytes_per_pixel = img->header_info.bits / 8;
    uint32_t row_pitch = ((uint32_t)img->width * bytes_per_pixel + 3u) & ~3u;
    img->header_info.imagesize = row_pitch * (uint32_t)img->height;
    img->header.size = img->header.offset + img->header_info.imagesize;

    img->header_info.xresolution = 0;
    img->header_info.yresolution = 0;
    img->header_info.ncolors = 0;
    img->header_info.importantcolors = 0;

    if (fwrite(&img->header, sizeof(t_bmp_header), 1, file) != 1) {
        fprintf(stderr, "Error writing BMP file header.\n"); fclose(file); return;
    }
    if (fwrite(&img->header_info, sizeof(t_bmp_info), 1, file) != 1) {
        fprintf(stderr, "Error writing BMP info header.\n"); fclose(file); return;
    }

    bmp24_writePixelData(img, file);

    fclose(file);
}

void bmp24_negative(t_bmp24 *img) {
    if (!img || !img->data) return;
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            img->data[y][x].red = 255 - img->data[y][x].red;
            img->data[y][x].green = 255 - img->data[y][x].green;
            img->data[y][x].blue = 255 - img->data[y][x].blue;
        }
    }
}

void bmp24_grayscale(t_bmp24 *img) {
    if (!img || !img->data) return;
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            uint8_t r = img->data[y][x].red;
            uint8_t g = img->data[y][x].green;
            uint8_t b = img->data[y][x].blue;
            uint8_t gray = (uint8_t)roundf(((float)r + (float)g + (float)b) / 3.0f);
            img->data[y][x].red = gray;
            img->data[y][x].green = gray;
            img->data[y][x].blue = gray;
        }
    }
}

void bmp24_brightness(t_bmp24 *img, int value) {
    if (!img || !img->data) return;
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int r = img->data[y][x].red + value;
            int g = img->data[y][x].green + value;
            int b = img->data[y][x].blue + value;

            img->data[y][x].red = (r < 0) ? 0 : (r > 255) ? 255 : (uint8_t)r;
            img->data[y][x].green = (g < 0) ? 0 : (g > 255) ? 255 : (uint8_t)g;
            img->data[y][x].blue = (b < 0) ? 0 : (b > 255) ? 255 : (uint8_t)b;
        }
    }
}

t_pixel bmp24_convolution(t_bmp24 *img, int cx, int cy, float **kernel, int kernelSize) {
    t_pixel new_pixel = {0, 0, 0};
    if (!img || !img->data || !kernel) {
        fprintf(stderr, "Error: NULL pointer passed to bmp24_convolution.\n");
        if (img && img->data && cx >=0 && cx < img->width && cy >=0 && cy < img->height) return img->data[cy][cx]; // Return original if possible
        return new_pixel;
    }

    float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
    int n = kernelSize / 2;

    for (int i = -n; i <= n; i++) {
        for (int j = -n; j <= n; j++) {
            // Image pixel coordinates
            int pixel_y = cy - i;
            int pixel_x = cx - j;

            if (pixel_y < 0) pixel_y = 0;
            if (pixel_y >= img->height) pixel_y = img->height - 1;
            if (pixel_x < 0) pixel_x = 0;
            if (pixel_x >= img->width) pixel_x = img->width - 1;

            float kernel_val = kernel[i + n][j + n];

            sum_r += (float)img->data[pixel_y][pixel_x].red * kernel_val;
            sum_g += (float)img->data[pixel_y][pixel_x].green * kernel_val;
            sum_b += (float)img->data[pixel_y][pixel_x].blue * kernel_val;
        }
    }

    new_pixel.red = float_to_uint8_clamp(sum_r);
    new_pixel.green = float_to_uint8_clamp(sum_g);
    new_pixel.blue = float_to_uint8_clamp(sum_b);

    return new_pixel;
}

typedef struct { float y, u, v; } t_yuv_pixel;

void bmp24_equalize(t_bmp24 *img) {
    if (!img || !img->data) return;

    int width = img->width;
    int height = img->height;

    t_yuv_pixel **yuv_data = (t_yuv_pixel **)malloc(height * sizeof(t_yuv_pixel *));
    if (!yuv_data) { fprintf(stderr, "Error: Failed to allocate YUV data rows.\n"); return; }
    for (int i = 0; i < height; i++) {
        yuv_data[i] = (t_yuv_pixel *)malloc(width * sizeof(t_yuv_pixel));
        if (!yuv_data[i]) {
            fprintf(stderr, "Error: Failed to allocate YUV data row %d.\n", i);
            for (int k = 0; k < i; k++) free(yuv_data[k]);
            free(yuv_data);
            return;
        }
    }

    for (int r_idx = 0; r_idx < height; r_idx++) {
        for (int c_idx = 0; c_idx < width; c_idx++) {
            float R = (float)img->data[r_idx][c_idx].red;
            float G = (float)img->data[r_idx][c_idx].green;
            float B = (float)img->data[r_idx][c_idx].blue;
            yuv_data[r_idx][c_idx].y = 0.299f * R + 0.587f * G + 0.114f * B;
            yuv_data[r_idx][c_idx].u = -0.14713f * R - 0.28886f * G + 0.436f * B;
            yuv_data[r_idx][c_idx].v = 0.615f * R - 0.51499f * G - 0.10001f * B;
        }
    }

    unsigned int y_histogram[256] = {0};
    for (int r_idx = 0; r_idx < height; r_idx++) {
        for (int c_idx = 0; c_idx < width; c_idx++) {
            y_histogram[float_to_uint8_clamp(yuv_data[r_idx][c_idx].y)]++;
        }
    }

    unsigned int y_cdf[256] = {0};
    y_cdf[0] = y_histogram[0];
    for (int i = 1; i < 256; i++) {
        y_cdf[i] = y_cdf[i - 1] + y_histogram[i];
    }

    unsigned int cdf_min = 0;
    for (int i = 0; i < 256; i++) {
        if (y_cdf[i] > 0) { cdf_min = y_cdf[i]; break; }
    }

    uint8_t y_equalized_map[256];
    unsigned long total_pixels = (unsigned long)width * height;
    float N_minus_cdf_min = (float)(total_pixels - cdf_min);
    if (N_minus_cdf_min < 1.0f) N_minus_cdf_min = 1.0f; // Avoid division by zero or issues if N=cdf_min

    for (int i = 0; i < 256; i++) {
        float mapped_val = roundf(((float)(y_cdf[i] - cdf_min) / N_minus_cdf_min) * 255.0f);
        y_equalized_map[i] = float_to_uint8_clamp(mapped_val);
    }

    for (int r_idx = 0; r_idx < height; r_idx++) {
        for (int c_idx = 0; c_idx < width; c_idx++) {
            yuv_data[r_idx][c_idx].y = (float)y_equalized_map[float_to_uint8_clamp(yuv_data[r_idx][c_idx].y)];
        }
    }

    for (int r_idx = 0; r_idx < height; r_idx++) {
        for (int c_idx = 0; c_idx < width; c_idx++) {
            float Y_eq = yuv_data[r_idx][c_idx].y;
            float U = yuv_data[r_idx][c_idx].u;
            float V = yuv_data[r_idx][c_idx].v;
            img->data[r_idx][c_idx].red = float_to_uint8_clamp(Y_eq + 1.13983f * V);
            img->data[r_idx][c_idx].green = float_to_uint8_clamp(Y_eq - 0.39465f * U - 0.58060f * V);
            img->data[r_idx][c_idx].blue = float_to_uint8_clamp(Y_eq + 2.03211f * U);
        }
    }

    for (int i = 0; i < height; i++) free(yuv_data[i]);
    free(yuv_data);
}