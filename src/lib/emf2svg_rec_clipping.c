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
#include "uemf.h"
#include "emf2svg.h"
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include "pmf2svg.h"
#include "pmf2svg_print.h"

void U_EMREXCLUDECLIPRECT_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMREXCLUDECLIPRECT_print(contents, states);
    }
    //PU_EMRELLIPSE pEmr = (PU_EMRELLIPSE)(contents);
}
void U_EMREXTSELECTCLIPRGN_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMREXTSELECTCLIPRGN_print(contents, states);
    }
    // PU_EMREXTSELECTCLIPRGN pEmr = (PU_EMREXTSELECTCLIPRGN) (contents);
}
void U_EMRINTERSECTCLIPRECT_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRINTERSECTCLIPRECT_print(contents, states);
    }
}
void U_EMROFFSETCLIPRGN_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMROFFSETCLIPRGN_print(contents, states);
    }
    // PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);
}
void U_EMRSELECTCLIPPATH_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSELECTCLIPPATH_print(contents, states);
    }
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
