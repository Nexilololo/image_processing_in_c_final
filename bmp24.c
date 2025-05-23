#include "bmp24.h"
#include <stdlib.h>
#include <string.h> // For memcpy
#include <math.h>   // For round, roundf

// --- Allocation and Deallocation Functions ---

t_pixel **bmp24_allocateDataPixels(int width, int height) {
    if (width <= 0 || height <= 0) return NULL;
    t_pixel **pixels = (t_pixel **)malloc(height * sizeof(t_pixel *));
    if (!pixels) {
        perror("Failed to allocate memory for pixel rows");
        return NULL;
    }
    for (int i = 0; i < height; ++i) {
        pixels[i] = (t_pixel *)malloc(width * sizeof(t_pixel));
        if (!pixels[i]) {
            perror("Failed to allocate memory for a pixel row");
            for (int j = 0; j < i; ++j) free(pixels[j]);
            free(pixels);
            return NULL;
        }
    }
    return pixels;
}

void bmp24_freeDataPixels(t_pixel **pixels, int height) {
    if (!pixels) return;
    for (int i = 0; i < height; ++i) {
        free(pixels[i]);
    }
    free(pixels);
}

t_bmp24 *bmp24_allocate(int width, int height, int colorDepth) {
    if (width <= 0 || height <= 0) return NULL;
    t_bmp24 *img = (t_bmp24 *)malloc(sizeof(t_bmp24));
    if (!img) {
        perror("Failed to allocate memory for t_bmp24");
        return NULL;
    }
    img->data = bmp24_allocateDataPixels(width, height);
    if (!img->data) {
        free(img);
        return NULL;
    }
    img->width = width;
    img->height = height;
    img->colorDepth = colorDepth;
    // header and header_info should be initialized by loadImage or manually
    return img;
}

void bmp24_free(t_bmp24 *img) {
    if (img) {
        bmp24_freeDataPixels(img->data, img->height);
        img->data = NULL;
        free(img);
    }
}

// --- Helper Read/Write Functions ---

void file_rawRead(uint32_t position, void *buffer, uint32_t size_elem, size_t n_elem, FILE *file) {
    if (!file || !buffer) return;
    fseek(file, position, SEEK_SET);
    fread(buffer, size_elem, n_elem, file);
}

void file_rawWrite(uint32_t position, void *buffer, uint32_t size_elem, size_t n_elem, FILE *file) {
    if (!file || !buffer) return;
    fseek(file, position, SEEK_SET);
    fwrite(buffer, size_elem, n_elem, file);
}

// --- Pixel Data Read/Write ---
// Assuming padding is 0 for Part 2 as per PDF note: "ensure that the width and height of the image are multiples of 4"
// If width is a multiple of 4, then width*3 is a multiple of 4, so padding is 0.

void bmp24_readPixelValue(t_bmp24 *image, int x, int y, FILE *file) {
    if (!image || !image->data || !file || x < 0 || x >= image->width || y < 0 || y >= image->height) return;

    uint32_t row_size_bytes = (uint32_t)image->width * 3; // Assuming no padding
    uint32_t file_y = image->height - 1 - y;
    uint32_t pixel_offset = image->header.offset + (file_y * row_size_bytes) + ((uint32_t)x * 3);

    uint8_t bgr[3];
    file_rawRead(pixel_offset, bgr, sizeof(uint8_t), 3, file);

    image->data[y][x].blue = bgr[0];
    image->data[y][x].green = bgr[1];
    image->data[y][x].red = bgr[2];
}

void bmp24_readPixelData(t_bmp24 *image, FILE *file) {
    if (!image || !image->data || !file) return;
    fseek(file, image->header.offset, SEEK_SET);

    // Assuming padding is 0 because width is a multiple of 4 (PDF Part 2 note)
    // int padding = (4 - (image->width * 3) % 4) % 4;
    // if (image->width % 4 == 0) padding = 0;

    for (int y_mem = 0; y_mem < image->height; ++y_mem) {
    }
    // More efficient sequential read:
    for (int y_file_idx = image->height - 1; y_file_idx >= 0; --y_file_idx) {
        int y_mem_idx = image->height - 1 - y_file_idx;
        for (int x_idx = 0; x_idx < image->width; ++x_idx) {
            uint8_t bgr[3];
            if (fread(bgr, sizeof(uint8_t), 3, file) != 3) {
                // Error reading
                return;
            }
            image->data[y_mem_idx][x_idx].blue = bgr[0];
            image->data[y_mem_idx][x_idx].green = bgr[1];
            image->data[y_mem_idx][x_idx].red = bgr[2];
        }
        // if (padding > 0) fseek(file, padding, SEEK_CUR); // Skip padding if any
    }
}


