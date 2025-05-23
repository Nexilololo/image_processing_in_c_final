#ifndef BMP24_H
#define BMP24_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define BITMAP_MAGIC        0x00
#define BITMAP_SIZE         0x02
#define BITMAP_OFFSET       0x0A
#define BITMAP_WIDTH        0x12
#define BITMAP_HEIGHT       0x16
#define BITMAP_DEPTH        0x1C
#define BITMAP_SIZE_RAW     0x22

#define BMP_TYPE            0x4D42

#define HEADER_SIZE         0x0E
#define INFO_SIZE           0x28

#define DEFAULT_DEPTH       0x18

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} t_pixel;

typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} t_bmp_header;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t imagesize;
    int32_t xresolution;
    int32_t yresolution;
    uint32_t ncolors;
    uint32_t importantcolors;
} t_bmp_info;

typedef struct {
    t_bmp_header header;
    t_bmp_info header_info;
    int width;
    int height;
    int colorDepth;
    t_pixel **data;
} t_bmp24;

t_pixel **bmp24_allocateDataPixels(int width, int height);
void bmp24_freeDataPixels(t_pixel **pixels, int height);
t_bmp24 *bmp24_allocate(int width, int height, int colorDepth);
void bmp24_free(t_bmp24 *img);

t_bmp24 *bmp24_loadImage(const char *filename);
void bmp24_saveImage(t_bmp24 *img, const char *filename);

void file_rawRead(uint32_t position, void *buffer, uint32_t size, size_t n, FILE *file);
void file_rawWrite(uint32_t position, void *buffer, uint32_t size, size_t n, FILE *file);

void bmp24_readPixelValue(t_bmp24 *image, int x, int y, FILE *file);
void bmp24_readPixelData(t_bmp24 *image, FILE *file);

void bmp24_writePixelValue(t_bmp24 *image, int x, int y, FILE *file);
void bmp24_writePixelData(t_bmp24 *image, FILE *file);

void bmp24_negative(t_bmp24 *img);
void bmp24_grayscale(t_bmp24 *img);
void bmp24_brightness(t_bmp24 *img, int value);

t_pixel bmp24_convolution(t_bmp24 *img, int x, int y, float **kernel, int kernelSize);
void bmp24_boxBlur(t_bmp24 *img);
void bmp24_gaussianBlur(t_bmp24 *img);
void bmp24_outline(t_bmp24 *img);
void bmp24_emboss(t_bmp24 *img);
void bmp24_sharpen(t_bmp24 *img);

#endif // BMP24_H