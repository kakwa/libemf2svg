/**
  @file uemf_print.h
  
  @brief Prototypes for functions for printing records from EMF files.
*/

/*
File:      uemf_print.h
Version:   0.0.5
Date:      14-FEB-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

extern "C" {

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() macro */
#include <string.h>
#include "uemf.h"
#ifdef DARWIN
#include <memstream.h>
#endif

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define verbose_printf(...) if(states->verbose) printf(__VA_ARGS__);
#define FLAG_SUPPORTED verbose_printf("   Status: %sSUPPORTED%s\n", KGRN, KNRM);
#define FLAG_IGNORED   verbose_printf("   Status: %sIGNORED%s\n", KRED, KNRM);
#define FLAG_PARTIAL   verbose_printf("   Status: %sPARTIAL SUPPORT%s\n", KYEL, KNRM);
#define FLAG_USELESS   verbose_printf("   Status  %sUSELESS%s\n", KCYN, KNRM);
#define FLAG_RESET     verbose_printf("%s", KNRM);

// EMF Device Context structure
typedef struct emf_device_context {
    char           *font_name;
    bool            stroke_set;
    int             stroke_mode;  // enumeration from drawmode, not used if fill_set is not True
    int             stroke_idx;   // used with DRAW_PATTERN and DRAW_IMAGE to return the appropriate fill
    int             stroke_recidx;// record used to regenerate hatch when it needs to be redone due to bkmode, textmode, etc. change
    bool            fill_set;
    int             fill_mode;    // enumeration from drawmode, not used if fill_set is not True
    int             fill_idx;     // used with DRAW_PATTERN and DRAW_IMAGE to return the appropriate fill
    int             fill_recidx;  // record used to regenerate hatch when it needs to be redone due to bkmode, textmode, etc. change
    int             dirty;        // holds the dirty bits for text, stroke, fill
    U_SIZEL         sizeWnd;
    U_SIZEL         sizeView;
    U_POINTL        winorg;
    U_POINTL        vieworg;
    double          ScaleInX, ScaleInY;
    double          ScaleOutX, ScaleOutY;
    uint16_t        bkMode;
    U_COLORREF      bkColor;
    U_COLORREF      textColor;
    uint32_t        textAlign;
    U_XFORM         worldTransform;
    U_POINTL        cur;
} EMF_DEVICE_CONTEXT, *PEMF_DEVICE_CONTEXT;

// Stack of EMF Device Contexts
typedef struct dc_stack {
    EMF_DEVICE_CONTEXT DeviceContext;
    struct dc_stack * previous;
} EMF_DEVICE_CONTEXT_STACK;

// structure containing generator arguments
typedef struct {

    // SVG namespace (the '<something>:' before each fields)
    char *nameSpace; 

    // Verbose mode, output fields and fields values if True
    bool verbose; 

    // draw svg document delimiter or not
    bool svgDelimiter;

} generatorOptions;

// structure recording drawing states
typedef struct {

    // SVG namespace (the '<something>:' before each fields)
    char *nameSpace; 

    // Verbose mode, output fields and fields values if True
    bool verbose; 

    // draw svg document delimiter or not
    bool svgDelimiter;

    /* The current EMF Device Context
     * (Device Context == pen, brush, palette... see [MS-EMF].pdf) */
    EMF_DEVICE_CONTEXT currentDeviceContext; 

    /* Device Contexts can be saved (EMR_SAVEDC),
     * Previous Device Contexts can be restored (EMR_RESTOREDC)
     * So we store then in this stack 
     */
    EMF_DEVICE_CONTEXT_STACK * DeviceContextStack; 

    // flag to know if we are in an SVG path or not
    bool inPath;
} drawingStates;

#define BUFFERSIZE 1024
//! \cond

/* manipulate device context */

// add a device context on the stack included in states
void saveDeviceContext(drawingStates * states);
// copy device context from src in dest
void copyDeviceContext(EMF_DEVICE_CONTEXT * dest, EMF_DEVICE_CONTEXT * src);
// restore device context at <index> in the stack as current device context
void restoreDeviceContext(drawingStates * states, int32_t index );

