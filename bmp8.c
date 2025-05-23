#include "bmp8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For memcpy
#include <math.h>   // For roundf

t_bmp8 *bmp8_loadImage(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file for reading");
        return NULL;
    }

    t_bmp8 *img = (t_bmp8 *)malloc(sizeof(t_bmp8));
    if (!img) {
        perror("Failed to allocate memory for t_bmp8");
        fclose(fp);
        return NULL;
    }

    if (fread(img->header, sizeof(unsigned char), 54, fp) != 54) {
        fprintf(stderr, "Error reading BMP header.\n");
        free(img);
        fclose(fp);
        return NULL;
    }

    img->width = *(unsigned int *)&(img->header[18]);
    img->height = *(unsigned int *)&(img->header[22]);
    img->colorDepth = *(unsigned short *)&(img->header[28]);
    img->dataSize = *(unsigned int *)&(img->header[34]);

    if (img->dataSize == 0) { // Calculate if not present (common for uncompressed)
        img->dataSize = img->width * img->height; // For 8-bit, 1 byte per pixel
    }


    if (img->colorDepth != 8) {
        fprintf(stderr, "Image is not 8-bit grayscale. Color depth: %u\n", img->colorDepth);
        free(img);
        fclose(fp);
        return NULL;
    }

    if (fread(img->colorTable, sizeof(unsigned char), 1024, fp) != 1024) {
        fprintf(stderr, "Error reading BMP color table.\n");
        free(img);
        fclose(fp);
        return NULL;
    }

    img->data = (unsigned char *)malloc(img->dataSize);
    if (!img->data) {
        perror("Failed to allocate memory for image data");
        free(img);
        fclose(fp);
        return NULL;
    }

    // Seek to the start of pixel data if necessary (bfOffBits)
    unsigned int pixel_data_offset = *(unsigned int *)&(img->header[10]);
    fseek(fp, pixel_data_offset, SEEK_SET);

    if (fread(img->data, sizeof(unsigned char), img->dataSize, fp) != img->dataSize) {
        fprintf(stderr, "Error reading BMP pixel data.\n");
        free(img->data);
        free(img);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return img;
}

void bmp8_saveImage(const char *filename, t_bmp8 *img) {
    if (!img || !img->data) {
        fprintf(stderr, "Invalid image data for saving.\n");
        return;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error opening file for writing");
        return;
    }

    if (fwrite(img->header, sizeof(unsigned char), 54, fp) != 54) {
        fprintf(stderr, "Error writing BMP header.\n");
        fclose(fp);
        return;
    }

    if (fwrite(img->colorTable, sizeof(unsigned char), 1024, fp) != 1024) {
        fprintf(stderr, "Error writing BMP color table.\n");
        fclose(fp);
        return;
    }

    // Seek to the start of pixel data if necessary (bfOffBits)
    // For saving, we assume the header's bfOffBits is correct (54 + 1024)
    // or that the data immediately follows the color table.
    // If bfOffBits was different, we'd fseek here.
    // For simplicity, assuming data follows color table.

    if (fwrite(img->data, sizeof(unsigned char), img->dataSize, fp) != img->dataSize) {
        fprintf(stderr, "Error writing BMP pixel data.\n");
        fclose(fp);
        return;
    }

    fclose(fp);
}

void bmp8_free(t_bmp8 *img) {
    if (img) {
        if (img->data) {
            free(img->data);
            img->data = NULL;
        }
        free(img);
    }
}

void bmp8_printInfo(t_bmp8 *img) {
    if (!img) {
        printf("Image is NULL.\n");
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
    for (unsigned int i = 0; i < img->dataSize; ++i) {
        img->data[i] = 255 - img->data[i];
    }
}

void bmp8_brightness(t_bmp8 *img, int value) {
    if (!img || !img->data) return;
    for (unsigned int i = 0; i < img->dataSize; ++i) {
        int new_val = img->data[i] + value;
        if (new_val < 0) new_val = 0;
        if (new_val > 255) new_val = 255;
        img->data[i] = (unsigned char)new_val;
    }
}

void bmp8_threshold(t_bmp8 *img, int threshold_val) {
    if (!img || !img->data) return;
    for (unsigned int i = 0; i < img->dataSize; ++i) {
        if (img->data[i] >= threshold_val) {
            img->data[i] = 255;
        } else {
            img->data[i] = 0;
        }
    }
}

void bmp8_applyFilter(t_bmp8 *img, float **kernel, int kernelSize) {
    if (!img || !img->data || !kernel || kernelSize % 2 == 0 || kernelSize < 1) {
        return;
    }

    unsigned char *temp_data = (unsigned char *)malloc(img->dataSize);
    if (!temp_data) {
        perror("Failed to allocate memory for temp_data in applyFilter");
        return;
    }
    memcpy(temp_data, img->data, img->dataSize);

    int n = kernelSize / 2; // Kernel center offset

    for (unsigned int y_out = n; y_out < img->height - n; ++y_out) {
        for (unsigned int x_out = n; x_out < img->width - n; ++x_out) {
            float sum = 0.0f;
            for (int ky_offset = -n; ky_offset <= n; ++ky_offset) { // Kernel row offset from center
                for (int kx_offset = -n; kx_offset <= n; ++kx_offset) { // Kernel col offset from center
                    unsigned int read_y = y_out - ky_offset;
                    unsigned int read_x = x_out - kx_offset;

                    sum += (float)temp_data[read_y * img->width + read_x] * kernel[ky_offset + n][kx_offset + n];
                }
            }
            if (sum < 0.0f) sum = 0.0f;
            if (sum > 255.0f) sum = 255.0f;
            img->data[y_out * img->width + x_out] = (unsigned char)roundf(sum);
        }
    }
    free(temp_data);
}