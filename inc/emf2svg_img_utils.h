#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() macro */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct _dibImg {
    uint8_t *img;
    size_t size;
    uint32_t width;
    uint32_t height;
} dibImg;

#define RLE_MARK 0x00
#define RLE_EOL 0x00
#define RLE_EOB 0x01
#define RLE_DELTA 0x02

/* Pixels in this bitmap structure are stored as BGR. */
typedef struct _RGBPixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} RGBPixel;

/* Structure for containing decompressed bitmaps. */
typedef struct _RGBBitmap {
    RGBPixel *pixels;
    size_t width;
    size_t height;
    size_t bytewidth;
    uint8_t bytes_per_pixel;
} RGBBitmap;

/* Returns pixel of bitmap at given point. */
#define RGBPixelAtPoint(image, x, y)                                           \
    *(((image)->pixels) +                                                      \
      (((image)->bytewidth * (y)) + ((x) * (image)->bytes_per_pixel)))

int rgb2png(RGBBitmap *bitmap, char **out, size_t *size);
dibImg rle8ToBitmap(dibImg img);
dibImg rle4ToBitmap(dibImg img);
