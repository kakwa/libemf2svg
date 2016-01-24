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
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRALPHABLEND_print(contents, states);
    }
}
void U_EMRBITBLT_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRBITBLT_print(contents, states);
    }
    // PU_EMRBITBLT pEmr = (PU_EMRBITBLT) (contents);
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
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSTRETCHBLT_print(contents, states);
    }
    // PU_EMRSTRETCHBLT pEmr = (PU_EMRSTRETCHBLT) (contents);
}
void U_EMRSTRETCHDIBITS_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSTRETCHDIBITS_print(contents, states);
    }
    PU_EMRSTRETCHDIBITS pEmr = (PU_EMRSTRETCHDIBITS)(contents);
    // FIXME Highly unsafe, trusting the EMF offsets where we shouldn't
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);
    // FIXME Highly unsafe, trusting the EMF offsets where we shouldn't
    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);
    char *b64Bmp = NULL;
    size_t b64s;
    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);
    switch (BmiSrc->biCompression) {
    case U_BI_UNKNOWN:
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    case U_BI_RGB:
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    case U_BI_RLE8:
        // FIXME need to implement
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    case U_BI_RLE4:
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    case U_BI_BITFIELDS:
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    case U_BI_JPEG:
        b64Bmp = base64_encode(BmpSrc, pEmr->cbBitsSrc, &b64s);
        fprintf(out, "xlink:href=\"data:image/jpg;base64,");
        break;
    case U_BI_PNG:
        b64Bmp = base64_encode(BmpSrc, pEmr->cbBitsSrc, &b64s);
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    }
    if (b64Bmp != NULL) {
        fprintf(out, "%s\" />\n", b64Bmp);
        free(b64Bmp);
    } else {
        // transparent 5x5 px png
        fprintf(out, "iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAABGdBTUEAA"
                     "LGPC/xhBQAAAAZiS0dEAP8A/wD/"
                     "oL2nkwAAAAlwSFlzAAALEwAACxMBAJqcGAAAAAd0SU1FB+"
                     "ABFREtOJX7FAkAAAAIdEVYdENvbW1lbnQA9syWvwAAAAxJREFUCNdjYKA"
                     "TAAAAaQABwB3y+AAAAABJRU5ErkJggg==\" />\n");
    }
}
void U_EMRTRANSPARENTBLT_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRTRANSPARENTBLT_print(contents, states);
    }
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