void bmp24_writePixelValue(t_bmp24 *image, int x, int y, FILE *file) {
    if (!image || !image->data || !file || x < 0 || x >= image->width || y < 0 || y >= image->height) return;

    uint32_t row_size_bytes = (uint32_t)image->width * 3; // Assuming no padding
    uint32_t file_y = image->height - 1 - y;
    uint32_t pixel_offset = image->header.offset + (file_y * row_size_bytes) + ((uint32_t)x * 3);

    uint8_t bgr[3];
    bgr[0] = image->data[y][x].blue;
    bgr[1] = image->data[y][x].green;
    bgr[2] = image->data[y][x].red;

    file_rawWrite(pixel_offset, bgr, sizeof(uint8_t), 3, file);
}

void bmp24_writePixelData(t_bmp24 *image, FILE *file) {
    if (!image || !image->data || !file) return;
    fseek(file, image->header.offset, SEEK_SET);

    // Assuming padding is 0
    // int padding = (4 - (image->width * 3) % 4) % 4;
    // if (image->width % 4 == 0) padding = 0;

    for (int y_file_idx = image->height - 1; y_file_idx >= 0; --y_file_idx) {
        int y_mem_idx = image->height - 1 - y_file_idx;
        for (int x_idx = 0; x_idx < image->width; ++x_idx) {
            uint8_t bgr[3];
            bgr[0] = image->data[y_mem_idx][x_idx].blue;
            bgr[1] = image->data[y_mem_idx][x_idx].green;
            bgr[2] = image->data[y_mem_idx][x_idx].red;
            if (fwrite(bgr, sizeof(uint8_t), 3, file) != 3) {
                // Error writing
                return;
            }
        }
        // if (padding > 0) {
        //     uint8_t pad_byte = 0;
        //     for(int p=0; p<padding; ++p) fwrite(&pad_byte, 1, 1, file);
        // }
    }
}

// --- Loading and Saving ---

t_bmp24 *bmp24_loadImage(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file for reading");
        return NULL;
    }

    int32_t width_val, height_val;
    uint16_t bits_val;

    file_rawRead(BITMAP_WIDTH, &width_val, sizeof(int32_t), 1, fp);
    file_rawRead(BITMAP_HEIGHT, &height_val, sizeof(int32_t), 1, fp);
    file_rawRead(BITMAP_DEPTH, &bits_val, sizeof(uint16_t), 1, fp);

    if (bits_val != 24) {
        fprintf(stderr, "Image is not 24-bit. Bits: %u\n", bits_val);
        fclose(fp);
        return NULL;
    }

    t_bmp24 *img = bmp24_allocate(width_val, height_val, bits_val);
    if (!img) {
        fclose(fp);
        return NULL;
    }

    file_rawRead(BITMAP_MAGIC, &(img->header), sizeof(t_bmp_header), 1, fp);
    file_rawRead(HEADER_SIZE, &(img->header_info), sizeof(t_bmp_info), 1, fp);

    if (img->header.type != BMP_TYPE) {
        fprintf(stderr, "Not a BMP file. Type: %x\n", img->header.type);
        bmp24_free(img);
        fclose(fp);
        return NULL;
    }
    // Ensure allocated dimensions match header info (should be redundant if read correctly)
    img->width = img->header_info.width;
    img->height = img->header_info.height;
    img->colorDepth = img->header_info.bits;


    bmp24_readPixelData(img, fp);

    fclose(fp);
    return img;
}

void bmp24_saveImage(t_bmp24 *img, const char *filename) {
    if (!img || !img->data) {
        fprintf(stderr, "Invalid image data for saving.\n");
        return;
    }
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error opening file for writing");
        return;
    }

    file_rawWrite(BITMAP_MAGIC, &(img->header), sizeof(t_bmp_header), 1, fp);
    file_rawWrite(HEADER_SIZE, &(img->header_info), sizeof(t_bmp_info), 1, fp);
    bmp24_writePixelData(img, fp);

    fclose(fp);
}

// --- Image Processing ---

void bmp24_negative(t_bmp24 *img) {
    if (!img || !img->data) return;
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            img->data[y][x].red = 255 - img->data[y][x].red;
            img->data[y][x].green = 255 - img->data[y][x].green;
            img->data[y][x].blue = 255 - img->data[y][x].blue;
        }
    }
}

void bmp24_grayscale(t_bmp24 *img) {
    if (!img || !img->data) return;
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            uint8_t r = img->data[y][x].red;
            uint8_t g = img->data[y][x].green;
            uint8_t b = img->data[y][x].blue;
            uint8_t gray = (uint8_t)roundf((float)(r + g + b) / 3.0f);
            img->data[y][x].red = gray;
            img->data[y][x].green = gray;
            img->data[y][x].blue = gray;
        }
    }
}

