/**
  @file upmf_draw.c

  @brief Functions for printing EMF records
  */

/*
File:      upmf_draw.c
Version:   0.0.3
Date:      24-MAR-2014
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2014 David Mathog and California Institute of Technology (Caltech)
*/

/* compiler options:

   -DNOBRUSH causes brush objects to be treated as pen objects.  PowerPoint 2003
   and 2010 define pen objects
   as brush objects, and this is one way to see their structure even though they
   are misidentified.
   This option should only be used for tiny test files, consisting of just line
   objects.
   */

#ifdef __cplusplus
extern "C" {
#endif

#include "emf2svg_private.h"
#include "pmf2svg.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//! \cond

#define UNUSED(x)                                                              \
    (void)(x) //! Please ignore - Doxygen simply insisted on including this

/*
   this function is not visible in the API.  Map an EMF+ world coordinate to
   an SVG coordinate: apply the EMF+ world transform, which yields device
   coordinates, then the global scaling ratio.  The GDI logical mapping
   (point_cal: map mode, window/viewport) must NOT be applied here - EMF+
   has its own transform stack and its device space is the EMF device space.
   */
static POINT_D pmf_point_cal(drawingStates *states, double x, double y) {
    POINT_D ret;
    double devX, devY;
    if (states->pmfTransformSet) {
        devX = states->pmfM11 * x + states->pmfM21 * y + states->pmfDx;
        devY = states->pmfM12 * x + states->pmfM22 * y + states->pmfDy;
    } else {
        devX = x;
        devY = y;
    }
    ret.x = devX * states->scaling;
    ret.y = devY * states->scaling;
    return ret;
}

/*
   this function is not visible in the API.  Build SVG fill attributes from an
   EMF+ U_PMF_ARGB color (0xAARRGGBB), translating the alpha channel to
   fill-opacity, into the caller buffer (>= 64 bytes).
   */
static void pmf_color_attr(uint32_t argb, char *buf) {
    uint8_t alpha = (argb >> 24) & 0xFF;
    if (alpha != 0xFF) {
        sprintf(buf, "fill=\"#%02x%02x%02x\" fill-opacity=\"%.4f\"",
                (argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF,
                alpha / 255.0);
    } else {
        sprintf(buf, "fill=\"#%02x%02x%02x\"", (argb >> 16) & 0xFF,
                (argb >> 8) & 0xFF, argb & 0xFF);
    }
}

/*
   this function is not visible in the API.  Emit one SVG gradient <stop> from
   an EMF+ U_PMF_ARGB color.
   */
static void pmf_grad_stop(drawingStates *states, U_PMF_ARGB c, double offset,
                          FILE *out) {
    fprintf(out, "<%sstop offset=\"%.4f\" style=\"stop-color:#%02x%02x%02x",
            states->nameSpaceString, offset, c.Red, c.Green, c.Blue);
    if (c.Alpha != 0xFF)
        fprintf(out, ";stop-opacity:%.4f", c.Alpha / 255.0);
    fprintf(out, "\" />");
}

/*
   this function is not visible in the API.  Resolve the fill for an EMF+
   Fill* record into the caller buffer `buf` (>= 64 bytes), emitting any
   required <defs> to `out` first.  When `btype` is set BrushID is itself an
   inline U_PMF_ARGB; otherwise BrushID indexes the EMF+ object table.
   Supported brushes: inline color, SolidColor object, LinearGradient object.
   Returns 1 when a fill was produced, 0 when nothing should be drawn.
   */
static int pmf_resolve_fill(drawingStates *states, int btype, uint32_t BrushID,
                            FILE *out, char *buf) {
    uint32_t Version, Type;
    const char *Data;
    pmfGraphObject *brush;
    const char *blimit;

    if (btype) { /* BrushID is an inline ARGB color */
        pmf_color_attr(BrushID, buf);
        return (1);
    }
    if (BrushID > 63)
        return (0);
    brush = &(states->pmfObjectTable[BrushID]);
    if ((brush->type != U_OT_Brush) || (brush->data == NULL))
        return (0);
    blimit = brush->data + brush->size;
    if (!U_PMF_BRUSH_get(brush->data, &Version, &Type, &Data, blimit))
        return (0);

    if (Type == U_BT_SolidColor) {
        uint8_t b, g, r, a;
        if (!U_PMF_ARGB_get(Data, &b, &g, &r, &a, blimit))
            return (0);
        pmf_color_attr(((uint32_t)a << 24) | ((uint32_t)r << 16) |
                           ((uint32_t)g << 8) | (uint32_t)b,
                       buf);
        return (1);
    }
    if (Type == U_BT_LinearGradient) {
        U_PMF_LINEARGRADIENTBRUSHDATA Lgbd;
        const char *Optdata;
        uint32_t id;
        POINT_D p1, p2;
        if (!U_PMF_LINEARGRADIENTBRUSHDATA_get(Data, &Lgbd, &Optdata, blimit))
            return (0);
        p1 = pmf_point_cal(states, Lgbd.RectF.X, Lgbd.RectF.Y);
        p2 = pmf_point_cal(states, Lgbd.RectF.X + Lgbd.RectF.Width,
                           Lgbd.RectF.Y + Lgbd.RectF.Height);
        /* a non-finite RectF would emit x1="nan" etc., which is invalid SVG
           that DTD (CDATA attributes) does not catch; draw nothing instead */
        if (!isfinite(p1.x) || !isfinite(p1.y) || !isfinite(p2.x) ||
            !isfinite(p2.y))
            return (0);
        id = states->pmfGradId++;
        fprintf(out,
                "<%sdefs><%slinearGradient id=\"pmf-grad-%u\" "
                "gradientUnits=\"userSpaceOnUse\" "
                "x1=\"%.4f\" y1=\"%.4f\" x2=\"%.4f\" y2=\"%.4f\">",
                states->nameSpaceString, states->nameSpaceString, id, p1.x,
                p1.y, p2.x, p2.y);
        pmf_grad_stop(states, Lgbd.StartColor, 0.0, out);
        pmf_grad_stop(states, Lgbd.EndColor, 1.0, out);
        fprintf(out, "</%slinearGradient></%sdefs>", states->nameSpaceString,
                states->nameSpaceString);
        sprintf(buf, "fill=\"url(#pmf-grad-%u)\"", id);
        return (1);
    }
    /* Hatch / Texture / PathGradient not handled yet */
    return (0);
}

/*
   this function is not visible in the API.  Resolve the stroke for an EMF+
   Draw* record from a Pen in the EMF+ object table into the caller buffer
   `buf` (>= 64 bytes): stroke color (from the pen's embedded SolidColor
   brush, defaulting to black), optional stroke-opacity, and stroke-width
   (pen width in EMF+ world units scaled to device).  Returns 1 on success.
   */
static int pmf_resolve_stroke(drawingStates *states, uint32_t PenID,
                              char *buf) {
    uint32_t Version, Type, Flags = 0, Unit = 0, bVersion, bType, penFlags;
    U_FLOAT Width = 1.0;
    const char *PenData, *Brush, *bData, *optData, *blimit;
    const unsigned char *fp;
    pmfGraphObject *pen;
    uint8_t b = 0, g = 0, r = 0, a = 0xFF; /* default opaque black */
    double sw, mag = 1.0;
    int n;

    if (PenID > 63)
        return (0);
    pen = &(states->pmfObjectTable[PenID]);
    /* require Version(4)+Type(4)+PenData Flags(4) so the PenData Flags read
       below stays inside the exact-sized object copy */
    if ((pen->type != U_OT_Pen) || (pen->data == NULL) || (pen->size < 12))
        return (0);
    blimit = pen->data + pen->size;
    /* PenData begins after Version(4)+Type(4); read its Flags directly (LE,
       in bounds since size >= 12) */
    PenData = pen->data + 8;
    fp = (const unsigned char *)PenData;
    penFlags = (uint32_t)fp[0] | ((uint32_t)fp[1] << 8) |
               ((uint32_t)fp[2] << 16) | ((uint32_t)fp[3] << 24);
    /* width (U_PMF_PENDATA_get bounds-checks Flags/Unit/Width) */
    (void)U_PMF_PENDATA_get(PenData, &Flags, &Unit, &Width, &optData, blimit);
    /* The embedded brush sits after the optional pen data, whose length the
       vendored U_PMF_PEN_get / U_PMF_LEN_PENDATA walk WITHOUT bounds-checking
       the variable-length fields (dashed/compound line data, custom caps);
       a truncated pen declaring one would over-read the object copy. Resolve
       the brush color only when none of those flags are set; otherwise keep
       the default black (these pens still get the correct width). */
    if (!(penFlags & (U_PD_DLData | U_PD_CLData | U_PD_CustomStartCap |
                      U_PD_CustomEndCap)) &&
        U_PMF_PEN_get(pen->data, &Version, &Type, &PenData, &Brush, blimit) &&
        (Brush >= pen->data) && (Brush + 8 <= blimit) &&
        U_PMF_BRUSH_get(Brush, &bVersion, &bType, &bData, blimit) &&
        (bType == U_BT_SolidColor)) {
        (void)U_PMF_ARGB_get(bData, &b, &g, &r, &a, blimit);
    }
    /* a world-unit pen width magnifies with the world transform, exactly like
       the geometry (pmf_point_cal); device-unit widths do not */
    if (states->pmfTransformSet && (Unit == U_UT_World)) {
        double det =
            states->pmfM11 * states->pmfM22 - states->pmfM12 * states->pmfM21;
        mag = sqrt(fabs(det));
    }
    sw = (Width > 0.0 ? (double)Width : 1.0) * mag * states->scaling;
    if (!isfinite(sw) || sw < 0.0)
        sw = states->scaling;
    n = sprintf(buf, "stroke=\"#%02x%02x%02x\"", r, g, b);
    if (a != 0xFF)
        n += sprintf(buf + n, " stroke-opacity=\"%.4f\"", a / 255.0);
    sprintf(buf + n, " stroke-width=\"%.4f\"", sw);
    return (1);
}

/*
   this function is not visible in the API.  Validate that the full declared
   extent of the current EMF+ record lies inside the EMF buffer, BEFORE any
   U_PMR_*_get parses record data (those getters trust the declared size).
   Returns 0 when the record is safe to parse, non zero otherwise.

   The bound is computed by subtraction rather than by adding the attacker
   controlled Size to the record pointer, which would wrap on 32 bit builds.
   checkOutOfEMF() is intentionally NOT used here: it latches states->Error,
   which would abort the whole conversion and discard all output on a single
   truncated EMF+ sub-record - the rest of the (valid) file must still render.
   */
static int pmf_record_unsafe(drawingStates *states, const char *contents,
                             U_PMF_CMN_HDR *hdr) {
    const char *cursor = contents;
    if (!U_PMF_CMN_HDR_get(&cursor, hdr))
        return (1);
    if ((uintptr_t)contents > states->endAddress)
        return (1);
    if ((uint64_t)hdr->Size > states->endAddress - (uintptr_t)contents)
        return (1);
    return (0);
}

/*
   this function is not visible in the API.  Build an SVG path "d" attribute
   from an EMF+ PATH object (raw object data, EMF+ file byte order).
   Coordinates are mapped through pmf_point_cal.  On success returns a
   malloc'ed string and sets *winding to the path fill mode; returns NULL
   when the path cannot be converted (unsupported encoding, malformed type
   stream, out of range coordinates).
   */
static char *pmf_path_build(drawingStates *states, const char *data,
                            uint32_t size, int *winding) {
    uint32_t Version, Count;
    uint16_t Flags;
    const char *Points;
    const char *Types;
    const char *blimit = data + size;
    POINT_D *pts;
    char *d;
    size_t pos;
    uint32_t i;
    int bez;

    if (!U_PMF_PATH_get(data, &Version, &Count, &Flags, &Points, &Types,
                        blimit))
        return (NULL);
    if (Count == 0)
        return (NULL);
    /* RLE-compressed point types and relative points are not handled yet */
    if (Flags & (U_PPF_R | U_PPF_P))
        return (NULL);
    if ((Types + Count) > blimit)
        return (NULL);

    /* read and transform the points */
    pts = (POINT_D *)malloc(Count * sizeof(POINT_D));
    if (pts == NULL)
        return (NULL);
    for (i = 0; i < Count; i++) {
        U_FLOAT x = 0.0, y = 0.0;
        int ok = (Flags & U_PPF_C) ? U_PMF_POINT_get(&Points, &x, &y, blimit)
                                   : U_PMF_POINTF_get(&Points, &x, &y, blimit);
        if (!ok) {
            free(pts);
            return (NULL);
        }
        pts[i] = pmf_point_cal(states, x, y);
        /* keep %.4f output length bounded */
        if (!isfinite(pts[i].x) || !isfinite(pts[i].y) ||
            (fabs(pts[i].x) > 1e9) || (fabs(pts[i].y) > 1e9)) {
            free(pts);
            return (NULL);
        }
    }

    /* validate the point type stream: only Start/Line/Bezier types,
       Bezier points coming in complete triples */
    bez = 0;
    for (i = 0; i < Count; i++) {
        switch (((uint8_t)Types[i]) & U_PPT_MASK) {
        case U_PPT_Start:
            if ((i != 0) && (bez % 3))
                goto invalid;
            break;
        case U_PPT_Line:
            if (bez % 3)
                goto invalid;
            break;
        case U_PPT_Bezier:
            if (i == 0)
                goto invalid;
            bez++;
            break;
        default:
            goto invalid;
        }
    }
    if (bez % 3)
        goto invalid;

    /* worst case per point: "C " + "-123456789.1234,-123456789.1234 " + "Z "
       which stays under 48 bytes */
    d = (char *)malloc((size_t)Count * 48 + 16);
    if (d == NULL)
        goto invalid;
    pos = 0;
    bez = 0;
    for (i = 0; i < Count; i++) {
        uint8_t t = (uint8_t)Types[i];
        switch (t & U_PPT_MASK) {
        case U_PPT_Start:
            pos += sprintf(d + pos, "M %.4f,%.4f ", pts[i].x, pts[i].y);
            break;
        case U_PPT_Line:
            pos += sprintf(d + pos, "L %.4f,%.4f ", pts[i].x, pts[i].y);
            break;
        case U_PPT_Bezier:
            if ((bez % 3) == 0)
                pos += sprintf(d + pos, "C ");
            pos += sprintf(d + pos, "%.4f,%.4f ", pts[i].x, pts[i].y);
            bez++;
            break;
        }
        if (t & U_PTP_CloseSubpath)
            pos += sprintf(d + pos, "Z ");
    }
    if (pos > 0)
        d[pos - 1] = '\0'; /* trim the trailing space */
    free(pts);
    *winding = (Flags & U_PPF_F) ? 1 : 0;
    return (d);

invalid:
    free(pts);
    return (NULL);
}

/*
   this function is not visible in the API.  Print "data" for one of the many
   records that has none.
   */
int U_PMR_NODATAREC_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    return (1);
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw points.
   */
void U_PMF_VARPOINTS_draw(const char **contents, int Flags, uint32_t Elements,
                          FILE *out, drawingStates *states) {
    return;
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw points.
   */
void U_PMF_VARPOINTF_S_draw(U_PMF_POINTF *Points, uint32_t Elements, FILE *out,
                            drawingStates *states) {
    return;
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw rectangles.
   */
int U_PMF_VARRECTF_S_draw(U_PMF_RECTF *Rects, uint32_t Elements, FILE *out,
                          drawingStates *states) {
    return (1);
}

/*
   this function is not visible in the API.  Common routine used by many
   functions.
   */
int U_PMF_VARBRUSHID_draw(int btype, uint32_t BrushID, FILE *out,
                          drawingStates *states) {
    return (1);
}
//! \endcond

/**
  \brief Print any EMF+ record
  \returns record length for a normal record, 0 for EMREOF or , -1 for a bad
  record
  \param contents   pointer to a buffer holding this EMF+ record
  \param blimit     one byte past the end of data of this EMF+ record
  \param recnum     EMF number of this record in contents
  \param off        Offset from the beginning of the EMF+ file.
  */
int U_pmf_onerec_draw(const char *contents, const char *blimit, int recnum,
                      int off, FILE *out, drawingStates *states) {
    int status;
    static U_OBJ_ACCUM ObjCont = {
        NULL, 0, 0, 0,
        0}; /* for keeping track of object continuation. These may
               be split across multiple EMF Comment records */
    U_PMF_CMN_HDR Header;
    const char *contemp = contents;
    if (!U_PMF_CMN_HDR_get(&contemp, &Header)) {
        return (-1);
    }

    int type = Header.Type & U_PMR_TYPE_MASK; /* strip the U_PMR_RECFLAG bit,
                                                 leaving the indexable part */
    if (type < U_PMR_MIN || type > U_PMR_MAX)
        return (-1); /* unknown EMF+ record type */
    /* Dual-mode arbitration (see U_emf_onerec_draw): GetDC opens a window in
       which the following GDI records are intended output; any other EMF+
       record closes it. The GDI fallback is muted only once the EMF+ layer has
       actually drawn something (states->emfPlusDrew, set by the draw handlers
       below), never on mere record presence. */
    states->gdiPlay = (type == U_PMR_GETDC);
    status =
        U_PMF_CMN_HDR_draw(Header, recnum, off, out, states); /* EMF+ part */

    /* Buggy EMF+ can set the continue bit and then do something else. In that
       case, force out the pending
       Object.  Side effect - clears the pending object. */
    if ((type != U_PMR_OBJECT) && (ObjCont.used > 0)) {
        U_PMR_OBJECT_draw(contents, blimit, &ObjCont, 1, out, states);
    }

    switch (type) {
    case (U_PMR_HEADER):
        U_PMR_HEADER_draw(contents, out, states);
        break;
    case (U_PMR_ENDOFFILE):
        U_PMR_ENDOFFILE_draw(contents, out, states);
        U_OA_release(&ObjCont);
        break;
    case (U_PMR_COMMENT):
        U_PMR_COMMENT_draw(contents, out, states);
        break;
    case (U_PMR_GETDC):
        U_PMR_GETDC_draw(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATSTART):
        U_PMR_MULTIFORMATSTART_draw(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATSECTION):
        U_PMR_MULTIFORMATSECTION_draw(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATEND):
        U_PMR_MULTIFORMATEND_draw(contents, out, states);
        break;
    case (U_PMR_OBJECT):
        U_PMR_OBJECT_draw(contents, blimit, &ObjCont, 0, out, states);
        break;
    case (U_PMR_CLEAR):
        U_PMR_CLEAR_draw(contents, out, states);
        break;
    case (U_PMR_FILLRECTS):
        U_PMR_FILLRECTS_draw(contents, blimit, out, states);
        break;
    case (U_PMR_DRAWRECTS):
        U_PMR_DRAWRECTS_draw(contents, blimit, out, states);
        break;
    case (U_PMR_FILLPOLYGON):
        U_PMR_FILLPOLYGON_draw(contents, out, states);
        break;
    case (U_PMR_DRAWLINES):
        U_PMR_DRAWLINES_draw(contents, out, states);
        break;
    case (U_PMR_FILLELLIPSE):
        U_PMR_FILLELLIPSE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWELLIPSE):
        U_PMR_DRAWELLIPSE_draw(contents, out, states);
        break;
    case (U_PMR_FILLPIE):
        U_PMR_FILLPIE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWPIE):
        U_PMR_DRAWPIE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWARC):
        U_PMR_DRAWARC_draw(contents, out, states);
        break;
    case (U_PMR_FILLREGION):
        U_PMR_FILLREGION_draw(contents, out, states);
        break;
    case (U_PMR_FILLPATH):
        U_PMR_FILLPATH_draw(contents, out, states);
        break;
    case (U_PMR_DRAWPATH):
        U_PMR_DRAWPATH_draw(contents, out, states);
        break;
    case (U_PMR_FILLCLOSEDCURVE):
        U_PMR_FILLCLOSEDCURVE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWCLOSEDCURVE):
        U_PMR_DRAWCLOSEDCURVE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWCURVE):
        U_PMR_DRAWCURVE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWBEZIERS):
        U_PMR_DRAWBEZIERS_draw(contents, out, states);
        break;
    case (U_PMR_DRAWIMAGE):
        U_PMR_DRAWIMAGE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWIMAGEPOINTS):
        U_PMR_DRAWIMAGEPOINTS_draw(contents, out, states);
        break;
    case (U_PMR_DRAWSTRING):
        U_PMR_DRAWSTRING_draw(contents, out, states);
        break;
    case (U_PMR_SETRENDERINGORIGIN):
        U_PMR_SETRENDERINGORIGIN_draw(contents, out, states);
        break;
    case (U_PMR_SETANTIALIASMODE):
        U_PMR_SETANTIALIASMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETTEXTRENDERINGHINT):
        U_PMR_SETTEXTRENDERINGHINT_draw(contents, out, states);
        break;
    case (U_PMR_SETTEXTCONTRAST):
        U_PMR_SETTEXTCONTRAST_draw(contents, out, states);
        break;
    case (U_PMR_SETINTERPOLATIONMODE):
        U_PMR_SETINTERPOLATIONMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETPIXELOFFSETMODE):
        U_PMR_SETPIXELOFFSETMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETCOMPOSITINGMODE):
        U_PMR_SETCOMPOSITINGMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETCOMPOSITINGQUALITY):
        U_PMR_SETCOMPOSITINGQUALITY_draw(contents, out, states);
        break;
    case (U_PMR_SAVE):
        U_PMR_SAVE_draw(contents, out, states);
        break;
    case (U_PMR_RESTORE):
        U_PMR_RESTORE_draw(contents, out, states);
        break;
    case (U_PMR_BEGINCONTAINER):
        U_PMR_BEGINCONTAINER_draw(contents, out, states);
        break;
    case (U_PMR_BEGINCONTAINERNOPARAMS):
        U_PMR_BEGINCONTAINERNOPARAMS_draw(contents, out, states);
        break;
    case (U_PMR_ENDCONTAINER):
        U_PMR_ENDCONTAINER_draw(contents, out, states);
        break;
    case (U_PMR_SETWORLDTRANSFORM):
        U_PMR_SETWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_RESETWORLDTRANSFORM):
        U_PMR_RESETWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_MULTIPLYWORLDTRANSFORM):
        U_PMR_MULTIPLYWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_TRANSLATEWORLDTRANSFORM):
        U_PMR_TRANSLATEWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_SCALEWORLDTRANSFORM):
        U_PMR_SCALEWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_ROTATEWORLDTRANSFORM):
        U_PMR_ROTATEWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_SETPAGETRANSFORM):
        U_PMR_SETPAGETRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_RESETCLIP):
        U_PMR_RESETCLIP_draw(contents, out, states);
        break;
    case (U_PMR_SETCLIPRECT):
        U_PMR_SETCLIPRECT_draw(contents, out, states);
        break;
    case (U_PMR_SETCLIPPATH):
        U_PMR_SETCLIPPATH_draw(contents, out, states);
        break;
    case (U_PMR_SETCLIPREGION):
        U_PMR_SETCLIPREGION_draw(contents, out, states);
        break;
    case (U_PMR_OFFSETCLIP):
        U_PMR_OFFSETCLIP_draw(contents, out, states);
        break;
    case (U_PMR_DRAWDRIVERSTRING):
        U_PMR_DRAWDRIVERSTRING_draw(contents, out, states);
        break;
    case (U_PMR_STROKEFILLPATH):
        U_PMR_STROKEFILLPATH_draw(contents, out, states);
        break;
    case (U_PMR_SERIALIZABLEOBJECT):
        U_PMR_SERIALIZABLEOBJECT_draw(contents, out, states);
        break;
    case (U_PMR_SETTSGRAPHICS):
        U_PMR_SETTSGRAPHICS_draw(contents, out, states);
        break;
    case (U_PMR_SETTSCLIP):
        U_PMR_SETTSCLIP_draw(contents, out, states);
        break;
    }
    if (states->Error) {
        U_OA_release(&ObjCont);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CMN_HDR object
  \return number of bytes in record, 0 on error
  \param  Header     Header of the record
  \param  precnum    EMF+ record number in file.
  \param  off        Offset in file to the start of this EMF+ record.
  common structure present at the beginning of all(*) EMF+ records
  */
int U_PMF_CMN_HDR_draw(U_PMF_CMN_HDR Header, int precnum, int off, FILE *out,
                       drawingStates *states) {
    return ((int)Header.Size);
}

/**
  \brief Print data from a an array of uint8_t values
  \return 1
  \param  Start      Text to lead array data
  \param  Array      uint8_t array of data passed as char *
  \param  Elements   Number of elements in Array
  \param  End        Text to follow array data
  */
int U_PMF_UINT8_ARRAY_draw(const char *Start, const uint8_t *Array,
                           int Elements, char *End, FILE *out,
                           drawingStates *states) {
    return (1);
}

/**
  \brief Print value of an BrushType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.3, Microsoft name: BrushType Enumeration
  */
int U_PMF_BRUSHTYPEENUMERATION_draw(int otype, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of an BrushType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.4, Microsoft name: BrushType Enumeration
  */
int U_PMF_COMBINEMODEENUMERATION_draw(int otype, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of a HatchStyle Enumeration
  \returns record 1 on sucess, 0 on error
  \param hstype    Value to print.
  EMF+ manual 2.1.1.13, Microsoft name: HatchStyle Enumeration
  */
int U_PMF_HATCHSTYLEENUMERATION_draw(int hstype, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of an ObjectType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.22, Microsoft name: ObjectType Enumeration
  */
int U_PMF_OBJECTTYPEENUMERATION_draw(int otype, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of a  U_PMF_PATHPOINTTYPE_ENUM object
  \return 1
  \param  Type   Value to print
  EMF+ manual 2.1.1.23, Microsoft name: PathPointType Enumeration
  */
int U_PMF_PATHPOINTTYPE_ENUM_draw(int Type, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a PixelFormat Enumeration value
  \return 1 always
  \param  pfe   A PixelFormat Enumeration value
  EMF+ manual 2.1.1.25, Microsoft name: PixelFormat Enumeration (U_PF_*)
  */
int U_PMF_PX_FMT_ENUM_draw(int pfe, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print as text a RegionNodeDataType Enumeration
  \return 1
  \param  Type   RegionNodeDataType Enumeration
  EMF+ manual 2.1.1.27, Microsoft name: RegionNodeDataType Enumeration
  (U_RNDT_*)
  */
int U_PMF_NODETYPE_draw(int Type, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_BRUSH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.1, Microsoft name: EmfPlusBrush Object
  */
int U_PMF_BRUSH_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAP object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Which      A string which is either "Start" or "End".
  EMF+ manual 2.2.1.2, Microsoft name: EmfPlusCustomLineCap Object
  */
int U_PMF_CUSTOMLINECAP_draw(const char *contents, const char *Which, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_FONT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.3, Microsoft name: EmfPlusFont Object
  */
int U_PMF_FONT_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IMAGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.4, Microsoft name: EmfPlusImage Object
  */
int U_PMF_IMAGE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IMAGEATTRIBUTES object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.5, Microsoft name: EmfPlusImageAttributes Object
  */
int U_PMF_IMAGEATTRIBUTES_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.6, Microsoft name: EmfPlusPath Object
  */
int U_PMF_PATH_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PEN object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.7, Microsoft name: EmfPlusPen Object
  */
int U_PMF_PEN_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_REGION object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.8, Microsoft name: EmfPlusRegion Object
  */
int U_PMF_REGION_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_STRINGFORMAT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.9, Microsoft name: EmfPlusStringFormat Object
  */
int U_PMF_STRINGFORMAT_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_ARGB object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.1, Microsoft name: EmfPlusARGB Object
  */
int U_PMF_ARGB_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BITMAP object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.2, Microsoft name: EmfPlusBitmap Object
  */
int U_PMF_BITMAP_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BITMAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.3, Microsoft name: EmfPlusBitmapData Object
  */
int U_PMF_BITMAPDATA_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BLENDCOLORS object
  \return size in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.4, Microsoft name: EmfPlusBlendColors Object
  */
int U_PMF_BLENDCOLORS_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BLENDFACTORS object
  \return size on success, 0 on error
  \param  type       Type of BlendFactors, usually H or V
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.5, Microsoft name: EmfPlusBlendFactors Object
  */
int U_PMF_BLENDFACTORS_draw(const char *contents, const char *type, FILE *out,
                            drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BOUNDARYPATHDATA object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.6, Microsoft name: EmfPlusBoundaryPathData Object
  */
int U_PMF_BOUNDARYPATHDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BOUNDARYPOINTDATA object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.7, Microsoft name: EmfPlusBoundaryPointData Object
  */
int U_PMF_BOUNDARYPOINTDATA_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CHARACTERRANGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.8, Microsoft name: EmfPlusCharacterRange Object
  */
int U_PMF_CHARACTERRANGE_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_DASHEDLINEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.9, Microsoft name: EmfPlusCompoundLineData Object
  */
int U_PMF_COMPOUNDLINEDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_COMPRESSEDIMAGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.10, Microsoft name: EmfPlusCompressedImage Object

  This function does not do anything useful, but it is included so that all
  objects have a corresponding _get().
  */
int U_PMF_COMPRESSEDIMAGE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMENDCAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.11, Microsoft name: EmfPlusCustomEndCapData Object
  */
int U_PMF_CUSTOMENDCAPDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPARROWDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.12, Microsoft name: EmfPlusCustomLineCapArrowData Object
  */
int U_PMF_CUSTOMLINECAPARROWDATA_draw(const char *contents, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.13, Microsoft name: EmfPlusCustomLineCapData Object
  */
int U_PMF_CUSTOMLINECAPDATA_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Flags      CustomLineCapData Flags

  EMF+ manual 2.2.2.14, Microsoft name: EmfPlusCustomLineCapOptionalData Object
  */
int U_PMF_CUSTOMLINECAPOPTIONALDATA_draw(const char *contents, uint32_t Flags,
                                         FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMSTARTCAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.15, Microsoft name: EmfPlusCustomStartCapData Object
  */
int U_PMF_CUSTOMSTARTCAPDATA_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_DASHEDLINEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.16, Microsoft name: EmfPlusDashedLineData Object
  */
int U_PMF_DASHEDLINEDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_FILLPATHOBJ object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.17, Microsoft name: EmfPlusFillPath Object
  */
int U_PMF_FILLPATHOBJ_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_FOCUSSCALEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.18, Microsoft name: EmfPlusFocusScaleData Object
  */
int U_PMF_FOCUSSCALEDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_GRAPHICSVERSION_draw object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.19, Microsoft name: EmfPlusGraphicsVersion Object
  */
int U_PMF_GRAPHICSVERSION_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_HATCHBRUSHDATA_draw object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.20, Microsoft name: EmfPlusHatchBrushData Object
  */
int U_PMF_HATCHBRUSHDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_LANGUAGEIDENTIFIER object
  \return 1 on success, 0 on error
  \param  LId   Record from which to print data
  EMF+ manual 2.2.2.23, Microsoft name: EmfPlusLanguageIdentifier Object
  */
int U_PMF_LANGUAGEIDENTIFIER_draw(U_PMF_LANGUAGEIDENTIFIER LId, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEARGRADIENTBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.24, Microsoft name: EmfPlusLinearGradientBrushData Object
  */
int U_PMF_LINEARGRADIENTBRUSHDATA_draw(const char *contents, FILE *out,
                                       drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  BDFlag     Describes optional values in contents
  EMF+ manual 2.2.2.25, Microsoft name: EmfPlusLinearGradientBrushOptionalData
  Object
  */
int U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_draw(const char *contents, int BDFlag,
                                               FILE *out,
                                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEPATH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.26, Microsoft name: EmfPlusLinePath Object
  */
int U_PMF_LINEPATH_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_METAFILE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.27, Microsoft name: EmfPlusMetafile Object
  */
int U_PMF_METAFILE_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PALETTE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.28, Microsoft name: EmfPlusPalette Object
  */
int U_PMF_PALETTE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHGRADIENTBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.29, Microsoft name: EmfPlusPathGradientBrushData Object
  */
int U_PMF_PATHGRADIENTBRUSHDATA_draw(const char *contents, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHGRADIENTBRUSHOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  BDFlag     Describes optional values in contents
  EMF+ manual 2.2.2.30, Microsoft name: EmfPlusPathGradientBrushOptionalData
  Object
  */
int U_PMF_PATHGRADIENTBRUSHOPTIONALDATA_draw(const char *contents, int BDFlag,
                                             FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.31, Microsoft name: EmfPlusPathPointType Object
  */
int U_PMF_PATHPOINTTYPE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPERLE object
  \return Number of elements in the run, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.32, Microsoft name: EmfPlusPathPointTypeRLE Object
  */
int U_PMF_PATHPOINTTYPERLE_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPERLE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.33, Microsoft name: EmfPlusPenData Object
  */
int U_PMF_PENDATA_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PENOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Flags      PenData Flags that determine which optionaldata fields are
  present in the record.

  EMF+ manual 2.2.2.34, Microsoft name: EmfPlusPenOptionalData Object
  */
int U_PMF_PENOPTIONALDATA_draw(const char *contents, int Flags, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}
/**
  \brief Print data from a  U_PMF_POINT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.35, Microsoft name: EmfPlusPoint Object
  */
int U_PMF_POINT_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_POINT Structure
  \return 1 on success, 0 on error
  \param  Point   U_PMF_POINT Structure to print
  EMF+ manual 2.2.2.35, Microsoft name: EmfPlusPoint Object
  */
int U_PMF_POINT_S_draw(U_PMF_POINT *Point, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_POINTF object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.36, Microsoft name: EmfPlusPointF Object
  */
int U_PMF_POINTF_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_POINTF Structure
  \return 1 on success, 0 on error
  \param  Point   U_PMF_POINTF Structure to print
  EMF+ manual 2.2.2.36, Microsoft name: EmfPlusPointF Object
  */
int U_PMF_POINTF_S_draw(U_PMF_POINTF *Point, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_POINTR object
  \return bytes traversed on success, 0 on error
  \param  contents   Pointer to next data to print
  \param  Xpos       X coordinate for current point
  \param  Ypos       Y coordinate for current point

  On each call the next relative offset is extracted, the current
  coordinates are modified with that offset, and the pointer is
  advanced to the next data point.

  EMF+ manual 2.2.2.37, Microsoft name: EmfPlusPointR Object
  */
int U_PMF_POINTR_draw(const char **contents, U_FLOAT *Xpos, U_FLOAT *Ypos,
                      FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.38, Microsoft name: EmfPlusRect Object
  */
int U_PMF_RECT_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECT Structure
  \return 1 on success, 0 on error
  \param  Rect   U_PMF_RECT structure
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECT_S_draw(U_PMF_RECT *Rect, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_RECTF object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECTF_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECTF Structure
  \return 1 on success, 0 on error
  \param  Rect   U_PMF_RECTF Structure
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECTF_S_draw(U_PMF_RECTF *Rect, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_REGIONNODE object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  \param  Level      Tree level.  This routine is recursive and could go down
  many levels. 1 is the top, >1 are child nodes.
  EMF+ manual 2.2.2.40, Microsoft name: EmfPlusRegionNode Object
  */
int U_PMF_REGIONNODE_draw(const char *contents, int Level, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_REGIONNODECHILDNODES object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  \param  Level      Tree level.  This routine is recursive and could go down
  many levels. 1 is the top, >1 are child nodes.
  EMF+ manual 2.2.2.41, Microsoft name: EmfPlusRegionNodeChildNodes Object
  */
int U_PMF_REGIONNODECHILDNODES_draw(const char *contents, int Level, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_REGIONNODEPATH object
  \return Size of data on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.42, Microsoft name: EmfPlusRegionNodePath Object
  */
int U_PMF_REGIONNODEPATH_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_SOLIDBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.43, Microsoft name: EmfPlusSolidBrushData Object
  */
int U_PMF_SOLIDBRUSHDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_STRINGFORMATDATA object
  \return 1 on success, 0 on error
  \param  contents      Record from which to print data
  \param  TabStopCount  Entries in TabStop array
  \param  RangeCount    Entries in CharRange array
  EMF+ manual 2.2.2.44, Microsoft name: EmfPlusStringFormatData Object
  */
int U_PMF_STRINGFORMATDATA_draw(const char *contents, uint32_t TabStopCount,
                                uint32_t RangeCount, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TEXTUREBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.45, Microsoft name: EmfPlusTextureBrushData Object
  */
int U_PMF_TEXTUREBRUSHDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TEXTUREBRUSHOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  HasImage   True if the record contains an image.

  EMF+ manual 2.2.2.46, Microsoft name: EmfPlusTextureBrushOptionalData Object
  */
int U_PMF_TEXTUREBRUSHOPTIONALDATA_draw(const char *contents, int HasImage,
                                        FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TRANSFORMMATRIX object stored in file byte
  order.
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.47, Microsoft name: EmfPlusTransformMatrix Object
  */
int U_PMF_TRANSFORMMATRIX_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TRANSFORMMATRIX structure
  \return 1 on success, 0 on error
  \param  Tm  U_PMF_TRANSFORMMATRIX structure
  EMF+ manual 2.2.2.47, Microsoft name: EmfPlusTransformMatrix Object
  */
int U_PMF_TRANSFORMMATRIX2_draw(U_PMF_TRANSFORMMATRIX *Tm, FILE *out,
                                drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_ROTMATRIX object
  \return 1 on success, 0 on error
  \param  Rm   U_PMF_ROTMATRIX object
  NOT DOCUMENTED, like EMF+ manual 2.2.2.47, Microsoft name:
  EmfPlusTransformMatrix Object, but missing offset values
  */
int U_PMF_ROTMATRIX2_draw(U_PMF_ROTMATRIX *Rm, FILE *out,
                          drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_IE_BLUR object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.1, Microsoft name: BlurEffect Object
  */
int U_PMF_IE_BLUR_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_BRIGHTNESSCONTRAST object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.2, Microsoft name: BrightnessContrastEffect Object
  */
int U_PMF_IE_BRIGHTNESSCONTRAST_draw(const char *contents, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORBALANCE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.3, Microsoft name: ColorBalanceEffect Object
  */
int U_PMF_IE_COLORBALANCE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORCURVE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.4, Microsoft name: ColorCurveEffect Object
  */
int U_PMF_IE_COLORCURVE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORLOOKUPTABLE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.5, Microsoft name: ColorLookupTableEffect Object
  */
int U_PMF_IE_COLORLOOKUPTABLE_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORMATRIX object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.6, Microsoft name: ColorMatrixEffect Object
  */
int U_PMF_IE_COLORMATRIX_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_HUESATURATIONLIGHTNESS object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.7, Microsoft name: HueSaturationLightnessEffect Object
  */
int U_PMF_IE_HUESATURATIONLIGHTNESS_draw(const char *contents, FILE *out,
                                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_LEVELS object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.8, Microsoft name: LevelsEffect Object
  */
int U_PMF_IE_LEVELS_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_REDEYECORRECTION object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.9, Microsoft name: RedEyeCorrectionEffect Object
  */
int U_PMF_IE_REDEYECORRECTION_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_SHARPEN object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.10, Microsoft name: SharpenEffect Object
  */
int U_PMF_IE_SHARPEN_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_TINT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.11, Microsoft name: TintEffect Object
  */
int U_PMF_IE_TINT_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/* *****************************************************************************************
 */
/* EMF+ records, the EMF+ record header is printed separately, these print the
 * contents only */
/* *****************************************************************************************
 */

/**
  \brief Print data from a  U_PMR_OFFSETCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.1, Microsoft name: EmfPlusOffsetClip Record,  Index 0x35
  */
int U_PMR_OFFSETCLIP_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_OFFSETCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.2, Microsoft name: EmfPlusResetClip Record, Index 0x31
  */
int U_PMR_RESETCLIP_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_SETCLIPPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.3, Microsoft name: EmfPlusSetClipPath Record, Index 0x33
  */
int U_PMR_SETCLIPPATH_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCLIPRECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.4, Microsoft name: EmfPlusSetClipRect Record, Index 0x32
  */
int U_PMR_SETCLIPRECT_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCLIPREGION record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.5, Microsoft name: EmfPlusSetClipRegion Record, Index 0x34
  */
int U_PMR_SETCLIPREGION_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_COMMENT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.2.1, Microsoft name: EmfPlusComment Record, Index 0x03
  */
int U_PMR_COMMENT_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_ENDOFFILE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.1, Microsoft name: EmfPlusEndOfFile Record, Index 0x02
  */
int U_PMR_ENDOFFILE_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_ENDOFFILE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.2, Microsoft name: EmfPlusGetDC Record, Index 0x04
  */
int U_PMR_GETDC_draw(const char *contents, FILE *out, drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_HEADER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.3, Microsoft name: EmfPlusHeader Record, Index 0x01
  */
int U_PMR_HEADER_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_CLEAR record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.1, Microsoft name: EmfPlusClear Record, Index 0x09
  */
int U_PMR_CLEAR_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWARC record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.2, Microsoft name: EmfPlusDrawArc Record, Index 0x12
  */
int U_PMR_DRAWARC_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWBEZIERS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.3, Microsoft name: EmfPlusDrawBeziers Record, Index 0x19
  */
int U_PMR_DRAWBEZIERS_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWCLOSEDCURVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data

  Curve is a cardinal spline.
  References sent by MS support:
http://alvyray.com/Memos/CG/Pixar/spline77.pdf
http://msdn.microsoft.com/en-us/library/4cf6we5y(v=vs.110).aspx

EMF+ manual 2.3.4.4, Microsoft name: EmfPlusDrawClosedCurve Record, Index 0x17
*/
int U_PMR_DRAWCLOSEDCURVE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWCURVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data

  Curve is a cardinal spline, using doubled terminator points to generate curves
for the terminal segments.
  References sent by MS support:
http://alvyray.com/Memos/CG/Pixar/spline77.pdf
http://msdn.microsoft.com/en-us/library/4cf6we5y(v=vs.110).aspx

EMF+ manual 2.3.4.5, Microsoft name: EmfPlusDrawCurve Record, Index 0x18
*/
int U_PMR_DRAWCURVE_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWDRIVERSTRING record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.6, Microsoft name: EmfPlusDrawDriverString Record, Index
  0x36
  */
int U_PMR_DRAWDRIVERSTRING_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWELLIPSE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.7, Microsoft name: EmfPlusDrawEllipse Record, Index 0x0F
  */
int U_PMR_DRAWELLIPSE_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWIMAGE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.8, Microsoft name: EmfPlusDrawImage Record, Index 0x1A
  */
int U_PMR_DRAWIMAGE_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWIMAGEPOINTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.9, Microsoft name: EmfPlusDrawImagePoints Record, Index 0x1B
  */
int U_PMR_DRAWIMAGEPOINTS_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWLINES record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.10, Microsoft name: EmfPlusDrawLines Record, Index 0x0D
  */
int U_PMR_DRAWLINES_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    int ctype, dtype, RelAbs, ok;
    uint32_t i, PenID, Elements, count, maxpts, psize;
    U_PMF_POINTF *Points = NULL;
    U_PMF_CMN_HDR hdr;
    char stroke[128];
    if (states->inPath)
        return (status);
    if (pmf_record_unsafe(states, contents, &hdr))
        return (status);
    if (!U_PMR_DRAWLINES_get(contents, &hdr, &PenID, &ctype, &dtype, &RelAbs,
                             &Elements, &Points))
        return (status);
    /* relative points are variable-length; not handled yet */
    if (RelAbs) {
        free(Points);
        return (status);
    }
    /* U_PMF_VARPOINTS_get NULLs *Points on failure, so a non-NULL pointer is a
       valid owned buffer. Bound the count by what fits the record so a bogus
       Elements cannot emit garbage points past the real data (points start at
       header 12 + Elements 4 = 16). */
    if ((Points != NULL) && pmf_resolve_stroke(states, PenID, stroke)) {
        psize = ctype ? sizeof(U_PMF_POINT) : sizeof(U_PMF_POINTF);
        maxpts = (hdr.Size > 16) ? (uint32_t)((hdr.Size - 16) / psize) : 0;
        count = (Elements < maxpts) ? Elements : maxpts;
        /* a single non-finite point would emit "nan" and break the path, so
           require the whole polyline to be finite before emitting anything */
        ok = (count >= 2);
        for (i = 0; ok && i < count; i++) {
            POINT_D p = pmf_point_cal(states, Points[i].X, Points[i].Y);
            if (!isfinite(p.x) || !isfinite(p.y))
                ok = 0;
        }
        if (ok) {
            states->emfPlusDrew = true;
            fprintf(out, "<!-- EMF+ DrawLines --><%spath d=\"",
                    states->nameSpaceString);
            for (i = 0; i < count; i++) {
                POINT_D p = pmf_point_cal(states, Points[i].X, Points[i].Y);
                fprintf(out, "%c %.4f,%.4f ", i ? 'L' : 'M', p.x, p.y);
            }
            if (dtype)
                fprintf(out, "Z");
            fprintf(out, "\" fill=\"none\" %s />\n", stroke);
        }
    }
    free(Points);
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.11, Microsoft name: EmfPlusDrawPath Record, Index 0x15
  */
int U_PMR_DRAWPATH_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    int winding = 0;
    uint32_t PathID, PenID;
    U_PMF_CMN_HDR hdr;
    pmfGraphObject *po;
    char *d;
    char stroke[128];
    /* a GDI path may be open (BEGINPATH..ENDPATH streams an unterminated
       "<path d=\"" attribute); emitting our own element now would corrupt it */
    if (states->inPath)
        return (status);
    if (pmf_record_unsafe(states, contents, &hdr))
        return (status);
    if (!U_PMR_DRAWPATH_get(contents, NULL, &PathID, &PenID))
        return (status);
    if (PathID > 63)
        return (status);
    po = &(states->pmfObjectTable[PathID]);
    if ((po->type != U_OT_Path) || (po->data == NULL))
        return (status);
    d = pmf_path_build(states, po->data, po->size, &winding);
    if (d == NULL)
        return (status);
    if (pmf_resolve_stroke(states, PenID, stroke)) {
        states->emfPlusDrew = true;
        fprintf(out,
                "<!-- EMF+ DrawPath --><%spath d=\"%s\" fill=\"none\" %s />\n",
                states->nameSpaceString, d, stroke);
    }
    free(d);
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWPIE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.12, Microsoft name: EmfPlusDrawPie Record, Index 0x0D
  */
int U_PMR_DRAWPIE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWRECTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param blimit      One byte past the last record in memory.
  EMF+ manual 2.3.4.13, Microsoft name: EmfPlusDrawRects Record, Index 0x0B
  */
int U_PMR_DRAWRECTS_draw(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states) {
    int status = 1;
    int ctype;
    uint32_t i, PenID, Elements;
    uint64_t avail;
    bool getterFreed;
    U_PMF_RECTF *Rects = NULL;
    U_PMF_CMN_HDR hdr;
    char stroke[128];
    UNUSED(blimit);
    if (states->inPath)
        return (status);
    if (pmf_record_unsafe(states, contents, &hdr))
        return (status);
    if (!U_PMR_DRAWRECTS_get(contents, &hdr, &PenID, &ctype, &Elements, &Rects))
        return (status);
    /* same dangling-Rects hazard as FillRects (U_PMF_VARRECTS_get frees but
       does not NULL on overflow); rects start after header 12 + Elements 4 */
    avail = (hdr.Size > 16) ? (hdr.Size - 16) : 0;
    getterFreed = ((uint64_t)Elements * sizeof(U_PMF_RECT)) > avail;
    if (!getterFreed && Rects != NULL &&
        pmf_resolve_stroke(states, PenID, stroke)) {
        for (i = 0; i < Elements; i++) {
            U_PMF_RECTF *r = Rects + i;
            if (!isfinite(r->X) || !isfinite(r->Y) || !(r->Width >= 0.0) ||
                !(r->Height >= 0.0))
                continue;
            states->emfPlusDrew = true;
            POINT_D ul = pmf_point_cal(states, r->X, r->Y);
            POINT_D ur = pmf_point_cal(states, r->X + r->Width, r->Y);
            POINT_D lr =
                pmf_point_cal(states, r->X + r->Width, r->Y + r->Height);
            POINT_D ll = pmf_point_cal(states, r->X, r->Y + r->Height);
            fprintf(out, "<!-- EMF+ DrawRects --><%spath d=\"",
                    states->nameSpaceString);
            fprintf(out,
                    "M %.4f,%.4f L %.4f,%.4f L %.4f,%.4f L %.4f,%.4f Z\" "
                    "fill=\"none\" %s />\n",
                    ul.x, ul.y, ur.x, ur.y, lr.x, lr.y, ll.x, ll.y, stroke);
        }
    }
    if (!getterFreed)
        free(Rects);
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWSTRING record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.14, Microsoft name: EmfPlusDrawString Record, Index 0x1C
  */
int U_PMR_DRAWSTRING_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLCLOSEDCURVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.15, Microsoft name: EmfPlusFillClosedCurve Record, Index
  0x16
  */
int U_PMR_FILLCLOSEDCURVE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLELLIPSE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.16, Microsoft name: EmfPlusFillEllipse Record, Index 0x0E
  */
int U_PMR_FILLELLIPSE_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.17, Microsoft name: EmfPlusFillPath Record, Index 0x14
  */
int U_PMR_FILLPATH_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    int btype, winding = 0;
    uint32_t PathID, BrushID;
    U_PMF_CMN_HDR hdr;
    pmfGraphObject *po;
    char *d;
    char fill[128];
    /* a GDI path may be open (BEGINPATH..ENDPATH streams an unterminated
       "<path d=\"" attribute); emitting our own element now would corrupt it */
    if (states->inPath)
        return (status);
    if (pmf_record_unsafe(states, contents, &hdr))
        return (status);
    if (!U_PMR_FILLPATH_get(contents, NULL, &PathID, &btype, &BrushID))
        return (status);
    if (PathID > 63)
        return (status);
    po = &(states->pmfObjectTable[PathID]);
    if ((po->type != U_OT_Path) || (po->data == NULL))
        return (status);
    d = pmf_path_build(states, po->data, po->size, &winding);
    if (d == NULL)
        return (status);
    /* resolve fill (emits any <defs> first); skip the element if unresolved */
    if (pmf_resolve_fill(states, btype, BrushID, out, fill)) {
        states->emfPlusDrew = true;
        /* GDI+ default fill rule is alternate (even-odd) */
        fprintf(
            out,
            "<!-- EMF+ FillPath --><%spath d=\"%s\" fill-rule=\"%s\" %s />\n",
            states->nameSpaceString, d, winding ? "nonzero" : "evenodd", fill);
    }
    free(d);
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPIE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.18, Microsoft name: EmfPlusFillPie Record, Index 0x10
  */
int U_PMR_FILLPIE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPOLYGON record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.19, Microsoft name: EmfPlusFillPolygon Record, Index 0x0C
  */
int U_PMR_FILLPOLYGON_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    int btype, ctype, RelAbs, ok;
    uint32_t i, BrushID, Elements, count, maxpts, psize;
    U_PMF_POINTF *Points = NULL;
    U_PMF_CMN_HDR hdr;
    char fill[128];
    if (states->inPath)
        return (status);
    if (pmf_record_unsafe(states, contents, &hdr))
        return (status);
    if (!U_PMR_FILLPOLYGON_get(contents, &hdr, &btype, &ctype, &RelAbs,
                               &BrushID, &Elements, &Points))
        return (status);
    /* relative points are variable-length; not handled yet */
    if (RelAbs) {
        free(Points);
        return (status);
    }
    /* U_PMF_VARPOINTS_get NULLs *Points on failure; bound the count by what the
       record holds (points start at header 12 + BrushID 4 + Elements 4 = 20) */
    if (Points != NULL) {
        psize = ctype ? sizeof(U_PMF_POINT) : sizeof(U_PMF_POINTF);
        maxpts = (hdr.Size > 20) ? (uint32_t)((hdr.Size - 20) / psize) : 0;
        count = (Elements < maxpts) ? Elements : maxpts;
        /* a polygon needs >= 3 points and all must be finite (a non-finite
           coordinate would emit "nan", which DTD validation cannot catch);
           validate before resolving the fill so no orphan <defs> is emitted */
        ok = (count >= 3);
        for (i = 0; ok && i < count; i++) {
            POINT_D p = pmf_point_cal(states, Points[i].X, Points[i].Y);
            if (!isfinite(p.x) || !isfinite(p.y))
                ok = 0;
        }
        if (ok && pmf_resolve_fill(states, btype, BrushID, out, fill)) {
            states->emfPlusDrew = true;
            fprintf(out, "<!-- EMF+ FillPolygon --><%spath d=\"",
                    states->nameSpaceString);
            for (i = 0; i < count; i++) {
                POINT_D p = pmf_point_cal(states, Points[i].X, Points[i].Y);
                fprintf(out, "%c %.4f,%.4f ", i ? 'L' : 'M', p.x, p.y);
            }
            fprintf(out, "Z\" %s />\n", fill);
        }
    }
    free(Points);
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLRECTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param blimit      One byte past the last record in memory.
  EMF+ manual 2.3.4.20, Microsoft name: EmfPlusFillRects Record, Index 0x0A
  */
int U_PMR_FILLRECTS_draw(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states) {
    int status = 1;
    int btype, ctype;
    uint32_t i, BrushID, Elements;
    uint64_t avail;
    bool getterFreed;
    U_PMF_RECTF *Rects = NULL;
    U_PMF_CMN_HDR hdr;
    char fill[128];
    UNUSED(blimit);
    /* a GDI path may be open (BEGINPATH..ENDPATH streams an unterminated
       "<path d=\"" attribute); emitting our own element now would corrupt it */
    if (states->inPath)
        return (status);
    if (pmf_record_unsafe(states, contents, &hdr))
        return (status);
    if (!U_PMR_FILLRECTS_get(contents, &hdr, &btype, &ctype, &BrushID,
                             &Elements, &Rects))
        return (status);
    /* The vendored U_PMF_VARRECTS_get leaves *Rects pointing at an already
       freed buffer (non NULL) when the declared element count does not fit
       the record, and U_PMR_FILLRECTS_get does not propagate that failure.
       Recompute its bound (it keeps Rects only when Elements rects of the
       on-disk size fit after the 20 byte preamble: header 12 + BrushID 4 +
       Elements 4, and it always validates against sizeof(U_PMF_RECT)). When
       the buffer was freed we must neither read nor free it again. */
    avail = (hdr.Size > 20) ? (hdr.Size - 20) : 0;
    getterFreed = ((uint64_t)Elements * sizeof(U_PMF_RECT)) > avail;
    /* resolve fill once (emits any <defs>); the same fill applies to every
       rect in the record */
    if (!getterFreed && Rects != NULL &&
        pmf_resolve_fill(states, btype, BrushID, out, fill)) {
        for (i = 0; i < Elements; i++) {
            U_PMF_RECTF *r = Rects + i;
            if (!isfinite(r->X) || !isfinite(r->Y) || !(r->Width >= 0.0) ||
                !(r->Height >= 0.0))
                continue;
            states->emfPlusDrew = true;
            POINT_D ul = pmf_point_cal(states, r->X, r->Y);
            POINT_D ur = pmf_point_cal(states, r->X + r->Width, r->Y);
            POINT_D lr =
                pmf_point_cal(states, r->X + r->Width, r->Y + r->Height);
            POINT_D ll = pmf_point_cal(states, r->X, r->Y + r->Height);
            fprintf(out, "<!-- EMF+ FillRects --><%spath d=\"",
                    states->nameSpaceString);
            fprintf(
                out,
                "M %.4f,%.4f L %.4f,%.4f L %.4f,%.4f L %.4f,%.4f Z\" %s />\n",
                ul.x, ul.y, ur.x, ur.y, lr.x, lr.y, ll.x, ll.y, fill);
        }
    }
    if (!getterFreed)
        free(Rects);
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLREGION record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.21, Microsoft name: EmfPlusFillRegion Record, Index 0x13
  */
int U_PMR_FILLREGION_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_OBJECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param  blimit     One byte past the last record in memory.
  \param  ObjCont    Structure that holds accumulated object.
  \param  term       Flag used when an abnormal termination of a series of
  continuation records is encountered.
  EMF+ manual 2.3.5.1, Microsoft name: EmfPlusObject Record, Index 0x13
  */
int U_PMR_OBJECT_draw(const char *contents, const char *blimit,
                      U_OBJ_ACCUM *ObjCont, int term, FILE *out,
                      drawingStates *states) {
    U_PMF_CMN_HDR Header;
    uint32_t ObjID;
    int otype, ntype;
    uint32_t TSize;
    const char *Data;
    int ttype, status;

    /* Continued records are a pain. Each contains the total size of the
       continued object in the first 4 bytes
       of data.  When the total hits that then then the record is complete, even
       though the continuation bit will
       still be set on that last record.  Check for this and then print the
       terminated continued series.
       */

    if (term) { /* mode for handling unexpected end of accumulated object */
        if (ObjCont->used == 0)
            return (0); /* no continued object pending */
        U_PMF_OBJECTTYPEENUMERATION_draw(ObjCont->Type, out, states);
        ttype = ObjCont->Type & 0x3F;
        status = 1;
    } else {
        status = U_PMR_OBJECT_get(contents, &Header, &ObjID, &otype, &ntype,
                                  &TSize, &Data);
        /* In a corrupt EMF+ file we might hit a new type of record before all
           the continuation records
           expected have been found.  If that happens terminate whatever we have
           accumulated so far, and then go on
           to emit the new (unexpected) record. */
        if (contents + Header.Size >= blimit)
            return (0);
        if (!status)
            return (status);
        if ((ObjCont->used > 0) &&
            (U_OA_append(ObjCont, NULL, 0, otype, ObjID) < 0)) {
            U_PMR_OBJECT_draw(contents, blimit, ObjCont, 1, out, states);
        }
        U_PMF_OBJECTTYPEENUMERATION_draw(otype, out, states);
        if (ntype) {
            if (checkOutOfEMF(states,
                              (uintptr_t)((uintptr_t)Data +
                                          (uintptr_t)Header.DataSize - 4)) ||
                ((int64_t)Header.DataSize - 4) < 0) {
                status = 0;
            } else {
                U_OA_append(
                    ObjCont, Data, Header.DataSize - 4, otype,
                    ObjID); // The total byte count is not added to the object
            }
        } else {
            if (checkOutOfEMF(states,
                              (uintptr_t)Data + (uintptr_t)Header.DataSize)) {
                status = 0;
            } else {
                U_OA_append(
                    ObjCont, Data, Header.DataSize, otype,
                    ObjID); // The total byte count is not added to the object
            }
        }
        if (ntype && ObjCont->used < TSize)
            return (status);
        /* preceding terminates any continued series for >= accumulated bytes */
        ttype = otype;
    }
    if (status) {
        /* store the completed object in the EMF+ object table so that
           subsequent records can reference it by object ID */
        if ((ObjCont->Id >= 0) && (ObjCont->Id < 64) && (ObjCont->used > 0) &&
            (ObjCont->accum != NULL)) {
            pmfGraphObject *slot = &(states->pmfObjectTable[ObjCont->Id]);
            free(slot->data);
            slot->data = malloc(ObjCont->used);
            if (slot->data != NULL) {
                memcpy(slot->data, ObjCont->accum, ObjCont->used);
                slot->size = ObjCont->used;
                slot->type = ttype;
            } else {
                slot->size = 0;
                slot->type = 0;
            }
        }
        switch (ttype) {
        case U_OT_Brush:
            (void)U_PMF_BRUSH_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Pen:
            (void)U_PMF_PEN_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Path:
            (void)U_PMF_PATH_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Region:
            (void)U_PMF_REGION_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Image:
            (void)U_PMF_IMAGE_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Font:
            (void)U_PMF_FONT_draw(ObjCont->accum, out, states);
            break;
        case U_OT_StringFormat:
            (void)U_PMF_STRINGFORMAT_draw(ObjCont->accum, out, states);
            break;
        case U_OT_ImageAttributes:
            (void)U_PMF_IMAGEATTRIBUTES_draw(ObjCont->accum, out, states);
            break;
        case U_OT_CustomLineCap:
            (void)U_PMF_CUSTOMLINECAP_draw(ObjCont->accum, "", out, states);
            break;
        case U_OT_Invalid:
        default:
            break;
        }
        U_OA_clear(ObjCont);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SERIALIZABLEOBJECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.5.2, Microsoft name: EmfPlusSerializableObject Record, Index
  0x38
  */
int U_PMR_SERIALIZABLEOBJECT_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETANTIALIASMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.1, Microsoft name: EmfPlusSetAntiAliasMode Record, Index
  0x1E
  */
int U_PMR_SETANTIALIASMODE_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCOMPOSITINGMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.2, Microsoft name: EmfPlusSetCompositingMode Record, Index
  0x23
  */
int U_PMR_SETCOMPOSITINGMODE_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCOMPOSITINGQUALITY record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.3, Microsoft name: EmfPlusSetCompositingQuality Record,
  Index 0x24
  */
int U_PMR_SETCOMPOSITINGQUALITY_draw(const char *contents, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETINTERPOLATIONMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.4, Microsoft name: EmfPlusSetInterpolationMode Record, Index
  0x21
  */
int U_PMR_SETINTERPOLATIONMODE_draw(const char *contents, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETPIXELOFFSETMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.5, Microsoft name: EmfPlusSetPixelOffsetMode Record, Index
  0x22
  */
int U_PMR_SETPIXELOFFSETMODE_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETRENDERINGORIGIN record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.6, Microsoft name: EmfPlusSetRenderingOrigin Record, Index
  0x1D
  */
int U_PMR_SETRENDERINGORIGIN_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTEXTCONTRAST record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.7, Microsoft name: EmfPlusSetTextContrast Record, Index 0x20
  */
int U_PMR_SETTEXTCONTRAST_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTEXTRENDERINGHINT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.8, Microsoft name: EmfPlusSetTextRenderingHint Record, Index
  0x1F
  */
int U_PMR_SETTEXTRENDERINGHINT_draw(const char *contents, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_BEGINCONTAINER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.1, Microsoft name: EmfPlusBeginContainer Record, Index 0x27
  */
int U_PMR_BEGINCONTAINER_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_BEGINCONTAINERNOPARAMS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.2, Microsoft name: EmfPlusBeginContainerNoParams Record,
  Index 0x28
  */
int U_PMR_BEGINCONTAINERNOPARAMS_draw(const char *contents, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_ENDCONTAINER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.3, Microsoft name: EmfPlusEndContainer Record, Index 0x29
  */
int U_PMR_ENDCONTAINER_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_RESTORE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.4, Microsoft name: EmfPlusRestore Record, Index 0x26
  */
int U_PMR_RESTORE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SAVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.5, Microsoft name: EmfPlusSave Record, Index 0x25
  */
int U_PMR_SAVE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTSCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.8.1, Microsoft name: EmfPlusSetTSClip Record, Index 0x3A
  */
int U_PMR_SETTSCLIP_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTSGRAPHICS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.8.2, Microsoft name: EmfPlusSetTSGraphics Record, Index 0x39
  */
int U_PMR_SETTSGRAPHICS_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_MULTIPLYWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.1, Microsoft name: EmfPlusMultiplyWorldTransform Record,
  Index 0x2C
  */
int U_PMR_MULTIPLYWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_RESETWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.2, Microsoft name: EmfPlusResetWorldTransform Record, Index
  0x2B
  */
int U_PMR_RESETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_ROTATEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.3, Microsoft name: EmfPlusRotateWorldTransform Record, Index
  0x2F
  */
int U_PMR_ROTATEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SCALEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.4, Microsoft name: EmfPlusScaleWorldTransform Record, Index
  0x2E
  */
int U_PMR_SCALEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETPAGETRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.5, Microsoft name: EmfPlusSetPageTransform Record, Index
  0x30
  */
int U_PMR_SETPAGETRANSFORM_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.6, Microsoft name: EmfPlusSetWorldTransform Record, Index
  0x2A
  */
int U_PMR_SETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    int status = 1;
    U_PMF_CMN_HDR hdr;
    U_PMF_TRANSFORMMATRIX Matrix;
    UNUSED(out);
    if (pmf_record_unsafe(states, contents, &hdr))
        return (status);
    /* header (12) + 6 floats (24) */
    if (hdr.Size < 36)
        return (status);
    if (!U_PMR_SETWORLDTRANSFORM_get(contents, NULL, &Matrix))
        return (status);
    states->pmfM11 = Matrix.m11;
    states->pmfM12 = Matrix.m12;
    states->pmfM21 = Matrix.m21;
    states->pmfM22 = Matrix.m22;
    states->pmfDx = Matrix.dX;
    states->pmfDy = Matrix.dY;
    states->pmfTransformSet = true;
    return (status);
}

/**
  \brief Print data from a  U_PMR_TRANSLATEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.7, Microsoft name: EmfPlusTranslateWorldTransform Record,
  Index 0x2D
  */
int U_PMR_TRANSLATEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                       drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_STROKEFILLPATH record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  */
int U_PMR_STROKEFILLPATH_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATSTART record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatStart Record, Index 0x05
  */
int U_PMR_MULTIFORMATSTART_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATSECTION record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatSection Record, Index 0x06
  */
int U_PMR_MULTIFORMATSECTION_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATEND record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatEnd Record, Index 0x06
  */
int U_PMR_MULTIFORMATEND_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

#ifdef __cplusplus
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
