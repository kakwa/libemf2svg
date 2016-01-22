#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() macro */
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "emf2svg_img_utils.h"

// uncompress RLE8 to get bitmap (section 3.1.6.2 [MS-WMF].pdf)
dibImg rle8ToBitmap(dibImg img) {
    FILE *stream;
    bool decode = true;
    char *out;
    size_t size;

    dibImg out_img;
    out_img.size = 0;
    out_img.width = 0;
    out_img.height = 0;
    out_img.img = NULL;

    uint8_t *bm = (uint8_t *)img.img;
    uint32_t x = 0;
    uint32_t y = img.height - 1;
    uint8_t *bm_next;

    stream = open_memstream(&out, &size);
    if (stream == NULL) {
        return out_img;
    }

    uint8_t *end = bm + img.size;
    while (decode && (bm < end)) {
        switch (bm[0]) {
            // check against potential overflow
            if ((bm + 2) > end) {
                fclose(stream);
				free(out);
                return out_img;
            };
        case RLE_MARK:
            switch (bm[1]) {
            case RLE_EOL:
                // end of line, pad the rest of the line with zeros
                for (int i = 0; i < (img.width - x); i++)
                    fputc(0x00, stream);
                bm += 2;
                x = 0;
                y--;
                break;
            case RLE_EOB:
                // end of bitmap
                decode = false;
                break;
            case RLE_DELTA:
                // offset handling, pad with (off.x + off.y * width) zeros
                if ((bm + 3) > end) {
                    fclose(stream);
					free(out);
                    return out_img;
                };
                for (int i = 0; i < (bm[2] + img.width * bm[3]); i++)
                    fputc(0x00, stream);
                x += bm[2];
                y -= bm[3];
                bm += 4;
                break;
            default:
                // absolute mode handling (no compression)

                // data is padded to a word
                // calculate next address accordingly
                bm_next = bm + 1 + ((bm[1] + 1) / 2) * 2;
                if (bm_next > end) {
                    fclose(stream);
					free(out);
                    return out_img;
                };
                for (int i = 2; i < bm[1] + 2; i++)
                    fputc(bm[i], stream);
                x += bm[1];
                bm = bm_next + 1;
                break;
            }
            break;
        default:
            for (int i = 0; i < bm[0]; i++)
                fputc(bm[1], stream);
            x += bm[0];
            bm += 2;
            if (x >= img.width) {
                x = x % img.width;
                y--;
            }
            break;
        }
    }
    // pad the rest of the bitmap
    for (int i = 0; i < ((img.width - x) + img.width * y); i++)
        fputc(0x00, stream);

    fflush(stream);
    fclose(stream);
    out_img.img = (uint8_t *)out;
    out_img.size = size;
    out_img.width = img.width;
    out_img.height = img.height;
    return out_img;
}

// uncompress RLE4 to get bitmap (section 3.1.6.2 [MS-WMF].pdf)
// FIXME (probably) (handling 4 bits stuff is kind of messy...)
dibImg rle4ToBitmap(dibImg img) {
    FILE *stream;
    bool decode = true;
    char *out;
    size_t size;

    dibImg out_img;
    out_img.size = 0;
    out_img.width = 0;
    out_img.height = 0;
    out_img.img = NULL;

    uint8_t *bm = (uint8_t *)img.img;
    uint32_t x = 0;
    uint32_t y = img.height - 1;
    uint8_t *bm_next;

    stream = open_memstream(&out, &size);
    if (stream == NULL) {
        return out_img;
    }

    // upper 4 bits of the stuff to write
    uint8_t upper = 0x00;
    // lower 4 bits of the stuff to write
    uint8_t lower = 0x00;
    // Is the current position on upper 4 bits?
    // If it's the case, swap upper and lower of what we read
    // and eventualy print the upper part of previous record
    bool odd = false;

    uint8_t tmp_u;
    uint8_t tmp_l;

    uint8_t *end = bm + img.size;
    while (decode && (bm < end)) {
        switch (bm[0]) {
            // check against potential overflow
            if ((bm + 2) > end) {
                fclose(stream);
				free(out);
                return out_img;
            };
        case RLE_MARK:
            switch (bm[1]) {
            case RLE_EOL:
                if (odd) {
                    fputc(upper | 0x00, stream);
                    upper = 0x00;
                    lower = 0x00;
                }
                // end of line, pad the rest of the line with zeros
                for (int i = 0; i < (((img.width - x) / 2) - 1); i++)
                    fputc(0x00, stream);
                odd = img.width % 2;
                bm += 2;
                x = 0;
                y--;
                break;
            case RLE_EOB:
                // end of bitmap
                decode = false;
                break;
            case RLE_DELTA:
                // offset handling, pad with (off.x + off.y * width) zeros
                if ((bm + 3) > end) {
                    fclose(stream);
					free(out);
                    return out_img;
                };

                tmp_u = 0x00;
                tmp_l = 0x00;
                if (odd) {
                    fputc(upper | tmp_l, stream);
                }
                upper = tmp_u;
                lower = tmp_l;

                for (int i = 0; i < ((bm[2] + img.width * bm[3]) / 2); i++)
                    fputc(0x00, stream);

                odd = ((bm[2] + img.width * bm[3]) + odd) % 2;

                x += bm[2];
                y -= bm[3];
                bm += 4;
                break;
            default:
                // absolute mode handling (no compression)

                // data is padded to a word
                // calculate next address accordingly
                bm_next = bm + (bm[1] / 2) + 2;
                if (bm_next > end) {
                    fclose(stream);
					free(out);
                    return out_img;
                };
                for (int i = 2; i < (bm[1] / 2) + 2; i++) {
                    if (!odd) {
                        tmp_u = bm[i] & 0xF0;
                        tmp_l = bm[i] & 0x0F;
                        fputc(tmp_u | tmp_l, stream);
                    } else {
                        tmp_u = (bm[i] & 0x0F) << 4;
                        tmp_l = (bm[i] & 0xF0) >> 4;
                        fputc(upper | tmp_l, stream);
                    }
                    upper = tmp_u;
                    lower = tmp_l;
                }
                x += bm[1];
                bm = bm_next + 1;
                odd = (bm[1] + odd) % 2;

                if (odd) {
                    upper = (bm[0] & 0x0F) << 4;
                    lower = (bm[0] & 0xF0) >> 4;
                    bm += 1;
                }
                break;
            }
            break;
        default:
            if (!odd) {
                tmp_u = bm[1] & 0xF0;
                tmp_l = bm[1] & 0x0F;
            } else {
                tmp_u = (bm[1] & 0x0F) << 4;
                tmp_l = (bm[1] & 0xF0) >> 4;
                fputc(upper | tmp_l, stream);
            }
            upper = tmp_u;
            lower = tmp_l;

            for (int i = 0; i < (bm[0] / 2); i++) {
                fputc(upper | lower, stream);
            }
            odd = (bm[0] + odd) % 2;
            x += bm[0];
            bm += 2;
            if (x >= img.width) {
                x = x % img.width;
                y--;
            }
            break;
        }
    }
    // pad the rest of the bitmap
    if (odd) {
        fputc(upper | 0x00, stream);
        upper = 0x00;
        lower = 0x00;
    }
    // end of line, pad the rest of the line with zeros
    for (int i = 0; i < ((img.width - x + img.width * y) / 2); i++)
        fputc(0x00, stream);

    fflush(stream);
    fclose(stream);
    out_img.img = (uint8_t *)out;
    out_img.size = size;
    out_img.width = img.width;
    out_img.height = img.height;
    return out_img;
}