void bmp24_brightness(t_bmp24 *img, int value) {
    if (!img || !img->data) return;
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            int r = img->data[y][x].red + value;
            int g = img->data[y][x].green + value;
            int b = img->data[y][x].blue + value;

            img->data[y][x].red = (r < 0) ? 0 : ((r > 255) ? 255 : (uint8_t)r);
            img->data[y][x].green = (g < 0) ? 0 : ((g > 255) ? 255 : (uint8_t)g);
            img->data[y][x].blue = (b < 0) ? 0 : ((b > 255) ? 255 : (uint8_t)b);
        }
    }
}

// --- Convolution Filters ---

t_pixel bmp24_convolution(t_bmp24 *img_read_from, int x_out, int y_out, float **kernel, int kernelSize) {
    t_pixel new_pixel = {0, 0, 0};
    if (!img_read_from || !img_read_from->data || !kernel || kernelSize % 2 == 0 || kernelSize < 1) {
        return new_pixel; // Or some error indicator
    }

    float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
    int n = kernelSize / 2;

    for (int ky = -n; ky <= n; ++ky) { // Kernel row relative to center
        for (int kx = -n; kx <= n; ++kx) { // Kernel col relative to center
            int read_y = y_out - ky; // Image row to read from
            int read_x = x_out - kx; // Image col to read from

            // Boundary check (though caller loop should handle this for output pixel)
            if (read_y >= 0 && read_y < img_read_from->height && read_x >= 0 && read_x < img_read_from->width) {
                t_pixel current_pixel = img_read_from->data[read_y][read_x];
                float kernel_val = kernel[ky + n][kx + n];
                sum_r += (float)current_pixel.red * kernel_val;
                sum_g += (float)current_pixel.green * kernel_val;
                sum_b += (float)current_pixel.blue * kernel_val;
            }
        }
    }

    new_pixel.red = (sum_r < 0) ? 0 : ((sum_r > 255) ? 255 : (uint8_t)roundf(sum_r));
    new_pixel.green = (sum_g < 0) ? 0 : ((sum_g > 255) ? 255 : (uint8_t)roundf(sum_g));
    new_pixel.blue = (sum_b < 0) ? 0 : ((sum_b > 255) ? 255 : (uint8_t)roundf(sum_b));

    return new_pixel;
}

static void apply_filter_generic(t_bmp24 *img, float kernel_values[3][3], int kernelSize) {
    if (!img || !img->data) return;

    t_pixel **original_data = bmp24_allocateDataPixels(img->width, img->height);
    if (!original_data) return;

    for (int y = 0; y < img->height; ++y) {
        memcpy(original_data[y], img->data[y], img->width * sizeof(t_pixel));
    }

    t_bmp24 temp_read_img_struct;
    temp_read_img_struct.width = img->width;
    temp_read_img_struct.height = img->height;
    temp_read_img_struct.data = original_data;
    // Other fields of temp_read_img_struct are not strictly needed by bmp24_convolution

    float **kernel = (float **)malloc(kernelSize * sizeof(float *));
    for (int i = 0; i < kernelSize; ++i) {
        kernel[i] = (float *)malloc(kernelSize * sizeof(float));
        for (int j = 0; j < kernelSize; ++j) {
            kernel[i][j] = kernel_values[i][j];
        }
    }

    int n = kernelSize / 2;
    for (int y = n; y < img->height - n; ++y) {
        for (int x = n; x < img->width - n; ++x) {
            img->data[y][x] = bmp24_convolution(&temp_read_img_struct, x, y, kernel, kernelSize);
        }
    }

    for (int i = 0; i < kernelSize; ++i) free(kernel[i]);
    free(kernel);
    bmp24_freeDataPixels(original_data, img->height);
}


void bmp24_boxBlur(t_bmp24 *img) {
    float k[3][3] = {
        {1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f},
        {1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f},
        {1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f}
    };
    apply_filter_generic(img, k, 3);
}

void bmp24_gaussianBlur(t_bmp24 *img) {
    float k[3][3] = {
        {1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f},
        {2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f},
        {1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f}
    };
    apply_filter_generic(img, k, 3);
}

void bmp24_outline(t_bmp24 *img) {
     float k[3][3] = {
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  8.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f}
    };
    apply_filter_generic(img, k, 3);
}

void bmp24_emboss(t_bmp24 *img) {
    float k[3][3] = {
        {-2.0f, -1.0f,  0.0f},
        {-1.0f,  1.0f,  1.0f},
        { 0.0f,  1.0f,  2.0f}
    };
    apply_filter_generic(img, k, 3);
}

void bmp24_sharpen(t_bmp24 *img) {
    float k[3][3] = {
        { 0.0f, -1.0f,  0.0f},
        {-1.0f,  5.0f, -1.0f},
        { 0.0f, -1.0f,  0.0f}
    };
    apply_filter_generic(img, k, 3);
}
