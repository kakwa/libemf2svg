#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "emf2svg_private.h"
#include "emf2svg_print.h"

void U_EMREOF_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMREOF_print(contents, states);
    }
    if (states->transform_open) {
        fprintf(out, "</%sg>\n", states->nameSpaceString);
    }
    fprintf(out, "</%sg>\n", states->nameSpaceString);
    if (states->svgDelimiter)
        fprintf(out, "</%ssvg>\n", states->nameSpaceString);
}
void U_EMRHEADER_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRHEADER_print(contents, states);
    }
    char *string;
    int p1len;

    PU_EMRHEADER pEmr = (PU_EMRHEADER)(contents);
    if (pEmr->offDescription) {
        returnOutOfEmf((uint16_t *)((char *)(uint64_t)pEmr +
                                    (uint64_t)pEmr->offDescription) +
                       2 * (uint64_t)pEmr->nDescription);
        string =
            U_Utf16leToUtf8((uint16_t *)((char *)pEmr + pEmr->offDescription),
                            pEmr->nDescription, NULL);
        free(string);
        p1len =
            2 +
            2 * wchar16len((uint16_t *)((char *)pEmr + pEmr->offDescription));
        returnOutOfEmf((uint16_t *)((char *)(uint64_t)pEmr +
                                    (uint64_t)pEmr->offDescription +
                                    (uint64_t)p1len) +
                       2 * (uint64_t)pEmr->nDescription);
        string = U_Utf16leToUtf8(
            (uint16_t *)((char *)pEmr + pEmr->offDescription + p1len),
            pEmr->nDescription, NULL);
        free(string);
    }
    // object table allocation
    // allocate one more to directly use object indexes (starts at 1 and not 0)
    states->objectTable = calloc(pEmr->nHandles + 1, sizeof(emfGraphObject));
    states->objectTableSize = pEmr->nHandles;

    double ratioXY = (double)(pEmr->rclBounds.right - pEmr->rclBounds.left) /
                     (double)(pEmr->rclBounds.bottom - pEmr->rclBounds.top);

    if ((states->imgHeight != 0) && (states->imgWidth != 0)) {
        double tmpWidth = states->imgHeight * ratioXY;
        double tmpHeight = states->imgWidth / ratioXY;
        if (tmpWidth > states->imgWidth) {
            states->imgHeight = tmpHeight;
        } else {
            states->imgWidth = tmpWidth;
        }
    } else if (states->imgHeight != 0) {
        states->imgWidth = states->imgHeight * ratioXY;
    } else if (states->imgWidth != 0) {
        states->imgHeight = states->imgWidth / ratioXY;
    } else {
        states->imgWidth =
            (double)abs(pEmr->rclBounds.right - pEmr->rclBounds.left);
        states->imgHeight =
            (double)abs(pEmr->rclBounds.bottom - pEmr->rclBounds.top);
    }

    // set scaling for original resolution
    // states->scaling = 1;
    states->scaling = states->imgWidth /
                      (double)abs(pEmr->rclBounds.right - pEmr->rclBounds.left);

    // remember reference point of the output DC
    states->RefX = (double)pEmr->rclBounds.left;
    states->RefY = (double)pEmr->rclBounds.top;

    states->pxPerMm =
        (double)pEmr->szlDevice.cx / (double)pEmr->szlMillimeters.cx;

    if (states->svgDelimiter) {
        fprintf(
            out,
            "<?xml version=\"1.0\"  encoding=\"UTF-8\" standalone=\"no\"?>\n");
        fprintf(out, "<%ssvg version=\"1.1\" ", states->nameSpaceString);
        fprintf(out, "xmlns=\"http://www.w3.org/2000/svg\" ");
        fprintf(out, "xmlns:xlink=\"http://www.w3.org/1999/xlink\" ");
        if ((states->nameSpace != NULL) && (strlen(states->nameSpace) != 0)) {
            fprintf(out, "xmlns:%s=\"http://www.w3.org/2000/svg\" ",
                    states->nameSpace);
        }
        fprintf(out, "width=\"%.4f\" height=\"%.4f\">\n", states->imgWidth,
                states->imgHeight);
    }

    fprintf(out, "<%sg transform=\"translate(%.4f, %.4f)\">\n",
            states->nameSpaceString, -1.0 * states->RefX * states->scaling,
            -1.0 * states->RefY * states->scaling);
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
