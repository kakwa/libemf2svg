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

dibImg rle8ToBitmap(dibImg img);
dibImg rle4ToBitmap(dibImg img);
