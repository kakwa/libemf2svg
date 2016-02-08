#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() macro */
#include <string.h>
#include <math.h>
#include <png.h>
#include "uemf.h"
#include "emf2svg.h"
#include "emf2svg_private.h"
#include "emf2svg_img_utils.h"
#include "emf2svg_print.h"
#include "pmf2svg.h"
#include "pmf2svg_print.h"

void U_EMRALPHABLEND_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRALPHABLEND_print(contents, states);
    }

    PU_EMRALPHABLEND pEmr = (PU_EMRALPHABLEND)(contents);

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);

    float alpha = (float)pEmr->Blend.Global / 255.0;
    fprintf(out, " fill-opacity=\"%.4f\" ", alpha);
    clipset_draw(states, out);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc);
    fprintf(out, "/>\n");
}
void U_EMRBITBLT_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRBITBLT_print(contents, states);
    }
    PU_EMRBITBLT pEmr = (PU_EMRBITBLT)(contents);

    // FIXME
    // if no bitmap, return now.
    // Should fill the output with the current brush and the raster operation
    // specified
    if (pEmr->cbBitsSrc == 0) {
        return;
    }

    // FIXME doesn't handle ternary raster operation

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);
    clipset_draw(states, out);

    // float alpha = (float)pEmr->Blend.Global / 255.0;
    // fprintf(out, " fill-opacity=\"%.4f\" ", alpha);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc);
    fprintf(out, "/>\n");
}
void U_EMRMASKBLT_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRMASKBLT_print(contents, states);
    }
    // PU_EMRMASKBLT pEmr = (PU_EMRMASKBLT) (contents);
}
void U_EMRPLGBLT_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRPLGBLT_print(contents, states);
    }
    // PU_EMRPLGBLT pEmr = (PU_EMRPLGBLT) (contents);
}
void U_EMRSETDIBITSTODEVICE_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSETDIBITSTODEVICE_print(contents, states);
    }
    // PU_EMRSETDIBITSTODEVICE pEmr = (PU_EMRSETDIBITSTODEVICE) (contents);
}
void U_EMRSTRETCHBLT_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSTRETCHBLT_print(contents, states);
    }
    PU_EMRSTRETCHBLT pEmr = (PU_EMRSTRETCHBLT)(contents);
    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);
    clipset_draw(states, out);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc);
    fprintf(out, "/>\n");
}
void U_EMRSTRETCHDIBITS_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSTRETCHDIBITS_print(contents, states);
    }
    PU_EMRSTRETCHDIBITS pEmr = (PU_EMRSTRETCHDIBITS)(contents);

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);
    clipset_draw(states, out);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc);
    fprintf(out, "/>\n");
}
void U_EMRTRANSPARENTBLT_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRTRANSPARENTBLT_print(contents, states);
    }
}

void dib_img_writer(const char *contents, FILE *out, drawingStates *states,
                    PU_BITMAPINFOHEADER BmiSrc, const unsigned char *BmpSrc,
                    size_t size) {
    char *b64Bmp = NULL;
    size_t b64s;
    char *tmp = NULL;

    // Handle simple cases first, no treatment needed for them
    switch (BmiSrc->biCompression) {
    case U_BI_JPEG:
        b64Bmp = base64_encode(BmpSrc, size, &b64s);
        fprintf(out, "xlink:href=\"data:image/jpg;base64,");
        break;
    case U_BI_PNG:
        b64Bmp = base64_encode(BmpSrc, size, &b64s);
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    }
    if (b64Bmp != NULL) {
        fprintf(out, "%s\" ", b64Bmp);
        free(b64Bmp);
        return;
    }

    // more complexe treatment, with conversion to png
    RGBBitmap convert_in;
    convert_in.size = size;
    convert_in.width = BmiSrc->biWidth;
    convert_in.height = BmiSrc->biHeight;
    convert_in.pixels = (RGBPixel *)BmpSrc;
    convert_in.bytewidth = BmiSrc->biWidth * 3;
    convert_in.bytes_per_pixel = 3;

    RGBBitmap convert_out;
    convert_out.pixels = NULL;
    const U_RGBQUAD *ct = NULL;
    uint32_t width, height, colortype, numCt, invert;
    char *rgba_px = NULL;
    int dibparams;
    char *in;
    size_t img_size;

    RGBABitmap convert_inpng;

    // In any cases after that, we get a png blob
    fprintf(out, "xlink:href=\"data:image/png;base64,");

    switch (BmiSrc->biCompression) {
    case U_BI_RLE8:
        convert_out = rle8ToRGB8(convert_in);
        break;
    case U_BI_RLE4:
        convert_out = rle4ToRGB(convert_in);
        break;
    }

    if (convert_out.pixels != NULL) {
        in = (char *)convert_out.pixels;
        img_size = convert_out.size;
    } else {
        in = (char *)convert_in.pixels;
        img_size = convert_in.size;
    }

    dibparams =
        e2s_get_DIB_params((PU_BITMAPINFO)BmiSrc, (const U_RGBQUAD **)&ct,
                           &numCt, &width, &height, &colortype, &invert);
    // if enable to read header, then exit
    if (dibparams || width > MAX_BMP_WIDTH || height > MAX_BMP_HEIGHT) {
        free(convert_out.pixels);
        states->Error = true;
        return;
    }
    // check that what we will read in the DIB_to_RGBA conversion is actually
    // there
    size_t offset_check =
        (size_t)((float)width * (float)height * get_pixel_size(colortype));
    if (((in + img_size) < in + offset_check)) {
        free(convert_out.pixels);
        states->Error = true;
        return;
    }
    DIB_to_RGBA(in, ct, numCt, &rgba_px, width, height, colortype, numCt,
                invert);

    if (rgba_px != NULL) {
        convert_inpng.size = width * 4 * height;
        convert_inpng.width = width;
        convert_inpng.height = height;
        convert_inpng.pixels = (RGBAPixel *)rgba_px;
        convert_inpng.bytewidth = BmiSrc->biWidth * 3;
        convert_inpng.bytes_per_pixel = 3;

        rgb2png(&convert_inpng, &b64Bmp, &b64s);
        tmp = (char *)b64Bmp;
        b64Bmp = base64_encode((unsigned char *)b64Bmp, b64s, &b64s);
        free(convert_out.pixels);
        free(tmp);
        free(rgba_px);
    }

    if (b64Bmp != NULL) {
        fprintf(out, "%s\" ", b64Bmp);
        free(b64Bmp);
    } else {
        // transparent 5x5 px png
        fprintf(out, "iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAABGdBTUEAA"
                     "LGPC/xhBQAAAAZiS0dEAP8A/wD/"
                     "oL2nkwAAAAlwSFlzAAALEwAACxMBAJqcGAAAAAd0SU1FB+"
                     "ABFREtOJX7FAkAAAAIdEVYdENvbW1lbnQA9syWvwAAAAxJREFUCNdjYKA"
                     "TAAAAaQABwB3y+AAAAABJRU5ErkJggg==\" ");
    }
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
