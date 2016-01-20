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

void U_EMRNOTIMPLEMENTED_draw(const char *name, const char *contents, FILE *out,
                              drawingStates *states) {
    UNUSED(name);
    // if (states->verbose){U_EMRNOTIMPLEMENTED_print(contents, states);}
    UNUSED(contents);
}
void U_swap4(void *ul, unsigned int count);
//! \endcond

/**
  \brief Print rect and rectl objects from Upper Left and Lower Right corner
  points.
  \param rect U_RECTL object
  */
double _dsign(double v) {
    if (v >= 0)
        return 1;
    else
        return -1;
}
void addFormToStack(drawingStates *states) {
    formStack *newForm = calloc(1, sizeof(formStack));
    FILE *NewFormStream = open_memstream(&newForm->form, &newForm->len);
    if (NewFormStream == NULL) {
        return;
    }
    newForm->formStream = NewFormStream;
    newForm->prev = states->currentDeviceContext.clipStack;
    states->currentDeviceContext.clipStack = newForm;
}
void arc_circle_draw(const char *contents, FILE *out, drawingStates *states) {
    PU_EMRANGLEARC pEmr = (PU_EMRANGLEARC)(contents);
    startPathDraw(states, out);
    U_POINTL radii;
    int sweep_flag = 0;
    int large_arc_flag = 0;
    // FIXME calculate the real orientation
    if (states->currentDeviceContext.arcdir > 0) {
        sweep_flag = 0;
        large_arc_flag = 0;
    } else {
        sweep_flag = 0;
        large_arc_flag = 0;
    }
    radii.x = pEmr->nRadius;
    radii.y = pEmr->nRadius;

    fprintf(out, "M ");
    POINT_D start;
    double angle = pEmr->eStartAngle * U_PI / 180;
    start.x = pEmr->nRadius * cos(angle) + pEmr->ptlCenter.x;
    start.y = pEmr->nRadius * sin(angle) + pEmr->ptlCenter.y;
    point_draw_d(states, start, out);

    fprintf(out, "A ");
    point_draw(states, radii, out);

    fprintf(out, "0 ");
    fprintf(out, "%d %d ", large_arc_flag, sweep_flag);

    angle = (pEmr->eStartAngle + pEmr->eSweepAngle) * U_PI / 180;
    POINT_D end;
    end.x = pEmr->nRadius * cos(angle) + pEmr->ptlCenter.x;
    end.y = pEmr->nRadius * sin(angle) + pEmr->ptlCenter.y;
    point_draw_d(states, end, out);
    endPathDraw(states, out);
}
void arc_draw(const char *contents, FILE *out, drawingStates *states,
              int type) {
    PU_EMRARC pEmr = (PU_EMRARC)(contents);
    startPathDraw(states, out);
    U_POINTL radii;
    int sweep_flag = 0;
    int large_arc_flag = 0;
    // FIXME calculate the real orientation
    if (states->currentDeviceContext.arcdir > 0) {
        sweep_flag = 0;
        large_arc_flag = 0;
    } else {
        sweep_flag = 0;
        large_arc_flag = 0;
    }
    radii.x = (pEmr->rclBox.right - pEmr->rclBox.left) / 2;
    radii.y = (pEmr->rclBox.bottom - pEmr->rclBox.top) / 2;

    fprintf(out, "M ");
    POINT_D start = int_el_rad(pEmr->ptlStart, pEmr->rclBox);
    point_draw_d(states, start, out);

    fprintf(out, "A ");
    point_draw(states, radii, out);

    fprintf(out, "0 ");
    fprintf(out, "%d %d ", large_arc_flag, sweep_flag);

    POINT_D end = int_el_rad(pEmr->ptlEnd, pEmr->rclBox);
    point_draw_d(states, end, out);

    switch (type) {
    case ARC_PIE:
        fprintf(out, "L ");
        U_POINTL center;
        center.x = (pEmr->rclBox.right + pEmr->rclBox.left) / 2;
        center.y = (pEmr->rclBox.bottom + pEmr->rclBox.top) / 2;
        point_draw(states, center, out);
        fprintf(out, "Z ");
        endFormDraw(states, out);
        break;
    case ARC_CHORD:
        fprintf(out, "Z ");
        endFormDraw(states, out);
        break;
    default:
        endPathDraw(states, out);
        break;
    }
}
void basic_stroke(drawingStates *states, FILE *out) {
    color_stroke(states, out);
    width_stroke(states, out, states->currentDeviceContext.stroke_width);
}
bool checkOutOfEMF(drawingStates *states, uint64_t address) {
    if (address > states->endAddress) {
        states->Error = true;
        return true;
    } else {
        return false;
    }
}
bool checkOutOfOTIndex(drawingStates *states, int64_t index) {
    if (index > states->objectTableSize) {
        states->Error = true;
        return true;
    } else {
        return false;
    }
}
void color_stroke(drawingStates *states, FILE *out) {
    fprintf(out, "stroke=\"#%02X%02X%02X\" ",
            states->currentDeviceContext.stroke_red,
            states->currentDeviceContext.stroke_green,
            states->currentDeviceContext.stroke_blue);
}
void copyDeviceContext(EMF_DEVICE_CONTEXT *dest, EMF_DEVICE_CONTEXT *src) {
    // copy simple data (int, double...)
    *dest = *src;

    // copy more complex data (pointers...)
    if (src->font_name != NULL) {
        dest->font_name =
            (char *)calloc(strlen(src->font_name) + 1, sizeof(char));
        strcpy(dest->font_name, src->font_name);
    }
    if (src->font_family != NULL) {
        dest->font_family =
            (char *)calloc(strlen(src->font_family) + 1, sizeof(char));
        strcpy(dest->font_family, src->font_family);
    }
    dest->clipStack = cpFormStack(src->clipStack);
}
formStack *cpFormStack(formStack *stack) {
    if (stack == NULL)
        return NULL;
    formStack *ret = calloc(1, sizeof(formStack));
    FILE *NewFormStream = open_memstream(&ret->form, &ret->len);
    ret->formStream = NewFormStream;
    ret->drawn = stack->drawn;
    ret->id = stack->id;
    if (stack->len != 0)
        fprintf(ret->formStream, "%s", stack->form);

    formStack *cur = ret;
    stack = stack->prev;
    while (stack != NULL) {
        formStack *new = calloc(1, sizeof(formStack));
        FILE *NewFormStream = open_memstream(&new->form, &new->len);
        new->formStream = NewFormStream;
        new->drawn = stack->drawn;
        new->id = stack->id;
        if (stack->len != 0)
            fprintf(new->formStream, "%s", stack->form);
        stack = stack->prev;
        cur->prev = new;
        cur = new;
    }
    return ret;
}
void cubic_bezier16_draw(const char *name, const char *contents, FILE *out,
                         drawingStates *states, int startingPoint) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16)(contents);
    startPathDraw(states, out);
    PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
    returnOutOfEmf((uint64_t)papts + (uint64_t)(pEmr->cpts) * sizeof(uint32_t));
    if (startingPoint == 1) {
        fprintf(out, "M ");
        point16_draw(states, papts[0], out);
    }
    const int ctrl1 = (0 + startingPoint) % 3;
    const int ctrl2 = (1 + startingPoint) % 3;
    const int to = (2 + startingPoint) % 3;
    for (i = startingPoint; i < pEmr->cpts; i++) {
        if ((i % 3) == ctrl1) {
            fprintf(out, "C ");
            point16_draw(states, papts[i], out);
        } else if ((i % 3) == ctrl2) {
            point16_draw(states, papts[i], out);
        } else if ((i % 3) == to) {
            point16_draw(states, papts[i], out);
        }
    }
    endPathDraw(states, out);
}
void cubic_bezier_draw(const char *name, const char *contents, FILE *out,
                       drawingStates *states, int startingPoint) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYBEZIER pEmr = (PU_EMRPOLYBEZIER)(contents);
    startPathDraw(states, out);
    PU_POINT papts = (PU_POINT)(&(pEmr->aptl));
    returnOutOfEmf((uint64_t)papts + (uint64_t)pEmr->cptl * sizeof(uint32_t));
    if (startingPoint == 1) {
        fprintf(out, "M ");
        point_draw(states, papts[0], out);
    }
    const int ctrl1 = (0 + startingPoint) % 3;
    const int ctrl2 = (1 + startingPoint) % 3;
    const int to = (2 + startingPoint) % 3;
    for (i = startingPoint; i < pEmr->cptl; i++) {
        if ((i % 3) == ctrl1) {
            fprintf(out, "C ");
            point_draw(states, papts[i], out);
        } else if ((i % 3) == ctrl2) {
            point_draw(states, papts[i], out);
        } else if ((i % 3) == to) {
            point_draw(states, papts[i], out);
        }
    }
    endPathDraw(states, out);
}
void endFormDraw(drawingStates *states, FILE *out) {
    if (!(states->inPath)) {
        fprintf(out, "\" ");
        bool filled;
        bool stroked;
        stroke_draw(states, out, &filled, &stroked);
        fill_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");
        fprintf(out, " />\n");
    }
}
void endPathDraw(drawingStates *states, FILE *out) {
    if (!(states->inPath)) {
        fprintf(out, "\" ");
        bool filled;
        bool stroked;
        stroke_draw(states, out, &filled, &stroked);
        fprintf(out, " fill=\"none\" />\n");
    }
}
void fill_draw(drawingStates *states, FILE *out, bool *filled, bool *stroked) {
    if (states->verbose) {
        fill_print(states);
    }
    char *fill_rule = calloc(40, sizeof(char));
    switch (states->currentDeviceContext.fill_mode) {
    case (U_ALTERNATE):
        sprintf(fill_rule, "fill-rule:\"evenodd\" ");
        break;
    case (U_WINDING):
        sprintf(fill_rule, "fill-rule:\"nonzero\" ");
        break;
    default:
        sprintf(fill_rule, " ");
        break;
    }
    switch (states->currentDeviceContext.fill_mode) {
    case U_BS_SOLID:
        *filled = true;
        fprintf(out, "%s", fill_rule);
        fprintf(out, "fill=\"#%02X%02X%02X\" ",
                states->currentDeviceContext.fill_red,
                states->currentDeviceContext.fill_green,
                states->currentDeviceContext.fill_blue);
        break;
    case U_BS_NULL:
        fprintf(out, "fill=\"none\" ");
        *filled = true;
        break;
    case U_BS_HATCHED:
    case U_BS_PATTERN:
    case U_BS_INDEXED:
    case U_BS_DIBPATTERN:
    case U_BS_DIBPATTERNPT:
    case U_BS_PATTERN8X8:
    case U_BS_DIBPATTERN8X8:
    case U_BS_MONOPATTERN:
    default:
        // partial
        fprintf(out, "fill=\"#%02X%02X%02X\" ",
                states->currentDeviceContext.fill_red,
                states->currentDeviceContext.fill_green,
                states->currentDeviceContext.fill_blue);
        *filled = true;
        break;
    }
    free(fill_rule);
    return;
}
void freeDeviceContext(EMF_DEVICE_CONTEXT *dc) {
    if (dc != NULL) {
        if (dc->font_name != NULL)
            free(dc->font_name);
        if (dc->font_family != NULL)
            free(dc->font_family);
        freeFormStack(dc->clipStack);
    }
}
void freeDeviceContextStack(drawingStates *states) {
    EMF_DEVICE_CONTEXT_STACK *stack_entry = states->DeviceContextStack;
    while (stack_entry != NULL) {
        EMF_DEVICE_CONTEXT_STACK *next_entry = stack_entry->previous;
        freeDeviceContext(&(stack_entry->DeviceContext));
        free(stack_entry);
        stack_entry = next_entry;
    }
}
void freeFormStack(formStack *stack) {
    while (stack != NULL) {
        fclose(stack->formStream);
        free(stack->form);
        formStack *tmp = stack;
        stack = stack->prev;
        free(tmp);
    }
    return;
}
void freeObject(drawingStates *states, uint16_t index) {
    if (states->objectTable[index].font_name != NULL)
        free(states->objectTable[index].font_name);
    if (states->objectTable[index].font_family != NULL)
        free(states->objectTable[index].font_family);
    states->objectTable[index] = (const emfGraphObject){0};
}
void freeObjectTable(drawingStates *states) {
    for (int32_t i = 0; i < (states->objectTableSize + 1); i++) {
        freeObject(states, i);
    }
}
void freePathStack(pathStack *stack) {
    while (stack != NULL) {
        // free(stack->pathStruct);
        pathStack *tmp = stack;
        stack = stack->next;
        free(tmp);
    }
    return;
}
int get_id(drawingStates *states) {
    states->uniqId++;
    return states->uniqId;
}
POINT_D int_el_rad(U_POINTL pt, U_RECTL rect) {
    POINT_D center, intersect, radii, pt_no;
    center.x = (rect.right + rect.left) / 2;
    center.y = (rect.bottom + rect.top) / 2;

    radii.x = (rect.right - rect.left) / 2;
    radii.y = (rect.bottom - rect.top) / 2;

    if ((radii.x == 0) || (radii.y == 0)) {
        return center;
    }

    // change orgin (new origin is ellipse center)
    pt_no.x = pt.x - center.x;
    pt_no.y = pt.y - center.y;

    if (pt_no.x == 0) {
        intersect.x = center.x;
        intersect.y = _dsign(pt_no.y) * radii.y + center.y;
        return intersect;
    }

    if (pt_no.y == 0) {
        intersect.x = _dsign(pt_no.x) * radii.x + center.x;
        intersect.y = center.y;
        return intersect;
    }

    // slope of the radial
    double slope = pt_no.y / pt_no.x;

    // Calculate the intersection.
    // With center as the origin:
    // * ellipse equation is: (x / radii.x)^2 + (y / radii.y) + 1
    // * radial equation is:  y = x * slope
    // Three part of the calculus:
    // * '_dsign(...) *' -> get correct quadrant
    // * 'sqrt(...)'     -> solve the equation
    // * '+ center.x/y'  -> back the EMF origin
    intersect.x =
        _dsign(pt_no.x) *
            sqrt(1 / ((pow(1 / radii.x, 2)) + pow((slope / radii.y), 2))) +
        center.x;
    intersect.y = _dsign(pt_no.y) * sqrt(1 / ((pow(1 / (slope * radii.x), 2)) +
                                              pow((1 / radii.y), 2))) +
                  center.y;

    return intersect;
}
void lineto_draw(const char *name, const char *field1, const char *field2,
                 const char *contents, FILE *out, drawingStates *states) {
    UNUSED(name);
    PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR)(contents);
    startPathDraw(states, out);
    fprintf(out, "L ");
    point_draw(states, pEmr->pair, out);
    endPathDraw(states, out);
}
void moveto_draw(const char *name, const char *field1, const char *field2,
                 const char *contents, FILE *out, drawingStates *states) {
    UNUSED(name);
    PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR)(contents);
    point_draw(states, pEmr->pair, out);
}
void newPathStruct(drawingStates *states) {
    pathStack *new_entry = calloc(1, sizeof(pathStack));
    if (states->emfStructure.pathStack == NULL) {
        states->emfStructure.pathStack = new_entry;
        states->emfStructure.pathStackLast = new_entry;
    } else {
        states->emfStructure.pathStackLast->next = new_entry;
        states->emfStructure.pathStackLast = new_entry;
    }
}
void no_stroke(drawingStates *states, FILE *out) {
    if (states->currentDeviceContext.fill_mode != U_BS_NULL) {
        fprintf(out, "stroke-width=\"1px\" ");
        fprintf(out, "stroke=\"#%02X%02X%02X\" ",
                states->currentDeviceContext.fill_red,
                states->currentDeviceContext.fill_green,
                states->currentDeviceContext.fill_blue);
    } else {
        fprintf(out, "stroke=\"none\" ");
        fprintf(out, "stroke-width=\"0.0\" ");
    }
}
void point16_draw(drawingStates *states, U_POINT16 pt, FILE *out) {
    POINT_D ptd = point_cal(states, (double)pt.x, (double)pt.y);
    states->cur_x = pt.x;
    states->cur_y = pt.y;
    fprintf(out, "%.4f,%.4f ", ptd.x, ptd.y);
}
POINT_D point_cal(drawingStates *states, double x, double y) {
    POINT_D ret;
    ret.x = states->offsetX + ((double)x * (double)states->scalingX);
    ret.y = states->offsetY + ((double)y * (double)states->scalingY);
    return ret;
}
void point_draw(drawingStates *states, U_POINT pt, FILE *out) {
    POINT_D ptd = point_cal(states, (double)pt.x, (double)pt.y);
    states->cur_x = pt.x;
    states->cur_y = pt.y;
    fprintf(out, "%.4f,%.4f ", ptd.x, ptd.y);
}
void point_draw_d(drawingStates *states, POINT_D pt, FILE *out) {
    POINT_D ptd = point_cal(states, pt.x, pt.y);
    states->cur_x = pt.x;
    states->cur_y = pt.y;
    fprintf(out, "%.4f,%.4f ", ptd.x, ptd.y);
}
void polyline16_draw(const char *name, const char *contents, FILE *out,
                     drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16)(contents);
    PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
    returnOutOfEmf((uint64_t)papts + (uint64_t)(pEmr->cpts) * sizeof(uint32_t));
    startPathDraw(states, out);
    for (i = 0; i < pEmr->cpts; i++) {
        if (polygon && i == 0) {
            fprintf(out, "M ");
        } else {
            fprintf(out, "L ");
        }
        point16_draw(states, papts[i], out);
    }
    endPathDraw(states, out);
}
void polyline_draw(const char *name, const char *contents, FILE *out,
                   drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYLINETO pEmr = (PU_EMRPOLYLINETO)(contents);
    startPathDraw(states, out);
    PU_POINT papts = (PU_POINT)(&(pEmr->aptl));
    returnOutOfEmf((uint64_t)papts + (uint64_t)(pEmr->cptl) * sizeof(uint32_t));
    for (i = 0; i < pEmr->cptl; i++) {
        if (polygon && i == 0) {
            fprintf(out, "M ");
        } else {
            fprintf(out, "L ");
        }
        point_draw(states, pEmr->aptl[i], out);
    }
    endPathDraw(states, out);
}
void polypolygon16_draw(const char *name, const char *contents, FILE *out,
                        drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16)(contents);
    PU_POINT16 papts = (PU_POINT16)((char *)pEmr->aPolyCounts +
                                    sizeof(uint32_t) * pEmr->nPolys);
    returnOutOfEmf((uint64_t)papts + (uint64_t)(pEmr->cpts) * sizeof(uint32_t));

    int counter = 0;
    int polygon_index = 0;
    for (i = 0; i < pEmr->cpts; i++) {
        if (counter == 0) {
            fprintf(out, "M ");
            point16_draw(states, papts[i], out);
        } else {
            fprintf(out, "L ");
            point16_draw(states, papts[i], out);
        }
        counter++;
        if (pEmr->aPolyCounts[polygon_index] == counter) {
            if (polygon)
                fprintf(out, "Z ");
            counter = 0;
            polygon_index++;
        }
    }
}
void polypolygon_draw(const char *name, const char *contents, FILE *out,
                      drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16)(contents);
    PU_POINT papts =
        (PU_POINT)((char *)pEmr->aPolyCounts + sizeof(uint32_t) * pEmr->nPolys);

    int counter = 0;
    int polygon_index = 0;
    returnOutOfEmf((uint64_t)papts + (uint64_t)(pEmr->cpts) * sizeof(uint32_t));
    for (i = 0; i < pEmr->cpts; i++) {
        if (counter == 0) {
            fprintf(out, "M ");
            point_draw(states, papts[i], out);
        } else {
            fprintf(out, "L ");
            point_draw(states, papts[i], out);
        }
        counter++;
        if (pEmr->aPolyCounts[polygon_index] == counter) {
            if (polygon)
                fprintf(out, "Z ");
            counter = 0;
            polygon_index++;
        }
    }
}
void rectl_draw(drawingStates *states, FILE *out, U_RECTL rect) {
    U_POINT pt;
    fprintf(out, "M ");
    pt.x = rect.left;
    pt.y = rect.top;
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.right;
    pt.y = rect.top;
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.right;
    pt.y = rect.bottom;
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.left;
    pt.y = rect.bottom;
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.left;
    pt.y = rect.top;
    point_draw(states, pt, out);
    fprintf(out, "Z ");
}
void restoreDeviceContext(drawingStates *states, int32_t index) {
    EMF_DEVICE_CONTEXT_STACK *stack_entry = states->DeviceContextStack;
    // we recover the 'abs(index)' element of the stack
    // we stop if the index was outside the DeviceContextStack
    int i = -1;
    for (; i > index && stack_entry != NULL; i--) {
        if (stack_entry->previous != NULL) {
            stack_entry = stack_entry->previous;
        } else {
            break;
        }
    }
    if (stack_entry == NULL || i != index) {
        states->Error = true;
        return;
    }
    // we copy it as the current device context
    freeDeviceContext(&(states->currentDeviceContext));
    states->currentDeviceContext = (EMF_DEVICE_CONTEXT){0};
    copyDeviceContext(&(states->currentDeviceContext),
                      &(stack_entry->DeviceContext));
}
void saveDeviceContext(drawingStates *states) {
    // create the new device context in the stack
    EMF_DEVICE_CONTEXT_STACK *new_entry =
        (EMF_DEVICE_CONTEXT_STACK *)calloc(1, sizeof(EMF_DEVICE_CONTEXT_STACK));
    copyDeviceContext(&(new_entry->DeviceContext),
                      &(states->currentDeviceContext));
    // put the new entry on the stack
    new_entry->previous = states->DeviceContextStack;
    states->DeviceContextStack = new_entry;
}
void setTransformIdentity(drawingStates *states) {
    states->currentDeviceContext.worldTransform.eM11 = 1.0;
    states->currentDeviceContext.worldTransform.eM12 = 0.0;
    states->currentDeviceContext.worldTransform.eM21 = 0.0;
    states->currentDeviceContext.worldTransform.eM22 = 1.0;
    states->currentDeviceContext.worldTransform.eDx = 0.0;
    states->currentDeviceContext.worldTransform.eDy = 0.0;
}
void startPathDraw(drawingStates *states, FILE *out) {
    if (!(states->inPath)) {
        fprintf(out, "<%spath d=\"M ", states->nameSpaceString);
        U_POINT pt;
        pt.x = states->cur_x;
        pt.y = states->cur_y;
        point_draw(states, pt, out);
    }
}
void stroke_draw(drawingStates *states, FILE *out, bool *filled,
                 bool *stroked) {
    float unit_stroke =
        states->currentDeviceContext.stroke_width * states->scaling;
    float dash_len = unit_stroke * 5;
    float dot_len = unit_stroke;
    if (states->verbose) {
        stroke_print(states);
    }

    if ((states->currentDeviceContext.stroke_mode & 0x000000FF) == U_PS_NULL) {
        // no stroke with the fill color with a with of 1px
        no_stroke(states, out);
        *stroked = true;
        return;
    }
    // pen type
    switch (states->currentDeviceContext.stroke_mode & 0x000F0000) {
    case U_PS_COSMETIC:
        color_stroke(states, out);
        // width_stroke(states, out, 1 / states->scaling);
        width_stroke(states, out, 1);
        *stroked = true;
        break;
    case U_PS_GEOMETRIC:
        basic_stroke(states, out);
        *stroked = true;
        break;
    }
    // line style.
    switch (states->currentDeviceContext.stroke_mode & 0x000000FF) {
    case U_PS_SOLID:
        break;
    case U_PS_DASH:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f\" ", dash_len, dash_len);
        break;
    case U_PS_DOT:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f\" ", dot_len, dot_len);
        break;
    case U_PS_DASHDOT:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f,%.4f,%.4f\" ", dash_len,
                dash_len, dot_len, dash_len);
        break;
    case U_PS_DASHDOTDOT:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\" ",
                dash_len, dash_len, dot_len, dot_len, dot_len, dash_len);
        break;
    case U_PS_INSIDEFRAME:
    case U_PS_USERSTYLE:
    case U_PS_ALTERNATE:
    default:
        // partial
        break;
    }
    // line cap.
    switch (states->currentDeviceContext.stroke_mode & 0x00000F00) {
    case U_PS_ENDCAP_ROUND:
        fprintf(out, " stroke-linecap=\"round\" ");
        break;
    case U_PS_ENDCAP_SQUARE:
        fprintf(out, " stroke-linecap=\"square\" ");
        break;
    case U_PS_ENDCAP_FLAT:
        fprintf(out, " stroke-linecap=\"butt\" ");
        break;
    default:
        break;
    }
    // line join.
    switch (states->currentDeviceContext.stroke_mode & 0x0000F000) {
    case U_PS_JOIN_ROUND:
        fprintf(out, " stroke-linejoin=\"round\" ");
        break;
    case U_PS_JOIN_BEVEL:
        fprintf(out, " stroke-linejoin=\"bevel\" ");
        break;
    case U_PS_JOIN_MITER:
        fprintf(out, " stroke-linejoin=\"miter\" ");
        break;
    default:
        break;
    }
}
void text_draw(const char *contents, FILE *out, drawingStates *states,
               uint8_t type) {
    PU_EMRTEXT pemt =
        (PU_EMRTEXT)(contents + sizeof(U_EMREXTTEXTOUTA) - sizeof(U_EMRTEXT));
    returnOutOfEmf(pemt);

    uint8_t *string;
    size_t string_size;
    if (type == UTF_16) {
        returnOutOfEmf((uint64_t)contents + (uint64_t)pemt->offString +
                       2 * (uint64_t)pemt->nChars);
        string =
            (uint8_t *)U_Utf16leToUtf8((uint16_t *)(contents + pemt->offString),
                                       pemt->nChars, &string_size);
    } else {
        returnOutOfEmf((uint64_t)contents + (uint64_t)pemt->offString);
        string = (uint8_t *)(contents + pemt->offString);
    }
    int i = 0;
    while (string[i] != 0x0) {
        if (string[i] < 0x20 && string[i] != 0x09 && string[i] != 0x0A &&
            string[i] != 0x0B && string[i] != 0x09) {
            string[i] = 0x20;
        }
        if (type == ASCII && string[i] > 0x7F) {
            string[i] = 0x20;
        }
        i++;
    }
    fprintf(out, "<%stext ", states->nameSpaceString);
    POINT_D Org = point_cal(states, (double)pemt->ptlReference.x,
                            (double)pemt->ptlReference.y);
    double font_height = fabs((double)states->currentDeviceContext.font_height *
                              states->scalingY);
    if (states->currentDeviceContext.font_family != NULL)
        fprintf(out, "font-family=\"%s\" ",
                states->currentDeviceContext.font_family);
    fprintf(out, "fill=\"#%02X%02X%02X\" ",
            states->currentDeviceContext.text_red,
            states->currentDeviceContext.text_green,
            states->currentDeviceContext.text_blue);
    int orientation;
    if (states->scalingY > 0) {
        orientation = -1;
    } else {
        orientation = 1;
    }

    if (states->currentDeviceContext.font_escapement != 0) {
        fprintf(out, "transform=\"rotate(%d, %.4f, %.4f) translate(0, %.4f)\" ",
                (orientation *
                 (int)states->currentDeviceContext.font_escapement / 10),
                Org.x, (Org.y + font_height * 0.9), font_height * 0.9);
    }

    if (states->text_layout == U_LAYOUT_RTL) {
        fprintf(out, "writing-mode=\"rl-tb\" ");
    }

    if (states->currentDeviceContext.font_italic) {
        fprintf(out, "font-style=\"italic\" ");
    }

    if (states->currentDeviceContext.font_underline &&
        states->currentDeviceContext.font_strikeout) {
        fprintf(out, "text-decoration=\"line-through,underline\" ");
    } else if (states->currentDeviceContext.font_underline) {
        fprintf(out, "text-decoration=\"underline\" ");
    } else if (states->currentDeviceContext.font_strikeout) {
        fprintf(out, "text-decoration=\"line-through\" ");
    }

    if (states->currentDeviceContext.font_weight != 0)
        fprintf(out, "font-weight=\"%d\" ",
                states->currentDeviceContext.font_weight);

    // horizontal position
    uint16_t align = states->currentDeviceContext.text_align;
    if ((align & U_TA_CENTER) == U_TA_CENTER) {
        fprintf(out, "text-anchor=\"middle\" ");
    } else if ((align & U_TA_CENTER2) == U_TA_CENTER2) {
        fprintf(out, "text-anchor=\"middle\" ");
    } else if ((align & U_TA_RIGHT) == U_TA_RIGHT) {
        fprintf(out, "text-anchor=\"end\" ");
    } else {
        fprintf(out, "text-anchor=\"start\" ");
    }
    // vertical position
    if ((align & U_TA_BOTTOM) == U_TA_BOTTOM) {
        fprintf(out, "x=\"%.4f\" y=\"%.4f\" ", Org.x, Org.y);
    } else if ((align & U_TA_BASELINE) == U_TA_BASELINE) {
        fprintf(out, "x=\"%.4f\" y=\"%.4f\" ", Org.x, Org.y);
    } else {
        fprintf(out, "x=\"%.4f\" y=\"%.4f\" ", Org.x,
                Org.y + font_height * 0.9);
    }
    fprintf(out, "font-size=\"%.4f\" ", font_height);
    fprintf(out, ">");
    fprintf(out, "<![CDATA[%s]]>", string);
    fprintf(out, "</%stext>\n", states->nameSpaceString);
    if (type == UTF_16) {
        free(string);
    }
}
void transform_draw(drawingStates *states, FILE *out) {
    // transformation could be set inside path.
    // If we are in a path, we do nothing here.
    // However the transformation is set in BEGINPATH or ENDPATH.
    // The "pre" parsing is used to determine if such cases can occure
    // and records transformations that doesn't occure where the record is
    // declared.
    // (function U_emf_onerec_analyse)
    if (states->inPath)
        return;
    if (states->transform_open) {
        fprintf(out, "</%sg>\n", states->nameSpaceString);
    }
    fprintf(out, "<%sg transform=\"matrix(%.4f %.4f %.4f %.4f %.4f %.4f)\">\n",
            states->nameSpaceString,
            (double)states->currentDeviceContext.worldTransform.eM11,
            (double)states->currentDeviceContext.worldTransform.eM12,
            (double)states->currentDeviceContext.worldTransform.eM21,
            (double)states->currentDeviceContext.worldTransform.eM22,
            (double)states->currentDeviceContext.worldTransform.eDx *
                (double)states->scalingX,
            (double)states->currentDeviceContext.worldTransform.eDy *
                (double)states->scalingY);
    states->transform_open = true;
}
bool transform_set(drawingStates *states, U_XFORM xform, uint32_t iMode) {
    switch (iMode) {
    case U_MWT_IDENTITY: {
        setTransformIdentity(states);
        return true;
    }
    case U_MWT_LEFTMULTIPLY: {
        float a11 = xform.eM11;
        float a12 = xform.eM12;
        float a13 = 0.0;
        float a21 = xform.eM21;
        float a22 = xform.eM22;
        float a23 = 0.0;
        float a31 = xform.eDx;
        float a32 = xform.eDy;
        float a33 = 1.0;

        float b11 = states->currentDeviceContext.worldTransform.eM11;
        float b12 = states->currentDeviceContext.worldTransform.eM12;
        // float b13 = 0.0;
        float b21 = states->currentDeviceContext.worldTransform.eM21;
        float b22 = states->currentDeviceContext.worldTransform.eM22;
        // float b23 = 0.0;
        float b31 = states->currentDeviceContext.worldTransform.eDx;
        float b32 = states->currentDeviceContext.worldTransform.eDy;
        // float b33 = 1.0;

        float c11 = a11 * b11 + a12 * b21 + a13 * b31;
        ;
        float c12 = a11 * b12 + a12 * b22 + a13 * b32;
        ;
        // float c13 = a11*b13 + a12*b23 + a13*b33;;
        float c21 = a21 * b11 + a22 * b21 + a23 * b31;
        ;
        float c22 = a21 * b12 + a22 * b22 + a23 * b32;
        ;
        // float c23 = a21*b13 + a22*b23 + a23*b33;;
        float c31 = a31 * b11 + a32 * b21 + a33 * b31;
        ;
        float c32 = a31 * b12 + a32 * b22 + a33 * b32;
        ;
        // float c33 = a31*b13 + a32*b23 + a33*b33;;

        states->currentDeviceContext.worldTransform.eM11 = c11;
        ;
        states->currentDeviceContext.worldTransform.eM12 = c12;
        ;
        states->currentDeviceContext.worldTransform.eM21 = c21;
        ;
        states->currentDeviceContext.worldTransform.eM22 = c22;
        ;
        states->currentDeviceContext.worldTransform.eDx = c31;
        states->currentDeviceContext.worldTransform.eDy = c32;
        return true;
    }
    case U_MWT_RIGHTMULTIPLY: {
        float a11 = states->currentDeviceContext.worldTransform.eM11;
        float a12 = states->currentDeviceContext.worldTransform.eM12;
        float a13 = 0.0;
        float a21 = states->currentDeviceContext.worldTransform.eM21;
        float a22 = states->currentDeviceContext.worldTransform.eM22;
        float a23 = 0.0;
        float a31 = states->currentDeviceContext.worldTransform.eDx;
        float a32 = states->currentDeviceContext.worldTransform.eDy;
        float a33 = 1.0;

        float b11 = xform.eM11;
        float b12 = xform.eM12;
        // float b13 = 0.0;
        float b21 = xform.eM21;
        float b22 = xform.eM22;
        // float b23 = 0.0;
        float b31 = xform.eDx;
        float b32 = xform.eDy;
        // float b33 = 1.0;

        float c11 = a11 * b11 + a12 * b21 + a13 * b31;
        ;
        float c12 = a11 * b12 + a12 * b22 + a13 * b32;
        ;
        // float c13 = a11*b13 + a12*b23 + a13*b33;;
        float c21 = a21 * b11 + a22 * b21 + a23 * b31;
        ;
        float c22 = a21 * b12 + a22 * b22 + a23 * b32;
        ;
        // float c23 = a21*b13 + a22*b23 + a23*b33;;
        float c31 = a31 * b11 + a32 * b21 + a33 * b31;
        ;
        float c32 = a31 * b12 + a32 * b22 + a33 * b32;
        ;
        // float c33 = a31*b13 + a32*b23 + a33*b33;;

        states->currentDeviceContext.worldTransform.eM11 = c11;
        ;
        states->currentDeviceContext.worldTransform.eM12 = c12;
        ;
        states->currentDeviceContext.worldTransform.eM21 = c21;
        ;
        states->currentDeviceContext.worldTransform.eM22 = c22;
        ;
        states->currentDeviceContext.worldTransform.eDx = c31;
        states->currentDeviceContext.worldTransform.eDy = c32;
        return true;
    }
    case U_MWT_SET: {
        states->currentDeviceContext.worldTransform = xform;
        return true;
    }
    default:
        return false;
    }
}
void width_stroke(drawingStates *states, FILE *out, double width) {
    fprintf(out, "stroke-width=\"%.4f\" ", width * states->scaling);
}

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

static int mod_table[] = {0, 2, 1};


char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = calloc(*output_length, 2);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
