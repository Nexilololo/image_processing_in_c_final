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


// --- Histogram Equalization ---
typedef struct { float y, u, v; } t_yuv_float;

void bmp24_equalize(t_bmp24 *img) {
    if (!img || !img->data) return;

    t_yuv_float **yuv_data = (t_yuv_float **)malloc(img->height * sizeof(t_yuv_float *));
    if (!yuv_data) return;
    for(int i=0; i<img->height; ++i) {
        yuv_data[i] = (t_yuv_float *)malloc(img->width * sizeof(t_yuv_float));
        if(!yuv_data[i]) {
            for(int k=0; k<i; ++k) free(yuv_data[k]);
            free(yuv_data);
            return;
        }
    }

    unsigned int y_histogram[256] = {0};

    for (int r = 0; r < img->height; ++r) {
        for (int c = 0; c < img->width; ++c) {
            float R = img->data[r][c].red;
            float G = img->data[r][c].green;
            float B = img->data[r][c].blue;

            yuv_data[r][c].y = 0.299f * R + 0.587f * G + 0.114f * B;
            yuv_data[r][c].u = -0.14713f * R - 0.28886f * G + 0.436f * B;
            yuv_data[r][c].v = 0.615f * R - 0.51499f * G - 0.10001f * B;

            uint8_t y_int = (uint8_t)roundf(yuv_data[r][c].y);
            if (y_int > 255) y_int = 255; // Should not happen if R,G,B are 0-255
            if (y_int < 0) y_int = 0;     // Should not happen
            y_histogram[y_int]++;
        }
    }

    unsigned int y_cdf[256] = {0};
    y_cdf[0] = y_histogram[0];
    for (int i = 1; i < 256; ++i) {
        y_cdf[i] = y_cdf[i - 1] + y_histogram[i];
    }

    unsigned int cdf_min = 0;
    for (int i = 0; i < 256; ++i) {
        if (y_cdf[i] > 0) {
            cdf_min = y_cdf[i]; // Smallest non-zero CDF value (actually, smallest hist value for cdf_min in formula)
                                // The PDF formula uses cdf_min from the CDF array.
                                // Let's find the first non-zero cdf value.
            break;
        }
    }
    // The PDF formula for hist_eq[i] uses cdf_min as the smallest non-zero value of the *cumulative* histogram.
    // If all pixels are black, cdf_min could be 0 if we take hist[0].
    // Let's find the first cdf[k] > 0.
    // If hist[0] is the only non-zero, cdf_min = hist[0].
    // The formula is (cdf[i] - cdf_min) / (N - cdf_min) * 255
    // N = total pixels = img->width * img->height
    // cdf_min should be the cdf value of the first gray level that has pixels.
    // If gray level 0 has pixels, cdf_min = cdf[0] = hist[0].
    // If gray level g is the first with pixels, cdf_min = cdf[g] = hist[g].
    // The PDF example for 8-bit shows hist_eq[52] = 0, where 52 is the first gray level.
    // So cdf_min is cdf[first_gray_level_with_pixels].

    int first_gray_level = -1;
    for(int i=0; i<256; ++i) {
        if(y_histogram[i] > 0) {
            first_gray_level = i;
            break;
        }
    }
    if(first_gray_level == -1) { // All black or empty image?
         for(int i=0; i<img->height; ++i) free(yuv_data[i]);
         free(yuv_data);
         return;
    }
    cdf_min = y_cdf[first_gray_level];


    uint8_t y_eq_map[256];
    unsigned int total_pixels = (unsigned int)img->width * img->height;

    for (int i = 0; i < 256; ++i) {
        if (y_cdf[i] == 0) { // No pixels at or below this Y value (should not happen if first_gray_level logic is correct for i >= first_gray_level)
            y_eq_map[i] = 0; // Or map to i if it's before first_gray_level
            if (i < first_gray_level) y_eq_map[i] = i; // Untouched
            else y_eq_map[i] = 0; // Should be covered by cdf_min logic
        } else {
            // Denominator (N - cdf_min) can be 0 if all pixels have the same first_gray_level intensity
            // and N = cdf_min. In this case, map to 0 or 255.
            float denominator = (float)(total_pixels - cdf_min);
            if (denominator < 1e-6) { // Effectively zero or N == cdf_min
                 y_eq_map[i] = (i >= first_gray_level) ? 255 : i; // Stretch if it's the only color
            } else {
                float val = roundf(((float)(y_cdf[i] - cdf_min) / denominator) * 255.0f);
                y_eq_map[i] = (val < 0) ? 0 : ((val > 255) ? 255 : (uint8_t)val);
            }
        }
    }
     if (total_pixels == cdf_min && first_gray_level != -1) { // All pixels have the same intensity as first_gray_level
        for(int i=0; i<256; ++i) {
            y_eq_map[i] = (i == first_gray_level) ? 255 : i; // Map this intensity to 255, others unchanged (or map all to 255)
                                                            // Or map first_gray_level to 0 if it's the only one.
                                                            // A single color image should remain single color.
                                                            // The formula would make it 255.
        }
         y_eq_map[first_gray_level] = 255; // Or 0, depending on desired behavior for single-color.
                                          // Standard behavior is to stretch the range.
                                          // If cdf[i] == cdf_min, then 0. If cdf[i] == N, then 255.
                                          // If N == cdf_min, then all pixels are of first_gray_level.
                                          // cdf[first_gray_level] = cdf_min. So (cdf_min - cdf_min) / (N-cdf_min) = 0.
                                          // This means the single color maps to 0. This is standard.
        for(int i=0; i<256; ++i) {
            if (y_histogram[i] > 0) y_eq_map[i] = 0; // All existing colors map to 0
        }
        if (y_histogram[255] == total_pixels) y_eq_map[255] = 255; // If all white, stays white.
                                                                // If all one color, it maps to 0.
    }


    for (int r = 0; r < img->height; ++r) {
        for (int c = 0; c < img->width; ++c) {
            uint8_t original_y_int = (uint8_t)roundf(yuv_data[r][c].y);
            if (original_y_int > 255) original_y_int = 255;

            float new_Y = y_eq_map[original_y_int];
            float U = yuv_data[r][c].u;
            float V = yuv_data[r][c].v;

            float R_new = new_Y + 1.13983f * V;
            float G_new = new_Y - 0.39465f * U - 0.58060f * V;
            float B_new = new_Y + 2.03211f * U;

            img->data[r][c].red = (R_new < 0) ? 0 : ((R_new > 255) ? 255 : (uint8_t)roundf(R_new));
            img->data[r][c].green = (G_new < 0) ? 0 : ((G_new > 255) ? 255 : (uint8_t)roundf(G_new));
            img->data[r][c].blue = (B_new < 0) ? 0 : ((B_new > 255) ? 255 : (uint8_t)roundf(B_new));
        }
    }

    for(int i=0; i<img->height; ++i) free(yuv_data[i]);
    free(yuv_data);
}