/* prototypes for objects used in EMR records */
void hexbytes_print(drawingStates *states, uint8_t *buf,unsigned int num);
void colorref_print(drawingStates *states, U_COLORREF color);
void rgbquad_print(drawingStates *states, U_RGBQUAD color);
void rectl_print(drawingStates *states, U_RECTL rect);
void sizel_print(drawingStates *states, U_SIZEL sz);
void pointl_print(drawingStates *states, U_POINTL pt);
void point16_print(drawingStates *states, U_POINT16 pt, FILE * out);
void point16_draw(drawingStates *states, U_POINT16 pt,FILE * out);
void lcs_gamma_print(drawingStates *states, U_LCS_GAMMA lg);
void lcs_gammargb_print(drawingStates *states, U_LCS_GAMMARGB lgr);
void trivertex_print(drawingStates *states, U_TRIVERTEX tv);
void gradient3_print(drawingStates *states, U_GRADIENT3 g3);
void gradient4_print(drawingStates *states, U_GRADIENT4 g4);
void logbrush_print(drawingStates *states, U_LOGBRUSH lb);
void xform_print(drawingStates *states, U_XFORM xform);
void ciexyz_print(drawingStates *states, U_CIEXYZ ciexyz);
void ciexyztriple_print(drawingStates *states, U_CIEXYZTRIPLE cie3);
void logcolorspacea_print(drawingStates *states, U_LOGCOLORSPACEA lcsa);
void logcolorspacew_print(drawingStates *states, U_LOGCOLORSPACEW lcsa);
void panose_print(drawingStates *states, U_PANOSE panose);
void logfont_print(drawingStates *states, U_LOGFONT lf);
void logfont_panose_print(drawingStates *states, U_LOGFONT_PANOSE lfp); 
int  bitmapinfoheader_print(drawingStates *states, const char *Bmih);
void bitmapinfo_print(drawingStates *states, const char *Bmi);
void blend_print(drawingStates *states, U_BLEND blend);
void extlogpen_print(drawingStates *states, const PU_EXTLOGPEN elp);
void logpen_print(drawingStates *states, U_LOGPEN lp);
void logpltntry_print(drawingStates *states, U_LOGPLTNTRY lpny);
void logpalette_print(drawingStates *states, const PU_LOGPALETTE lp);
void rgndataheader_print(drawingStates *states, U_RGNDATAHEADER rdh);
void rgndata_print(drawingStates *states, const PU_RGNDATA rd);
void coloradjustment_print(drawingStates *states, U_COLORADJUSTMENT ca);
void pixelformatdescriptor_print(drawingStates *states, U_PIXELFORMATDESCRIPTOR pfd);
void emrtext_print(drawingStates *states, const char *emt, const char *record, int type);

/* prototypes for EMR records */
void U_EMRNOTIMPLEMENTED_print(const char *name, const char *contents, FILE *out, drawingStates *states);
void U_EMRHEADER_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYBEZIER_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYGON_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYLINE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYBEZIERTO_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYLINETO_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYPOLYLINE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYPOLYGON_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETWINDOWEXTEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETWINDOWORGEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETVIEWPORTEXTEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETVIEWPORTORGEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETBRUSHORGEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREOF_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETPIXELV_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETMAPPERFLAGS_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETMAPMODE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETBKMODE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETPOLYFILLMODE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETROP2_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETSTRETCHBLTMODE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETTEXTALIGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETCOLORADJUSTMENT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETTEXTCOLOR_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETBKCOLOR_print(const char *contents, FILE *out, drawingStates *states);
void U_EMROFFSETCLIPRGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRMOVETOEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETMETARGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREXCLUDECLIPRECT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRINTERSECTCLIPRECT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSCALEVIEWPORTEXTEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSCALEWINDOWEXTEX_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSAVEDC_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRRESTOREDC_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETWORLDTRANSFORM_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRMODIFYWORLDTRANSFORM_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSELECTOBJECT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCREATEPEN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCREATEBRUSHINDIRECT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRDELETEOBJECT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRANGLEARC_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRELLIPSE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRRECTANGLE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRROUNDRECT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRARC_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCHORD_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPIE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSELECTPALETTE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCREATEPALETTE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETPALETTEENTRIES_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRRESIZEPALETTE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRREALIZEPALETTE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREXTFLOODFILL_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRLINETO_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRARCTO_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYDRAW_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETARCDIRECTION_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETMITERLIMIT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRBEGINPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRENDPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCLOSEFIGURE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRFILLPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSTROKEANDFILLPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSTROKEPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRFLATTENPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRWIDENPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSELECTCLIPPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRABORTPATH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCOMMENT_print(const char *contents, FILE *out, drawingStates *states, const char *blimit, size_t off);
void U_EMRFILLRGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRFRAMERGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRINVERTRGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPAINTRGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREXTSELECTCLIPRGN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRBITBLT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSTRETCHBLT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRMASKBLT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPLGBLT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETDIBITSTODEVICE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSTRETCHDIBITS_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREXTCREATEFONTINDIRECTW_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREXTTEXTOUTA_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREXTTEXTOUTW_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYBEZIER16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYGON16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYLINE16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYBEZIERTO16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYLINETO16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYPOLYLINE16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYPOLYGON16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYDRAW16_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCREATEMONOBRUSH_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCREATEDIBPATTERNBRUSHPT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMREXTCREATEPEN_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETICMMODE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCREATECOLORSPACE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETCOLORSPACE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRDELETECOLORSPACE_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRPIXELFORMAT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSMALLTEXTOUT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRALPHABLEND_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETLAYOUT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRTRANSPARENTBLT_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRGRADIENTFILL_print(const char *contents, FILE *out, drawingStates *states);
void U_EMRCREATECOLORSPACEW_print(const char *contents, FILE *out, drawingStates *states);
int U_emf_onerec_print(const char *contents, const char *blimit, int recnum, size_t off, FILE *out, drawingStates *states);
int emf2svg(char *contents, size_t length, char **out, generatorOptions *options);
//! \endcond


}

