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
#include "EMFSVG.h"
#include "EMFSVG_private.h"
#include "EMFSVG_print.h"
#include "PMFSVG.h"

    //! \cond
#define UNUSED(x) (void)(x)

    /** 
      \brief save the current device context on the stack.
      \param states drawingStates object    
      */

    void saveDeviceContext(drawingStates * states){
        // create the new device context in the stack
        EMF_DEVICE_CONTEXT_STACK * new_entry = (EMF_DEVICE_CONTEXT_STACK *)calloc(1, sizeof(EMF_DEVICE_CONTEXT_STACK));
        copyDeviceContext(&(new_entry->DeviceContext), &(states->currentDeviceContext));
        // put the new entry on the stack
        new_entry->previous = states->DeviceContextStack;
        states->DeviceContextStack = new_entry;
    }

    void startPathDraw(drawingStates *states,
            FILE * out
            ){
        if (!(states->inPath)){
            fprintf(out, "<%spath d=\"M ", states->nameSpaceString);
            U_POINT pt;
            pt.x = states->cur_x;
            pt.y = states->cur_y;
            point_draw(states,pt,out);
        }
    }

    void addFormToStack(drawingStates *states){
        formStack * newForm = calloc(1, sizeof(formStack));
        FILE * NewFormStream = open_memstream(&newForm->form, &newForm->len);
        if (NewFormStream == NULL){
            return;
        }
        newForm->formStream = NewFormStream;
        newForm->prev = states->currentDeviceContext.clipStack;
        states->currentDeviceContext.clipStack = newForm;
    }

    void freeFormStack(formStack * stack){
        while (stack != NULL){
            fclose(stack->formStream);
            free(stack->form);
            formStack * tmp = stack;
            stack = stack->prev;
            free(tmp);
        }
        return;
    }

    formStack * cpFormStack(formStack * stack){

        if (stack == NULL)
            return NULL;
        formStack * ret = calloc(1, sizeof(formStack));
        FILE * NewFormStream = open_memstream(&ret->form, &ret->len);
        ret->formStream = NewFormStream;
        ret->drawn = stack->drawn; 
        ret->id = stack->id;
        if (stack->len != 0)
            fprintf(ret->formStream, "%s", stack->form);

        formStack * cur = ret;
        stack = stack->prev;
        while (stack != NULL){
            formStack * new = calloc(1, sizeof(formStack));
            FILE * NewFormStream = open_memstream(&new->form, &new->len);
            new->formStream = NewFormStream;
            new->drawn = stack->drawn; 
            new->id = stack->id;
            if (stack->len != 0)
                fprintf(new->formStream, "%s", stack->form);
            stack = stack->prev;
            cur->prev = new;
            formStack * cur = new;
        }
        return ret;
    }



    void endPathDraw(drawingStates *states, FILE * out){
        if (!(states->inPath)){
            fprintf(out,"\" ");
            bool filled;
            bool stroked;
            stroke_draw(states, out, &filled, &stroked);
            fprintf(out," fill=\"none\" />\n");
        }
    }

    int get_id(drawingStates * states){
        states->uniqId++;
        return states->uniqId;
    }

    void fill_draw(drawingStates *states, FILE * out, bool * filled, bool * stroked){
        if (states->verbose){fill_print(states);}
        char * fill_rule = calloc(40, sizeof(char));
        switch(states->currentDeviceContext.fill_mode){
            case(U_ALTERNATE):
                sprintf(fill_rule, "fill-rule:\"evenodd\" ");
                break;
            case(U_WINDING):
                sprintf(fill_rule, "fill-rule:\"nonzero\" ");
                break;
            default:
                sprintf(fill_rule, " ");
                break;
        }
        switch(states->currentDeviceContext.fill_mode){
            case U_BS_SOLID:
                *filled = true;
                fprintf(out, "%s", fill_rule);
                fprintf(out, "fill=\"#%02X%02X%02X\" ", 
                        states->currentDeviceContext.fill_red,
                        states->currentDeviceContext.fill_green,
                        states->currentDeviceContext.fill_blue
                       );
                break;
            case U_BS_NULL:
                fprintf(out, "fill=\"none\" " );
                *filled = true;
                break;
            case U_BS_HATCHED:
                break;
            case U_BS_PATTERN:
                break;
            case U_BS_INDEXED:
                break;
            case U_BS_DIBPATTERN:
                break;
            case U_BS_DIBPATTERNPT:
                break;
            case U_BS_PATTERN8X8:
                break;
            case U_BS_DIBPATTERN8X8:
                break;
            case U_BS_MONOPATTERN:
                break;
            default:
                break;
            } 
            free(fill_rule);
            return;
    }

    void stroke_draw(drawingStates *states, FILE * out, bool * filled, bool * stroked){
        if (states->verbose){stroke_print(states);}
        switch(states->currentDeviceContext.stroke_mode){
            case U_PS_SOLID:
                fprintf(out, "stroke=\"#%02X%02X%02X\" stroke-width=\"%f\" ", 
                        states->currentDeviceContext.stroke_red,
                        states->currentDeviceContext.stroke_green,
                        states->currentDeviceContext.stroke_blue,
                        1.0
                        //states->currentDeviceContext.stroke_width
                       );
                *stroked = true;
                break;
            case U_PS_DASH:
                break;
            case U_PS_DOT:
                break;
            case U_PS_DASHDOT:
                break;
            case U_PS_DASHDOTDOT:
                break;
            case U_PS_NULL:
                fprintf(out, "stroke=\"none\" " );
                fprintf(out, "stroke-width=\"0.0\" " );
                *stroked = true;
                break;
            case U_PS_INSIDEFRAME:
                //partial
                fprintf(out, "stroke=\"#%02X%02X%02X\" stroke-width=\"%f\" ", 
                        states->currentDeviceContext.stroke_red,
                        states->currentDeviceContext.stroke_green,
                        states->currentDeviceContext.stroke_blue,
                        1.0
                        //states->currentDeviceContext.stroke_width
                       );
                *stroked = true;
                break;
            case U_PS_USERSTYLE:
                break;
            case U_PS_ALTERNATE:
                break;
            case U_PS_ENDCAP_SQUARE:
                break;
            case U_PS_ENDCAP_FLAT:
                break;
            case U_PS_JOIN_BEVEL:
                break;
            case U_PS_JOIN_MITER:
                break;
            case U_PS_GEOMETRIC:
                *stroked = true;
                fprintf(out, "stroke=\"#%02X%02X%02X\" stroke-width=\"%f\" ", 
                        states->currentDeviceContext.stroke_red,
                        states->currentDeviceContext.stroke_green,
                        states->currentDeviceContext.stroke_blue,
                        1.0
                        //states->currentDeviceContext.stroke_width
                       );
                break;
            default:
                break;
        }
    }

    void newPathStruct(drawingStates *states){
        pathStack * new_entry = calloc(1, sizeof(pathStack));
        if (states->emfStructure.pathStack == NULL){
            states->emfStructure.pathStack = new_entry;
            states->emfStructure.pathStackLast = new_entry;
        }
        else{
            states->emfStructure.pathStackLast->next = new_entry;
            states->emfStructure.pathStackLast = new_entry;
        }
    }

    void setTransformIdentity(drawingStates * states){
        states->currentDeviceContext.worldTransform.eM11 = 1.0;
        states->currentDeviceContext.worldTransform.eM12 = 0.0;
        states->currentDeviceContext.worldTransform.eM21 = 0.0;
        states->currentDeviceContext.worldTransform.eM22 = 1.0;
        states->currentDeviceContext.worldTransform.eDx  = 0.0;
        states->currentDeviceContext.worldTransform.eDy  = 0.0;
    }

    void copyDeviceContext(EMF_DEVICE_CONTEXT *dest, EMF_DEVICE_CONTEXT *src){
        // copy simple data (int, double...)
        *dest = *src;

        // copy more complex data (pointers...)
        if (src->font_name != NULL){
            dest->font_name = (char *)calloc(strlen(src->font_name) + 1, sizeof(char)); 
            strcpy(dest->font_name, src->font_name);
        }
        if (src->font_family != NULL){
            dest->font_family = (char *)calloc(strlen(src->font_family) + 1, sizeof(char));
            strcpy(dest->font_family, src->font_family);
        }
        dest->clipStack = cpFormStack(src->clipStack);
    }

    void freeDeviceContext(EMF_DEVICE_CONTEXT *dc){
        if(dc != NULL){
            if(dc->font_name != NULL)
                free(dc->font_name);
            if(dc->font_family != NULL)
                free(dc->font_family);
            freeFormStack(dc->clipStack);
        }
    }

    void restoreDeviceContext(drawingStates * states, int32_t index ){
        EMF_DEVICE_CONTEXT device_context_to_restore;
        EMF_DEVICE_CONTEXT_STACK * stack_entry = states->DeviceContextStack;
        // we recover the 'abs(index)' element of the stack
        for(int i=-1;i>index;i--){
            if (stack_entry != NULL){
                stack_entry = stack_entry->previous;
            }
        }
        // we copy it as the current device context
        freeDeviceContext(&(states->currentDeviceContext));
        states->currentDeviceContext = (EMF_DEVICE_CONTEXT){ 0 };
        copyDeviceContext(&(states->currentDeviceContext), &(stack_entry->DeviceContext));
    }

    void freeObject(drawingStates * states, uint16_t index){
        if(states->objectTable[index].font_name != NULL)
            free(states->objectTable[index].font_name);
        if(states->objectTable[index].font_family != NULL)
            free(states->objectTable[index].font_family);
    }
    void freeObjectTable(drawingStates * states){
        for(uint16_t i = 0; i < (states->objectTableSize + 1); i++){
            freeObject(states, i);
        }
    }

    void freeDeviceContextStack(drawingStates * states){
        EMF_DEVICE_CONTEXT_STACK * stack_entry = states->DeviceContextStack;
        while (stack_entry != NULL){
            EMF_DEVICE_CONTEXT_STACK * next_entry = stack_entry->previous;
            freeDeviceContext(&(stack_entry->DeviceContext));
            free(stack_entry);
            stack_entry = next_entry;
        }
    }

    /* one needed prototype */
    void U_swap4(void *ul, unsigned int count);
    //! \endcond

    /**
      \brief Print rect and rectl objects from Upper Left and Lower Right corner points.
      \param rect U_RECTL object
      */
    void rectl_draw(drawingStates *states, FILE * out, U_RECTL rect){
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

    POINT_D point_cal(drawingStates *states, double x, double y)
    {
        POINT_D ret;
        ret.x = states->offsetX + ((double)x * (double)states->scalingX);
        ret.y = states->offsetY + ((double)y * (double)states->scalingY);
        return ret;
    }
    /**
      \brief Print a pointer to a U_POINT16 object
      \param pt pointer to a U_POINT16 object
      Warning - WMF data may contain unaligned U_POINT16, do not call
      this routine with a pointer to such data!
      */
    void point16_draw(
            drawingStates *states,
            U_POINT16 pt,
            FILE * out
            ){
        POINT_D ptd = point_cal(states, (double)pt.x, (double)pt.y);
        states->cur_x = pt.x;
        states->cur_y = pt.y;
        fprintf(out, "%.2f %.2f ", ptd.x ,ptd.y);
    } 

    void point_draw(
            drawingStates *states,
            U_POINT pt,
            FILE * out
            ){
        POINT_D ptd = point_cal(states, (double)pt.x, (double)pt.y);
        states->cur_x = pt.x;
        states->cur_y = pt.y;
        fprintf(out, "%.2f %.2f ", ptd.x ,ptd.y);
    } 

    void transform_draw(
            drawingStates *states,
            FILE * out
            ){
        fprintf(out, "<%sg transform=\"matrix(%f %f %f %f %f %f)\">\n",  
                states->nameSpaceString,
                (double)states->currentDeviceContext.worldTransform.eM11,
                (double)states->currentDeviceContext.worldTransform.eM12,
                (double)states->currentDeviceContext.worldTransform.eM21,
                (double)states->currentDeviceContext.worldTransform.eM22,
                (double)states->currentDeviceContext.worldTransform.eDx * (double)states->scalingX,
                (double)states->currentDeviceContext.worldTransform.eDy * (double)states->scalingY
                );
        states->transform_open = true;
    }

    // Functions with the same form starting with U_EMRPOLYBEZIER16_draw
    void cubic_bezier16_draw(const char *name, const char *contents, FILE *out, drawingStates *states, int startingPoint){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16) (contents);
        startPathDraw(states, out);
        PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
        if (startingPoint == 1){
            fprintf(out, "M ");
            point16_draw(states, papts[0], out);
        }
        const int ctrl1 = (0 + startingPoint) % 3;
        const int ctrl2 = (1 + startingPoint) % 3;
        const int to = (2 + startingPoint) % 3;
        for(i = startingPoint; i<pEmr->cpts; i++){
            if (( i % 3 ) == ctrl1) {
                fprintf(out, "C ");
                point16_draw(states, papts[i], out);
            }
            else if (( i % 3 ) == ctrl2) {
                point16_draw(states, papts[i], out);
            }
            else if (( i % 3 ) == to) {
                point16_draw(states, papts[i], out);
            }
        }
        endPathDraw(states, out);
    } 

    void arc_draw(const char *contents, FILE *out, drawingStates *states){
        PU_EMRARC pEmr = (PU_EMRARC) (contents);
        startPathDraw(states, out);
        fprintf(out, "A ");
        point_draw(states, pEmr->ptlStart, out);
        point_draw(states, pEmr->ptlEnd, out);
        endPathDraw(states, out);
    } 



    void cubic_bezier_draw(const char *name, const char *contents, FILE *out, drawingStates *states, int startingPoint){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYBEZIER pEmr = (PU_EMRPOLYBEZIER) (contents);
        startPathDraw(states, out);
        PU_POINT papts = (PU_POINT)(&(pEmr->aptl));
        if (startingPoint == 1){
            fprintf(out, "M ");
            point_draw(states, papts[0], out);
        }
        const int ctrl1 = (0 + startingPoint) % 3;
        const int ctrl2 = (1 + startingPoint) % 3;
        const int to = (2 + startingPoint) % 3;
        for(i = startingPoint; i<pEmr->cptl; i++){
            if (( i % 3 ) == ctrl1) {
                fprintf(out, "C ");
                point_draw(states, papts[i], out);
            }
            else if (( i % 3 ) == ctrl2) {
                point_draw(states, papts[i], out);
            }
            else if (( i % 3 ) == to) {
                point_draw(states, papts[i], out);
            }
        }
        endPathDraw(states, out);
    } 


    // Functions drawing a polyline
    void polyline16_draw(const char *name, const char *contents, FILE *out, drawingStates *states, bool polygon){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16) (contents);
        PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
        startPathDraw(states, out);
        for(i=0; i<pEmr->cpts; i++){
            if (polygon && i == 0){
                fprintf(out, "M ");
            }
            else{
                fprintf(out, "L ");
            }
            point16_draw(states, papts[i], out);
        }
        endPathDraw(states, out);
    } 

    void polyline_draw(const char *name, const char *contents, FILE *out, drawingStates *states, bool polygon){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYLINETO pEmr = (PU_EMRPOLYLINETO) (contents);
        startPathDraw(states, out);
        for(i=0; i<pEmr->cptl; i++){
            if (polygon && i == 0){
                fprintf(out, "M ");
            }
            else{
                fprintf(out, "L ");
            }
            point_draw(states, pEmr->aptl[i], out);
        }
        endPathDraw(states, out);
    } 


    void moveto_draw(const char *name, const char *field1, const char *field2, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);
        point_draw(states,pEmr->pair,out);
    }

    void lineto_draw(const char *name, const char *field1, const char *field2, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);
        startPathDraw(states, out);
        fprintf(out, "L ");
        point_draw(states,pEmr->pair,out);
        endPathDraw(states, out);
    }

    void polypolygon16_draw(const char *name, const char *contents, FILE *out, drawingStates *states, bool polygon){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16) (contents);
        PU_POINT16 papts = (PU_POINT16)((char *)pEmr->aPolyCounts + sizeof(uint32_t)* pEmr->nPolys);

        int counter = 0;
        int polygon_index = 0;
        for(i=0; i<pEmr->cpts; i++){
            if(counter == 0){
                fprintf(out, "M ");
                point16_draw(states, papts[i], out);
            }
            else{
                fprintf(out, "L ");
                point16_draw(states, papts[i], out);
            }
            counter++;
            if (pEmr->aPolyCounts[polygon_index] == counter){
                if (polygon)
                    fprintf(out, "Z ");
                counter = 0;
                polygon_index++;
            }
        }
    } 

    void polypolygon_draw(const char *name, const char *contents, FILE *out, drawingStates *states, bool polygon){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16) (contents);
        PU_POINT papts = (PU_POINT)((char *)pEmr->aPolyCounts + sizeof(uint32_t)* pEmr->nPolys);

        int counter = 0;
        int polygon_index = 0;
        for(i=0; i<pEmr->cpts; i++){
            if(counter == 0){
                fprintf(out, "M ");
                point_draw(states, papts[i], out);
            }
            else{
                fprintf(out, "L ");
                point_draw(states, papts[i], out);
            }
            counter++;
            if (pEmr->aPolyCounts[polygon_index] == counter){
                if (polygon)
                    fprintf(out, "Z ");
                counter = 0;
                polygon_index++;
            }
        }
    } 

    void text_draw(const char *contents, FILE *out, drawingStates *states, uint8_t type){
        PU_EMREXTTEXTOUTW pEmr = (PU_EMREXTTEXTOUTW) (contents);
        PU_EMRTEXT pemt = (PU_EMRTEXT)(contents + sizeof(U_EMREXTTEXTOUTA) - sizeof(U_EMRTEXT));
        char *string;
        if(type == UTF_16){
            string = U_Utf16leToUtf8((uint16_t *)(contents + pemt->offString), pemt->nChars, NULL);
        }
        else{
            string = (char *)(contents + pemt->offString);
        }
        fprintf(out, "<%stext ", states->nameSpaceString);
        POINT_D Org = point_cal(states, (double)pemt->ptlReference.x, (double)pemt->ptlReference.y);
        pemt->ptlReference;
        double font_height = fabs((double)states->currentDeviceContext.font_height * states->scalingY);
        if( states->currentDeviceContext.font_family != NULL)
            fprintf(out, "font-family=\"%s\" ", states->currentDeviceContext.font_family);
        fprintf(out, "fill=\"#%02X%02X%02X\" ", 
                states->currentDeviceContext.text_red,
                states->currentDeviceContext.text_green,
                states->currentDeviceContext.text_blue
               );
        int orientation;
        if (states->scalingY > 0){
            orientation = -1;
        }
        else{
            orientation = 1;
        }

        if(states->currentDeviceContext.font_escapement != 0){
            fprintf(out, "transform=\"rotate(%d, %f, %f) translate(0, %f)\" ", (orientation * (int)states->currentDeviceContext.font_escapement / 10), Org.x, (Org.y + font_height * 0.9), font_height * 0.9);
        }

        if(states->currentDeviceContext.font_italic){
            fprintf(out, "font-style=\"italic\" ");
        }

        if(states->currentDeviceContext.font_underline && states->currentDeviceContext.font_strikeout){
            fprintf(out, "text-decoration=\"line-through,underline\" ");
        }
        else if(states->currentDeviceContext.font_underline){
            fprintf(out, "text-decoration=\"underline\" ");
        }
        else if(states->currentDeviceContext.font_strikeout){
            fprintf(out, "text-decoration=\"line-through\" ");
        }

        if(states->currentDeviceContext.font_weight != 0)
            fprintf(out, "font-weight=\"%d\" ", states->currentDeviceContext.font_weight);

        // horizontal position
        uint16_t align = states->currentDeviceContext.text_align;
        if((align & U_TA_CENTER) == U_TA_CENTER){
            fprintf(out, "text-anchor=\"middle\" ");
        }
        else if ((align & U_TA_CENTER2) == U_TA_CENTER2){
            fprintf(out, "text-anchor=\"middle\" ");
        }
        else if ((align & U_TA_RIGHT) == U_TA_RIGHT){
            fprintf(out, "text-anchor=\"end\" ");
        }
        else {
            fprintf(out, "text-anchor=\"start\" ");
        }
        // vertical position
        if((align & U_TA_BOTTOM) == U_TA_BOTTOM){
            fprintf(out, "x=\"%f\" y=\"%f\" ", Org.x, Org.y);
        }
        else if ((align & U_TA_BASELINE) == U_TA_BASELINE){
            fprintf(out, "x=\"%f\" y=\"%f\" ", Org.x, Org.y);
        }
        else {
            fprintf(out, "x=\"%f\" y=\"%f\" ", Org.x, Org.y + font_height * 0.9);
        }
        fprintf(out, "font-size=\"%f\" ", font_height);
        fprintf(out, ">");
        fprintf(out, "<![CDATA[%s]]>", string);
        fprintf(out, "</%stext>\n", states->nameSpaceString);
        if(type == UTF_16){
            free(string);
        }
    }


    void U_EMRNOTIMPLEMENTED_draw(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        //if (states->verbose){U_EMRNOTIMPLEMENTED_print(contents, states);}
        UNUSED(contents);
    }

    void U_EMRHEADER_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRHEADER_print(contents, states);}
        char *string;
        int  p1len;

        PU_EMRHEADER pEmr = (PU_EMRHEADER)(contents);
        if(pEmr->offDescription){
            string = U_Utf16leToUtf8((uint16_t *)((char *) pEmr + pEmr->offDescription), pEmr->nDescription, NULL);
            free(string);
            p1len = 2 + 2*wchar16len((uint16_t *)((char *) pEmr + pEmr->offDescription));
            string = U_Utf16leToUtf8((uint16_t *)((char *) pEmr + pEmr->offDescription + p1len), pEmr->nDescription, NULL);
            free(string);
        }
        // object table allocation
        // allocate one more to directly use object indexes (starts at 1 and not 0)
        states->objectTable = calloc(pEmr->nHandles + 1, sizeof(emfGraphObject));
        states->objectTableSize = pEmr->nHandles;

        double ratioXY = (double)(pEmr->rclBounds.right  - pEmr->rclBounds.left) / 
            (double)(pEmr->rclBounds.bottom  - pEmr->rclBounds.top);

        if ((states->imgHeight != 0) && (states->imgWidth != 0)){
            double tmpWidth = states->imgHeight * ratioXY;
            double tmpHeight = states->imgWidth / ratioXY;
            if (tmpWidth > states->imgWidth){
                states->imgHeight = tmpHeight;
            }
            else{
                states->imgWidth = tmpWidth;
            }
        }
        else if ( states->imgHeight != 0 ){
            states->imgWidth = states->imgHeight * ratioXY;
        }
        else if (states->imgWidth != 0){
            states->imgHeight = states->imgWidth / ratioXY;
        }
        else {
            states->imgWidth = pEmr->szlDevice.cx;
            states->imgHeight = states->imgWidth / ratioXY;
        }

        // set scaling for original resolution
        states->scaling = states->imgWidth / (double)(pEmr->rclBounds.right  - pEmr->rclBounds.left);

        states->scalingX = states->scaling;
        states->scalingY = states->scaling;

        states->pxPerMm = (double)pEmr->szlDevice.cx / (double)pEmr->szlMillimeters.cx;

        if (states->svgDelimiter){
            fprintf(out, "<?xml version=\"1.0\"  encoding=\"UTF-8\" standalone=\"no\"?>\n"); 
            fprintf(out, "<%ssvg version=\"1.1\" ", 
                    states->nameSpaceString);
            fprintf(out, "xmlns=\"http://www.w3.org/2000/svg\" ");
            if ((states->nameSpace != NULL) && (strlen(states->nameSpace) != 0)){
                fprintf(out, "xmlns:%s=\"http://www.w3.org/2000/svg\" ", states->nameSpace);
            }
                   fprintf(out, "width=\"%d\" height=\"%d\">\n",
                    (int)states->imgWidth, 
                    (int)states->imgHeight);
        }

        // set origin
        states->originX = -1 * (double)pEmr->rclBounds.left * states->scalingX;
        states->originY = -1 * (double)pEmr->rclBounds.top * states->scalingY;
        states->offsetX = states->originX;
        states->offsetY = states->originY;

        fprintf(out, "<%sg>\n", states->nameSpaceString);
    }

    void U_EMRPOLYBEZIER_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYBEZIER_print(contents, states);}
        cubic_bezier_draw("U_EMRPOLYBEZIER", contents, out, states, 1);
    } 

    void U_EMRPOLYGON_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYGON_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath ", states->nameSpaceString);
            if (states->clipSet)
                fprintf(out, " clip-path=\"url(#clip-%d)\" ", states->clipId);
            fprintf(out, "d=\"");
        }
        bool ispolygon = true;
        polyline_draw("U_EMRPOLYGON16", contents, out, states, ispolygon);

        if (localPath){
            states->inPath = false;
            fprintf(out, "Z\" ");
            bool filled = false;
            bool stroked = false;
            stroke_draw(states, out, &filled, &stroked);
            fill_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");

            fprintf(out, "/><!-- shit -->\n");
        }
    } 

    void U_EMRPOLYLINE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYLINE_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath ", states->nameSpaceString);
            if (states->clipSet)
                fprintf(out, " clip-path=\"url(#clip-%d)\" ", states->clipId);
            fprintf(out, "d=\"");
        }
        bool ispolygon = true;
        polyline_draw("U_EMRPOLYLINE", contents, out, states, ispolygon);
        if (localPath){
            states->inPath = false;
            //fprintf(out, "Z\" ");
            fprintf(out, "\" ");
            bool filled = false;
            bool stroked = false;
            stroke_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");
            fprintf(out, "/>\n");
        }
    } 

    void U_EMRPOLYBEZIERTO_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYBEZIERTO_print(contents, states);}
        cubic_bezier_draw("U_EMRPOLYBEZIER", contents, out, states, false);
    } 

    void U_EMRPOLYLINETO_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYLINETO_print(contents, states);}
        polyline_draw("U_EMRPOLYLINETO", contents, out, states, false);
    } 

    void U_EMRPOLYPOLYLINE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYPOLYLINE_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath ", states->nameSpaceString);
            if (states->clipSet)
                fprintf(out, " clip-path=\"url(#clip-%d)\" ", states->clipId);
            fprintf(out, "d=\"");
        }
        bool ispolygon = false;
        polypolygon_draw("U_EMRPOLYPOLYGON16", contents, out, states, false);

        if (localPath){
            states->inPath = false;
            fprintf(out, "\" ");
            bool filled = false;
            bool stroked = false;
            stroke_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");

            fprintf(out, "/>\n");
        }
    } 

    void U_EMRPOLYPOLYGON_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYPOLYLINE_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath d=\"", states->nameSpaceString);
        }
        bool ispolygon = true;
        polypolygon_draw("U_EMRPOLYPOLYGON", contents, out, states, ispolygon);

        if (localPath){
            states->inPath = false;
            fprintf(out, "\" ");
            bool filled = false;
            bool stroked = false;
            fill_draw(states, out, &filled, &stroked);
            stroke_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");

            fprintf(out, "/>\n");
        }
    } 

    void U_EMRSETWINDOWEXTEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSETWINDOWEXTEX_print(contents, states);}

        PU_EMRSETWINDOWEXTEX pEmr = (PU_EMRSETVIEWPORTEXTEX)(contents);
        states->scalingX = (double)states->imgWidth  / (double)pEmr->szlExtent.cx;
        states->scalingY = (double)states->imgHeight / (double)pEmr->szlExtent.cy;
        states->offsetX = states->originX;
        states->offsetY = states->originY;
    } 

    void U_EMRSETWINDOWORGEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSETWINDOWORGEX_print(contents, states);}

        PU_EMRSETWINDOWORGEX pEmr = (PU_EMRSETWINDOWORGEX)(contents);
        states->offsetX = -1 * (double)pEmr->ptlOrigin.x * states->scalingX + states->offsetX;
        states->offsetY = -1 * (double)pEmr->ptlOrigin.y * states->scalingY + states->offsetY;

    } 

    void U_EMRSETVIEWPORTEXTEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSETVIEWPORTEXTEX_print(contents, states);}
        PU_EMRSETVIEWPORTEXTEX pEmr = (PU_EMRSETVIEWPORTEXTEX)(contents);

        states->imgWidth  =  pEmr->szlExtent.cx * states->scaling;
        states->imgHeight =  pEmr->szlExtent.cy * states->scaling;
    } 

    void U_EMRSETVIEWPORTORGEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_UNUSED;
        if (states->verbose){U_EMRSETVIEWPORTORGEX_print(contents, states);}
        //PU_EMRSETVIEWPORTORGEX pEmr = (PU_EMRSETVIEWPORTORGEX)(contents);
    } 

    void U_EMRSETBRUSHORGEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETBRUSHORGEX_print(contents, states);}
    } 

    void U_EMREOF_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMREOF_print(contents, states);}
        PU_EMREOF pEmr = (PU_EMREOF)(contents);
        if(states->transform_open){
            fprintf(out, "</%sg>\n",  states->nameSpaceString);
        }
        fprintf(out, "</%sg>\n", states->nameSpaceString);
        if(states->svgDelimiter)
            fprintf(out, "</%ssvg>\n", states->nameSpaceString);
    } 

    void U_EMRSETPIXELV_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETPIXELV_print(contents, states);}
        PU_EMRSETPIXELV pEmr = (PU_EMRSETPIXELV)(contents);
    } 

    void U_EMRSETMAPPERFLAGS_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETMAPPERFLAGS_print(contents, states);}
        PU_EMRSETMAPPERFLAGS pEmr = (PU_EMRSETMAPPERFLAGS)(contents);
    } 

    void U_EMRSETMAPMODE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents);
        switch(pEmr->iMode){
            case U_MM_TEXT:
                states->scalingX = states->scaling * 1;
                states->scalingY = states->scaling * 1;
                states->offsetX = 0;
                states->offsetY = 0;
                break;
            case U_MM_LOMETRIC:
                // convert to 0.1 mm to pixel and invert Y
                states->scalingX = states->scaling * states->pxPerMm * 0.1 * 1;
                states->scalingY = states->scaling * states->pxPerMm * 0.1 * -1;
                states->offsetX = 0;
                states->offsetY = states->imgHeight;
                break;
            case U_MM_HIMETRIC:
                // convert to 0.01 mm to pixel and invert Y
                states->scalingX = states->scaling * states->pxPerMm * 0.01 * 1;
                states->scalingY = states->scaling * states->pxPerMm * 0.01 * -1;
                states->offsetX = 0;
                states->offsetY = states->imgHeight;
                break;
            case U_MM_LOENGLISH:
                // convert to 0.01 inch to pixel and invert Y
                states->scalingX = states->scaling * states->pxPerMm * 0.01 * mmPerInch * 1;
                states->scalingY = states->scaling * states->pxPerMm * 0.01 * mmPerInch * -1;
                states->offsetX = 0;
                states->offsetY = states->imgHeight;
                break;
            case U_MM_HIENGLISH:
                // convert to 0.001 inch to pixel and invert Y
                states->scalingX = states->scaling * states->pxPerMm * 0.001 * mmPerInch * 1;
                states->scalingY = states->scaling * states->pxPerMm * 0.001 * mmPerInch * -1;
                states->offsetX = 0;
                states->offsetY = states->imgHeight;
                break;
            case U_MM_TWIPS:
                // convert to 1 twips to pixel and invert Y
                states->scalingX = states->scaling * states->pxPerMm / 1440 * mmPerInch * 1;
                states->scalingY = states->scaling * states->pxPerMm / 1440 * mmPerInch * -1;
                states->offsetX = 0;
                states->offsetY = states->imgHeight;
                break;
            case U_MM_ISOTROPIC:
                states->MapMode = U_MM_ISOTROPIC;
                break;
            case U_MM_ANISOTROPIC:
                states->MapMode = U_MM_ANISOTROPIC;
                break;
        }
        if (states->verbose){U_EMRSETMAPMODE_print(contents, states);}
    }

    void U_EMRSETBKMODE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSETBKMODE_print(contents, states);}
        PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents);
        states->currentDeviceContext.bk_mode = pEmr->iMode;
    }

    void U_EMRSETPOLYFILLMODE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRSETPOLYFILLMODE_print(contents, states);}
        PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents);
        states->currentDeviceContext.fill_polymode = pEmr->iMode; 
    }

    void U_EMRSETROP2_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETROP2_print(contents, states);}
    }

    void U_EMRSETSTRETCHBLTMODE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETSTRETCHBLTMODE_print(contents, states);}
    }

    void U_EMRSETTEXTALIGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSETTEXTALIGN_print(contents, states);}
        PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents);
        states->currentDeviceContext.text_align = pEmr->iMode;
    }

    void U_EMRSETCOLORADJUSTMENT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETCOLORADJUSTMENT_print(contents, states);}
        PU_EMRSETCOLORADJUSTMENT pEmr = (PU_EMRSETCOLORADJUSTMENT)(contents);
    }

    void U_EMRSETTEXTCOLOR_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSETTEXTCOLOR_print(contents, states);}
        PU_EMRSETTEXTCOLOR pEmr = (PU_EMRSETTEXTCOLOR)(contents);
        states->currentDeviceContext.text_red   = pEmr->crColor.Red;
        states->currentDeviceContext.text_blue  = pEmr->crColor.Blue;
        states->currentDeviceContext.text_green = pEmr->crColor.Green;
    }

    void U_EMRSETBKCOLOR_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSETBKCOLOR_print(contents, states);}
        PU_EMRSETBKCOLOR pEmr = (PU_EMRSETBKCOLOR)(contents);
        states->currentDeviceContext.bk_red   = pEmr->crColor.Red;
        states->currentDeviceContext.bk_blue  = pEmr->crColor.Blue;
        states->currentDeviceContext.bk_green = pEmr->crColor.Green;
    }

    void U_EMROFFSETCLIPRGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMROFFSETCLIPRGN_print(contents, states);}
        PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);

    } 

    void U_EMRMOVETOEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRMOVETOEX_print(contents, states);}

        if (states->inPath){
            fprintf(out, "M ");
            moveto_draw("U_EMRMOVETOEX", "ptl:","",contents, out, states);
        }
        else{
            PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);
            U_POINT pt = pEmr->pair;

            states->cur_x = pt.x;
            states->cur_y = pt.y;
        }
    } 

    void U_EMRSETMETARGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETMETARGN_print(contents, states);}
        UNUSED(contents);
    }

    void U_EMREXCLUDECLIPRECT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMREXCLUDECLIPRECT_print(contents, states);}
        addFormToStack(states);
        PU_EMRELLIPSE pEmr      = (PU_EMRELLIPSE)(   contents);
        FILE * stream = states->currentDeviceContext.clipStack->formStream;
        fprintf(stream, "<%spath d\"", states->nameSpaceString);
        rectl_draw(states, stream, pEmr->rclBox);
        fprintf(stream, "fill=\"none\" draw=\"none\" />\n");
    }

    void U_EMRINTERSECTCLIPRECT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRINTERSECTCLIPRECT_print(contents, states);}
        addFormToStack(states);
        PU_EMRELLIPSE pEmr      = (PU_EMRELLIPSE)(   contents);
        FILE * stream = states->currentDeviceContext.clipStack->formStream;
        fprintf(stream, "<%spath d\"", states->nameSpaceString);

        rectl_draw(states, stream, pEmr->rclBox);
        fprintf(stream, "fill=\"none\" draw=\"none\" fill-rule=\"evenodd\" />\n");
    }

    void U_EMRSCALEVIEWPORTEXTEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSCALEVIEWPORTEXTEX_print(contents, states);}
    }


    void U_EMRSCALEWINDOWEXTEX_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSCALEWINDOWEXTEX_print(contents, states);}
    }

    void U_EMRSAVEDC_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRSAVEDC_print(contents, states);}
        saveDeviceContext(states);
        UNUSED(contents);
    }

    void U_EMRRESTOREDC_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRRESTOREDC_print(contents, states);}
        PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents);
        restoreDeviceContext(states, pEmr->iMode);

    }

    void U_EMRSETWORLDTRANSFORM_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRSETWORLDTRANSFORM_print(contents, states);}
        PU_EMRSETWORLDTRANSFORM pEmr = (PU_EMRSETWORLDTRANSFORM)(contents);
        states->currentDeviceContext.worldTransform = pEmr->xform;
        if(states->transform_open){
            fprintf(out, "</%sg>\n",  states->nameSpaceString);
        }
        transform_draw(states, out);
    } 

    void U_EMRMODIFYWORLDTRANSFORM_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRMODIFYWORLDTRANSFORM_print(contents, states);}
        PU_EMRMODIFYWORLDTRANSFORM pEmr = (PU_EMRMODIFYWORLDTRANSFORM)(contents);
        if(states->transform_open){
            fprintf(out, "</%sg>\n",  states->nameSpaceString);
        }
        switch (pEmr->iMode)
        {
            case U_MWT_IDENTITY:
                {
                    setTransformIdentity(states);
                    states->transform_open = false;
                    break;
                }
            case U_MWT_LEFTMULTIPLY:
                {

                    float a11 = pEmr->xform.eM11;
                    float a12 = pEmr->xform.eM12;
                    float a13 = 0.0;
                    float a21 = pEmr->xform.eM21;
                    float a22 = pEmr->xform.eM22;
                    float a23 = 0.0;
                    float a31 = pEmr->xform.eDx;
                    float a32 = pEmr->xform.eDy;
                    float a33 = 1.0;

                    float b11 = states->currentDeviceContext.worldTransform.eM11;
                    float b12 = states->currentDeviceContext.worldTransform.eM12;
                    //float b13 = 0.0;
                    float b21 = states->currentDeviceContext.worldTransform.eM21;
                    float b22 = states->currentDeviceContext.worldTransform.eM22;
                    //float b23 = 0.0;
                    float b31 = states->currentDeviceContext.worldTransform.eDx;
                    float b32 = states->currentDeviceContext.worldTransform.eDy;
                    //float b33 = 1.0;

                    float c11 = a11*b11 + a12*b21 + a13*b31;;
                    float c12 = a11*b12 + a12*b22 + a13*b32;;
                    //float c13 = a11*b13 + a12*b23 + a13*b33;;
                    float c21 = a21*b11 + a22*b21 + a23*b31;;
                    float c22 = a21*b12 + a22*b22 + a23*b32;;
                    //float c23 = a21*b13 + a22*b23 + a23*b33;;
                    float c31 = a31*b11 + a32*b21 + a33*b31;;
                    float c32 = a31*b12 + a32*b22 + a33*b32;;
                    //float c33 = a31*b13 + a32*b23 + a33*b33;;

                    states->currentDeviceContext.worldTransform.eM11 = c11;;
                    states->currentDeviceContext.worldTransform.eM12 = c12;;
                    states->currentDeviceContext.worldTransform.eM21 = c21;;
                    states->currentDeviceContext.worldTransform.eM22 = c22;;
                    states->currentDeviceContext.worldTransform.eDx = c31;
                    states->currentDeviceContext.worldTransform.eDy = c32;
                    transform_draw(states, out);
                    break;
                }
            case U_MWT_RIGHTMULTIPLY:
                {
                    float a11 = states->currentDeviceContext.worldTransform.eM11;
                    float a12 = states->currentDeviceContext.worldTransform.eM12;
                    float a13 = 0.0;
                    float a21 = states->currentDeviceContext.worldTransform.eM21;
                    float a22 = states->currentDeviceContext.worldTransform.eM22;
                    float a23 = 0.0;
                    float a31 = states->currentDeviceContext.worldTransform.eDx;
                    float a32 = states->currentDeviceContext.worldTransform.eDy;
                    float a33 = 1.0;

                    float b11 = pEmr->xform.eM11;
                    float b12 = pEmr->xform.eM12;
                    //float b13 = 0.0;
                    float b21 = pEmr->xform.eM21;
                    float b22 = pEmr->xform.eM22;
                    //float b23 = 0.0;
                    float b31 = pEmr->xform.eDx;
                    float b32 = pEmr->xform.eDy;
                    //float b33 = 1.0;

                    float c11 = a11*b11 + a12*b21 + a13*b31;;
                    float c12 = a11*b12 + a12*b22 + a13*b32;;
                    //float c13 = a11*b13 + a12*b23 + a13*b33;;
                    float c21 = a21*b11 + a22*b21 + a23*b31;;
                    float c22 = a21*b12 + a22*b22 + a23*b32;;
                    //float c23 = a21*b13 + a22*b23 + a23*b33;;
                    float c31 = a31*b11 + a32*b21 + a33*b31;;
                    float c32 = a31*b12 + a32*b22 + a33*b32;;
                    //float c33 = a31*b13 + a32*b23 + a33*b33;;

                    states->currentDeviceContext.worldTransform.eM11 = c11;;
                    states->currentDeviceContext.worldTransform.eM12 = c12;;
                    states->currentDeviceContext.worldTransform.eM21 = c21;;
                    states->currentDeviceContext.worldTransform.eM22 = c22;;
                    states->currentDeviceContext.worldTransform.eDx = c31;
                    states->currentDeviceContext.worldTransform.eDy = c32;
                    transform_draw(states, out);

                    break;
                }
            case U_MWT_SET:
                {
                    states->currentDeviceContext.worldTransform = pEmr->xform;
                    transform_draw(states, out);
                    break;
                }
            default:
                break;
        }
    } 

    void U_EMRSELECTOBJECT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSELECTOBJECT_print(contents, states);}
        PU_EMRSELECTOBJECT pEmr = (PU_EMRSELECTOBJECT)(contents);
        uint32_t index = pEmr->ihObject;
        if(index & U_STOCK_OBJECT){
            switch(index){
                case(U_WHITE_BRUSH):
                    states->currentDeviceContext.fill_red   = 0xFF;
                    states->currentDeviceContext.fill_blue  = 0xFF;
                    states->currentDeviceContext.fill_green = 0xFF;
                    states->currentDeviceContext.fill_mode  = U_BS_SOLID;
                    break;
                case(U_LTGRAY_BRUSH):
                    states->currentDeviceContext.fill_red   = 0xC0;
                    states->currentDeviceContext.fill_blue  = 0xC0;
                    states->currentDeviceContext.fill_green = 0xC0;
                    states->currentDeviceContext.fill_mode  = U_BS_SOLID;
                    break;
                case(U_GRAY_BRUSH):
                    states->currentDeviceContext.fill_red   = 0x80;
                    states->currentDeviceContext.fill_blue  = 0x80;
                    states->currentDeviceContext.fill_green = 0x80;
                    states->currentDeviceContext.fill_mode  = U_BS_SOLID;
                    break;
                case(U_DKGRAY_BRUSH):
                    states->currentDeviceContext.fill_red   = 0x40;
                    states->currentDeviceContext.fill_blue  = 0x40;
                    states->currentDeviceContext.fill_green = 0x40;
                    states->currentDeviceContext.fill_mode  = U_BS_SOLID;
                    break;
                case(U_BLACK_BRUSH):
                    states->currentDeviceContext.fill_red   = 0x00;
                    states->currentDeviceContext.fill_blue  = 0x00;
                    states->currentDeviceContext.fill_green = 0x00;
                    states->currentDeviceContext.fill_mode  = U_BS_SOLID;
                    break;
                case(U_NULL_BRUSH):
                    states->currentDeviceContext.fill_mode = U_BS_NULL;
                    break;
                case(U_WHITE_PEN):
                    states->currentDeviceContext.stroke_red   = 0xFF;
                    states->currentDeviceContext.stroke_blue  = 0xFF;
                    states->currentDeviceContext.stroke_green = 0xFF;
                    states->currentDeviceContext.stroke_mode  = U_PS_SOLID;
                    break;
                case(U_BLACK_PEN):
                    states->currentDeviceContext.stroke_red   = 0x00;
                    states->currentDeviceContext.stroke_blue  = 0x00;
                    states->currentDeviceContext.stroke_green = 0x00;
                    states->currentDeviceContext.stroke_mode  = U_PS_SOLID;
                    break;
                case(U_NULL_PEN):
                    states->currentDeviceContext.stroke_mode  = U_PS_NULL;
                    break;
                case(U_OEM_FIXED_FONT):
                    break;
                case(U_ANSI_FIXED_FONT):
                    break;
                case(U_ANSI_VAR_FONT):
                    break;
                case(U_SYSTEM_FONT):
                    break;
                case(U_DEVICE_DEFAULT_FONT):
                    break;
                case(U_DEFAULT_PALETTE):
                    break;
                case(U_SYSTEM_FIXED_FONT):
                    break;
                case(U_DEFAULT_GUI_FONT):
                    break;
                default:
                    break;
            }
        }
        else {
            if (index > states->objectTableSize){
                return;
            }
            if(states->objectTable[index].fill_set){
                states->currentDeviceContext.fill_red   = states->objectTable[index].fill_red;
                states->currentDeviceContext.fill_blue  = states->objectTable[index].fill_blue;
                states->currentDeviceContext.fill_green = states->objectTable[index].fill_green;
                states->currentDeviceContext.fill_mode  = states->objectTable[index].fill_mode;
            }
            else if(states->objectTable[index].stroke_set){
                states->currentDeviceContext.stroke_red     = states->objectTable[index].stroke_red;
                states->currentDeviceContext.stroke_blue    = states->objectTable[index].stroke_blue;
                states->currentDeviceContext.stroke_green   = states->objectTable[index].stroke_green;
                states->currentDeviceContext.stroke_mode    = states->objectTable[index].stroke_mode;
                states->currentDeviceContext.stroke_width   = states->objectTable[index].stroke_width;
            }
            else if(states->objectTable[index].font_set){
                states->currentDeviceContext.font_width       = states->objectTable[index].font_width;
                states->currentDeviceContext.font_height      = states->objectTable[index].font_height;
                states->currentDeviceContext.font_weight      = states->objectTable[index].font_weight;
                states->currentDeviceContext.font_italic      = states->objectTable[index].font_italic;
                states->currentDeviceContext.font_underline   = states->objectTable[index].font_underline;
                states->currentDeviceContext.font_strikeout   = states->objectTable[index].font_strikeout;
                states->currentDeviceContext.font_escapement  = states->objectTable[index].font_escapement;
                states->currentDeviceContext.font_orientation = states->objectTable[index].font_orientation;
                if (states->currentDeviceContext.font_name != NULL)
                    free(states->currentDeviceContext.font_name);
                if (states->objectTable[index].font_name != NULL){
                    size_t len = strlen(states->objectTable[index].font_name);
                    states->currentDeviceContext.font_name = calloc((len + 1), sizeof(char)); 
                    strcpy(states->currentDeviceContext.font_name, states->objectTable[index].font_name);
                }
                if (states->currentDeviceContext.font_family != NULL)
                    free(states->currentDeviceContext.font_family);
                if (states->objectTable[index].font_family != NULL){
                    size_t len = strlen(states->objectTable[index].font_family);
                    states->currentDeviceContext.font_family = calloc((len + 1), sizeof(char)); 
                    strcpy(states->currentDeviceContext.font_family, states->objectTable[index].font_family);
                }
            }
        }
    } 

    void U_EMRCREATEPEN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRCREATEPEN_print(contents, states);}

        PU_EMRCREATEPEN pEmr = (PU_EMRCREATEPEN)(contents);

        uint32_t index = pEmr->ihPen;
        if (index > states->objectTableSize){
            return;
        }
        states->objectTable[index].stroke_set     = true;
        states->objectTable[index].stroke_red     = pEmr->lopn.lopnColor.Red;
        states->objectTable[index].stroke_blue    = pEmr->lopn.lopnColor.Blue;
        states->objectTable[index].stroke_green   = pEmr->lopn.lopnColor.Green;
        states->objectTable[index].stroke_mode    = pEmr->lopn.lopnStyle;
        states->objectTable[index].stroke_width   = pEmr->lopn.lopnWidth.x;// * states->scaling;
    } 

    void U_EMRCREATEBRUSHINDIRECT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRCREATEBRUSHINDIRECT_print(contents, states);}
        PU_EMRCREATEBRUSHINDIRECT pEmr = (PU_EMRCREATEBRUSHINDIRECT)(contents);

        uint16_t index = pEmr->ihBrush;
        if (index > states->objectTableSize){
            return;
        }
        if(pEmr->lb.lbStyle == U_BS_SOLID){
            states->objectTable[index].fill_red     = pEmr->lb.lbColor.Red;
            states->objectTable[index].fill_green   = pEmr->lb.lbColor.Green;
            states->objectTable[index].fill_blue    = pEmr->lb.lbColor.Blue;
            states->objectTable[index].fill_mode    = U_BS_SOLID;
            states->objectTable[index].fill_set     = true;
        }
        else if(pEmr->lb.lbStyle == U_BS_HATCHED){
            states->objectTable[index].fill_recidx  = pEmr->ihBrush; // used if the hatch needs to be redone due to bkMode, textmode, etc. changes
            states->objectTable[index].fill_red     = pEmr->lb.lbColor.Red;
            states->objectTable[index].fill_green   = pEmr->lb.lbColor.Green;
            states->objectTable[index].fill_blue    = pEmr->lb.lbColor.Blue;
            states->objectTable[index].fill_mode    = U_BS_HATCHED;
            states->objectTable[index].fill_set     = true;
        }
    } 

    void U_EMRDELETEOBJECT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRDELETEOBJECT_print(contents, states);}
        PU_EMRDELETEOBJECT pEmr = (PU_EMRDELETEOBJECT)(contents);
        uint16_t index = pEmr->ihObject;
        if (index > states->objectTableSize){
            return;
        }
        freeObject(states, index);
        states->objectTable[index] = (const emfGraphObject){ 0 };
    } 

    void U_EMRANGLEARC_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRANGLEARC_print(contents, states);}
        PU_EMRANGLEARC pEmr = (PU_EMRANGLEARC)(contents);
    } 

    void U_EMRELLIPSE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRELLIPSE_print(contents, states);}
        PU_EMRELLIPSE pEmr      = (PU_EMRELLIPSE)(   contents);
        POINT_D LT = point_cal(states, (double)pEmr->rclBox.left, (double)pEmr->rclBox.top);
        POINT_D RB = point_cal(states, (double)pEmr->rclBox.right, (double)pEmr->rclBox.bottom);
        POINT_D center;
        POINT_D radius;
        center.x = (LT.x + RB.x) / 2;
        center.y = (LT.y + RB.y) / 2;
        radius.x = (RB.x - LT.x) / 2;
        radius.y = (RB.y - LT.y) / 2;
        bool localPath = false;
        fprintf(out, "<%sellipse cx=\"%.2f\" cy=\"%.2f\" rx=\"%.2f\" ry=\"%.2f\" ",
                states->nameSpaceString,
                center.x,
                center.y,
                radius.x,
                radius.y
               );
        bool filled = false;
        bool stroked = false;
        fill_draw(states, out, &filled, &stroked);
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");
        fprintf(out, "/>\n");
    }

    void U_EMRRECTANGLE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRRECTANGLE_print(contents, states);}
        PU_EMRRECTANGLE pEmr      = (PU_EMRRECTANGLE)(contents);
        POINT_D LT = point_cal(states, (double)pEmr->rclBox.left, (double)pEmr->rclBox.top);
        POINT_D RB = point_cal(states, (double)pEmr->rclBox.right, (double)pEmr->rclBox.bottom);
        POINT_D dim;
        dim.x = RB.x - LT.x;
        dim.y = RB.y - LT.y;
        bool localPath = false;
        fprintf(out, "<%srect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" ",
                states->nameSpaceString,
                LT.x,
                LT.y,
                dim.x,
                dim.y
               );
        bool filled = false;
        bool stroked = false;
        fill_draw(states, out, &filled, &stroked);
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");
        fprintf(out, "/>\n");

    }

    void U_EMRROUNDRECT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRROUNDRECT_print(contents, states);}
        PU_EMRROUNDRECT pEmr = (PU_EMRROUNDRECT)(contents);
        POINT_D LT = point_cal(states, (double)pEmr->rclBox.left, (double)pEmr->rclBox.top);
        POINT_D RB = point_cal(states, (double)pEmr->rclBox.right, (double)pEmr->rclBox.bottom);
        POINT_D dim;
        POINT_D round;
        dim.x = RB.x - LT.x;
        dim.y = RB.y - LT.y;
        bool localPath = false;
        fprintf(out, "<%srect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" ",
                states->nameSpaceString,
                LT.x,
                LT.y,
                dim.x,
                dim.y
               );
        round = point_cal(states, (double)pEmr->szlCorner.cx, (double)pEmr->szlCorner.cy);
        fprintf(out, "rx=\"%.2f\" ry=\"%.2f\" ",
                round.x,
                round.y
        );
        bool filled = false;
        bool stroked = false;
        fill_draw(states, out, &filled, &stroked);
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");
        fprintf(out, "/>\n");
    }

    void U_EMRARC_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRARC_print(contents, states);}
        arc_draw(contents, out, states);
    }

    void U_EMRCHORD_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRCHORD_print(contents, states);}
    }

    void U_EMRPIE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRPIE_print(contents, states);}
    }

    void U_EMRSELECTPALETTE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSELECTPALETTE_print(contents, states);}
    }

    void U_EMRCREATEPALETTE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRCREATEPALETTE_print(contents, states);}
        PU_EMRCREATEPALETTE pEmr = (PU_EMRCREATEPALETTE)(contents);
    }

    void U_EMRSETPALETTEENTRIES_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETPALETTEENTRIES_print(contents, states);}
        PU_EMRSETPALETTEENTRIES pEmr = (PU_EMRSETPALETTEENTRIES)(contents);
    }

    void U_EMRRESIZEPALETTE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRRESIZEPALETTE_print(contents, states);}
    } 

    void U_EMRREALIZEPALETTE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRREALIZEPALETTE_print(contents, states);}
        UNUSED(contents);
    }

    void U_EMREXTFLOODFILL_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMREXTFLOODFILL_print(contents, states);}
        PU_EMREXTFLOODFILL pEmr = (PU_EMREXTFLOODFILL)(contents);
    }

    void U_EMRLINETO_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRLINETO_print(contents, states);}
        lineto_draw("U_EMRLINETO", "ptl:","",contents, out, states);
    } 

    void U_EMRARCTO_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRARCTO_print(contents, states);}
        arc_draw(contents, out, states);
    }

    void U_EMRPOLYDRAW_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRPOLYDRAW_print(contents, states);}
        PU_EMRPOLYDRAW pEmr = (PU_EMRPOLYDRAW)(contents);
    }

    void U_EMRSETARCDIRECTION_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRSETARCDIRECTION_print(contents, states);}
        PU_EMRSETARCDIRECTION pEmr = (PU_EMRSETARCDIRECTION)contents;
        switch(pEmr->iArcDirection){
            case U_AD_CLOCKWISE:
                states->currentDeviceContext.arcdir = 1;
                break;
            case U_AD_COUNTERCLOCKWISE:
                states->currentDeviceContext.arcdir = -1;
                break;
        }
    }

    void U_EMRSETMITERLIMIT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETMITERLIMIT_print(contents, states);}

    }

    void U_EMRBEGINPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRBEGINPATH_print(contents, states);}
        pathStack * stack = states->emfStructure.pathStack;
        uint32_t clipOffset       = stack->pathStruct.clipOffset;
        if (clipOffset != 0){
            //int id = get_id(states);
            //fprintf(out, "<%sclipPath id=\"clip-%d\">\n", states->nameSpaceString, id);
            //states->clipId = id;
            states->inClip = true;
        }
        fprintf(out, "<%spath d=\"", states->nameSpaceString);
        states->inPath = 1;
        UNUSED(contents);
    }

    void U_EMRENDPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRENDPATH_print(contents, states);}
        fprintf(out, "\" ");
        states->inPath = 0;
        bool filled = false;
        bool stroked = false;
        pathStack * stack = states->emfStructure.pathStack;
        uint32_t fillOffset       = stack->pathStruct.fillOffset;
        uint32_t strokeOffset     = stack->pathStruct.strokeOffset;
        uint32_t strokeFillOffset = stack->pathStruct.strokeFillOffset;
        uint32_t clipOffset       = stack->pathStruct.clipOffset;
        if (fillOffset != 0)
            fill_draw(states, out, &filled, &stroked);
        if (strokeOffset != 0)
            stroke_draw(states, out, &filled, &stroked);
        if (strokeFillOffset !=0){
            fill_draw(states, out, &filled, &stroked);
            stroke_draw(states, out, &filled, &stroked);
        }
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");

        fprintf(out, "/>\n");
        if (clipOffset != 0){
            //fprintf(out, "</%sclipPath>\n", states->nameSpaceString);
            states->clipSet = true;
        }

        states->emfStructure.pathStack = stack->next;
        free(stack);
        UNUSED(contents);
    }

    void U_EMRCLOSEFIGURE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRCLOSEFIGURE_print(contents, states);}
        fprintf(out, "Z ");
        UNUSED(contents);
    }

    void U_EMRFILLPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRFILLPATH_print(contents, states);}
        // real work done in U_EMRENDPATH
    }

    void U_EMRSTROKEANDFILLPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSTROKEANDFILLPATH_print(contents, states);}
        // real work done in U_EMRENDPATH
    }

    void U_EMRSTROKEPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMRSTROKEPATH_print(contents, states);}
        // real work done in U_EMRENDPATH
    }

    void U_EMRFLATTENPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRFLATTENPATH_print(contents, states);}
        UNUSED(contents);
    }

    void U_EMRWIDENPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRWIDENPATH_print(contents, states);}
        UNUSED(contents);
    }

    void U_EMRSELECTCLIPPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSELECTCLIPPATH_print(contents, states);}
    }

    void U_EMRABORTPATH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRABORTPATH_print(contents, states);}
        UNUSED(contents);
    }

#define U_EMRUNDEF69_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRUNDEF69",A) //!< Not implemented.

    void U_EMRCOMMENT_draw(const char *contents, FILE *out, drawingStates *states, const char *blimit, size_t off){
        FLAG_IGNORED;
        if (states->verbose){U_EMRCOMMENT_print(contents, states, blimit, off);}
        char *string;
        char *src;
        uint32_t cIdent,cIdent2,cbData;
        size_t loff;
        int    recsize;
        static int recnum=0;

        PU_EMRCOMMENT pEmr = (PU_EMRCOMMENT)(contents);

        /* There are several different types of comments */

        cbData = pEmr->cbData;
        src = (char *)&(pEmr->Data);  // default
        if(cbData >= 4){
            /* Since the comment is just a big bag of bytes the emf endian code cannot safely touch
               any of its payload.  This is the only record type with that limitation.  Try to determine
               what the contents are even if more byte swapping is required. */
            cIdent = *(uint32_t *)(src);
            if(U_BYTE_SWAP){ U_swap4(&(cIdent),1); }
            if(     cIdent == U_EMR_COMMENT_PUBLIC       ){
                PU_EMRCOMMENT_PUBLIC pEmrp = (PU_EMRCOMMENT_PUBLIC) pEmr;
                cIdent2 = pEmrp->pcIdent;
                if(U_BYTE_SWAP){ U_swap4(&(cIdent2),1); }
                src = (char *)&(pEmrp->Data);
                cbData -= 8;
            }
            else if(cIdent == U_EMR_COMMENT_SPOOL        ){
                PU_EMRCOMMENT_SPOOL pEmrs = (PU_EMRCOMMENT_SPOOL) pEmr;
                cIdent2 = pEmrs->esrIdent;
                if(U_BYTE_SWAP){ U_swap4(&(cIdent2),1); }
                src = (char *)&(pEmrs->Data);
                cbData -= 8;
            }
            else if(cIdent == U_EMR_COMMENT_EMFPLUSRECORD){
                PU_EMRCOMMENT_EMFPLUS pEmrpl = (PU_EMRCOMMENT_EMFPLUS) pEmr;
                src = (char *)&(pEmrpl->Data);
                if (states->emfplus){
                    loff = 16;  /* Header size of the header part of an EMF+ comment record */
                    if (states->verbose){printf("\n   =====================%s START EMF+ RECORD ANALYSING %s=====================\n\n", KCYN, KNRM);}
                    while(loff < cbData + 12){  // EMF+ records may not fill the entire comment, cbData value includes cIdent, but not U_EMR or cbData
                        recsize =  U_pmf_onerec_draw(src, blimit, recnum, loff + off, out, states);
                        if(recsize<=0)break;
                        loff += recsize;
                        src  += recsize;
                        recnum++;
                    }
                    if (states->verbose){printf("\n   ======================%s END EMF+ RECORD ANALYSING %s======================\n", KBLU , KNRM);}
                }
                return;
            }
        }
        if(cbData){ // The data may not be printable, but try it just in case
            string = (char *)malloc(cbData + 1);
            (void)strncpy(string, src, cbData);
            string[cbData] = '\0'; // it might not be terminated - it might not even be text!
            free(string);
        }
    } 

    void U_EMRFILLRGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRFILLRGN_print(contents, states);}
        int i,roff;
        PU_EMRFILLRGN pEmr = (PU_EMRFILLRGN)(contents);
    } 

    void U_EMRFRAMERGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRFRAMERGN_print(contents, states);}
        PU_EMRFRAMERGN pEmr = (PU_EMRFRAMERGN)(contents);
    } 

    void U_EMRINVERTRGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRINVERTRGN_print(contents, states);}
    }

    void U_EMRPAINTRGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRPAINTRGN_print(contents, states);}
    }

    void U_EMREXTSELECTCLIPRGN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMREXTSELECTCLIPRGN_print(contents, states);}
        int i,roff;
        PU_EMREXTSELECTCLIPRGN pEmr = (PU_EMREXTSELECTCLIPRGN) (contents);
    } 

    void U_EMRBITBLT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRBITBLT_print(contents, states);}
        PU_EMRBITBLT pEmr = (PU_EMRBITBLT) (contents);
    }

    void U_EMRSTRETCHBLT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSTRETCHBLT_print(contents, states);}
        PU_EMRSTRETCHBLT pEmr = (PU_EMRSTRETCHBLT) (contents);
    }

    void U_EMRMASKBLT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRMASKBLT_print(contents, states);}
        PU_EMRMASKBLT pEmr = (PU_EMRMASKBLT) (contents);
    }

    void U_EMRPLGBLT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRPLGBLT_print(contents, states);}
        PU_EMRPLGBLT pEmr = (PU_EMRPLGBLT) (contents);
    }

    void U_EMRSETDIBITSTODEVICE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETDIBITSTODEVICE_print(contents, states);}
        PU_EMRSETDIBITSTODEVICE pEmr = (PU_EMRSETDIBITSTODEVICE) (contents);
    }

    void U_EMRSTRETCHDIBITS_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSTRETCHDIBITS_print(contents, states);}
        PU_EMRSTRETCHDIBITS pEmr = (PU_EMRSTRETCHDIBITS) (contents);
    }

    void U_EMREXTCREATEFONTINDIRECTW_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMREXTCREATEFONTINDIRECTW_print(contents, states);}
        PU_EMREXTCREATEFONTINDIRECTW pEmr = (PU_EMREXTCREATEFONTINDIRECTW) (contents);
        uint16_t index = pEmr->ihFont;
        if (index > states->objectTableSize){
            return;
        }
        if (states->objectTable[index].font_name != NULL)
            free(states->objectTable[index].font_name);

        if (states->objectTable[index].font_family != NULL)
            free(states->objectTable[index].font_family);

         U_LOGFONT logfont;

        if(pEmr->emr.nSize == sizeof(U_EMREXTCREATEFONTINDIRECTW)){ // holds logfont_panose
            U_LOGFONT_PANOSE lfp = pEmr->elfw;
            logfont = pEmr->elfw.elfLogFont; 
            char *fullname = U_Utf16leToUtf8(lfp.elfFullName, U_LF_FULLFACESIZE, NULL);
            states->objectTable[index].font_name = fullname;
        }
        else { // holds logfont
            logfont = *(PU_LOGFONT) &(pEmr->elfw);
        }
        char *family = U_Utf16leToUtf8(logfont.lfFaceName, U_LF_FACESIZE, NULL);
        states->objectTable[index].font_width       = abs(logfont.lfWidth);
        states->objectTable[index].font_height      = abs(logfont.lfHeight);
        states->objectTable[index].font_weight      = logfont.lfWeight;
        states->objectTable[index].font_italic      = logfont.lfItalic;
        states->objectTable[index].font_underline   = logfont.lfUnderline;
        states->objectTable[index].font_strikeout   = logfont.lfStrikeOut;
        states->objectTable[index].font_escapement  = (logfont.lfEscapement % 3600);
        states->objectTable[index].font_orientation = (logfont.lfOrientation % 3600);
        states->objectTable[index].font_family      = family;
        states->objectTable[index].font_set         = true;
    }

    void U_EMREXTTEXTOUTA_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMREXTTEXTOUTA_print(contents, states);}
        text_draw(contents, out, states, ASCII);
    }

    void U_EMREXTTEXTOUTW_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMREXTTEXTOUTW_print(contents, states);}
        text_draw(contents, out, states, UTF_16);
    }

    void U_EMRPOLYBEZIER16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYBEZIER16_print(contents, states);}
        cubic_bezier16_draw("U_EMRPOLYBEZIER16", contents, out, states, 1);
    }

    void U_EMRPOLYGON16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYGON16_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath ", states->nameSpaceString);
            if (states->clipSet)
                fprintf(out, " clip-path=\"url(#clip-%d)\" ", states->clipId);
            fprintf(out, "d=\"");
        }
        bool ispolygon = true;
        polyline16_draw("U_EMRPOLYGON16", contents, out, states, ispolygon);

        if (localPath){
            states->inPath = false;
            fprintf(out, "Z\" ");
            bool filled = false;
            bool stroked = false;
            stroke_draw(states, out, &filled, &stroked);
            fill_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");

            fprintf(out, "/><!-- shit -->\n");
        }
    }

    void U_EMRPOLYLINE16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYLINE16_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath ", states->nameSpaceString);
            if (states->clipSet)
                fprintf(out, " clip-path=\"url(#clip-%d)\" ", states->clipId);
            fprintf(out, "d=\"");
        }
        bool ispolygon = true;
        polyline16_draw("U_EMRPOLYGON16", contents, out, states, ispolygon);

        if (localPath){
            states->inPath = false;
            //fprintf(out, "Z\" ");
            fprintf(out, "\" ");
            bool filled = false;
            bool stroked = false;
            stroke_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");
            fprintf(out, "/>\n");
        }
    }

    void U_EMRPOLYBEZIERTO16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYBEZIERTO16_print(contents, states);}
        cubic_bezier16_draw("U_EMRPOLYBEZIERTO16", contents, out, states, false);
    }

    void U_EMRPOLYLINETO16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYLINETO16_print(contents, states);}
        polyline16_draw("U_EMRPOLYLINETO16", contents, out, states, false);
    }

    void U_EMRPOLYPOLYLINE16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYPOLYLINE16_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath ", states->nameSpaceString);
            if (states->clipSet)
                fprintf(out, " clip-path=\"url(#clip-%d)\" ", states->clipId);
            fprintf(out, "d=\"");
        }
        bool ispolygon = false;
        polypolygon16_draw("U_EMRPOLYPOLYGON16", contents, out, states, false);

        if (localPath){
            states->inPath = false;
            fprintf(out, "\" ");
            bool filled = false;
            bool stroked = false;
            stroke_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");

            fprintf(out, "/>\n");
        }

    }

    void U_EMRPOLYPOLYGON16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED;
        if (states->verbose){U_EMRPOLYPOLYGON16_print(contents, states);}
        bool localPath = false;
        if (!states->inPath){
            localPath = true;
            states->inPath = true;
            fprintf(out, "<%spath d=\"", states->nameSpaceString);
        }
        bool ispolygon = true;
        polypolygon16_draw("U_EMRPOLYPOLYGON16", contents, out, states, ispolygon);

        if (localPath){
            states->inPath = false;
            fprintf(out, "\" ");
            bool filled = false;
            bool stroked = false;
            fill_draw(states, out, &filled, &stroked);
            stroke_draw(states, out, &filled, &stroked);
            if (!filled)
                fprintf(out, "fill=\"none\" ");
            if (!stroked)
                fprintf(out, "stroke=\"none\" ");

            fprintf(out, "/>\n");
        }
    }

    void U_EMRPOLYDRAW16_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRPOLYDRAW16_print(contents, states);}
        unsigned int i;
        PU_EMRPOLYDRAW16 pEmr = (PU_EMRPOLYDRAW16)(contents);
    }

    void U_EMRCREATEMONOBRUSH_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRCREATEMONOBRUSH_print(contents, states);}
    }

    void U_EMRCREATEDIBPATTERNBRUSHPT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRCREATEDIBPATTERNBRUSHPT_print(contents, states);}
    }

    void U_EMREXTCREATEPEN_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL;
        if (states->verbose){U_EMREXTCREATEPEN_print(contents, states);}
        PU_EMREXTCREATEPEN pEmr = (PU_EMREXTCREATEPEN)(contents);
        uint32_t index = pEmr->ihPen;
        if (index > states->objectTableSize){
            return;
        }
        PU_EXTLOGPEN pen = (PU_EXTLOGPEN) &(pEmr->elp);
        states->objectTable[index].stroke_set     = true;
        states->objectTable[index].stroke_red     = pen->elpColor.Red;
        states->objectTable[index].stroke_blue    = pen->elpColor.Blue;
        states->objectTable[index].stroke_green   = pen->elpColor.Green;
        states->objectTable[index].stroke_mode    = pen->elpPenStyle;
        states->objectTable[index].stroke_width   = pen->elpWidth;// * states->scaling;

    } 

    // U_EMRPOLYTEXTOUTA         96 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTA_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRPOLYTEXTOUTA",A) //!< Not implemented.
    // U_EMRPOLYTEXTOUTW         97 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTW_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRPOLYTEXTOUTW",A) //!< Not implemented.

    void U_EMRSETICMMODE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_UNUSED;
        if (states->verbose){U_EMRSETICMMODE_print(contents, states);}
    }

    void U_EMRCREATECOLORSPACE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRCREATECOLORSPACE_print(contents, states);}
        PU_EMRCREATECOLORSPACE pEmr = (PU_EMRCREATECOLORSPACE)(contents);
    }

    void U_EMRSETCOLORSPACE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETCOLORSPACE_print(contents, states);}
    }

    void U_EMRDELETECOLORSPACE_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRDELETECOLORSPACE_print(contents, states);}
    }

    // U_EMRGLSRECORD           102  Not implemented
#define U_EMRGLSRECORD_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRGLSRECORD",A) //!< Not implemented.
    // U_EMRGLSBOUNDEDRECORD    103  Not implemented
#define U_EMRGLSBOUNDEDRECORD_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRGLSBOUNDEDRECORD",A) //!< Not implemented.

    void U_EMRPIXELFORMAT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRPIXELFORMAT_print(contents, states);}
        PU_EMRPIXELFORMAT pEmr = (PU_EMRPIXELFORMAT)(contents);
    }

    // U_EMRDRAWESCAPE          105  Not implemented
#define U_EMRDRAWESCAPE_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRDRAWESCAPE",A) //!< Not implemented.
    // U_EMREXTESCAPE           106  Not implemented
#define U_EMREXTESCAPE_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMREXTESCAPE",A) //!< Not implemented.
    // U_EMRUNDEF107            107  Not implemented
#define U_EMRUNDEF107_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRUNDEF107",A) //!< Not implemented.

    void U_EMRSMALLTEXTOUT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSMALLTEXTOUT_print(contents, states);}
        PU_EMRSMALLTEXTOUT pEmr = (PU_EMRSMALLTEXTOUT)(contents);
        //text_draw(contents, out, states, UTF_16);
    }

    // U_EMRFORCEUFIMAPPING     109  Not implemented
#define U_EMRFORCEUFIMAPPING_draw(A)     U_EMRNOTIMPLEMENTED_draw("U_EMRFORCEUFIMAPPING",A) //!< Not implemented.
    // U_EMRNAMEDESCAPE         110  Not implemented
#define U_EMRNAMEDESCAPE_draw(A)         U_EMRNOTIMPLEMENTED_draw("U_EMRNAMEDESCAPE",A) //!< Not implemented.
    // U_EMRCOLORCORRECTPALETTE 111  Not implemented
#define U_EMRCOLORCORRECTPALETTE_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMRCOLORCORRECTPALETTE",A) //!< Not implemented.
    // U_EMRSETICMPROFILEA      112  Not implemented
#define U_EMRSETICMPROFILEA_draw(A)      U_EMRNOTIMPLEMENTED_draw("U_EMRSETICMPROFILEA",A) //!< Not implemented.
    // U_EMRSETICMPROFILEW      113  Not implemented
#define U_EMRSETICMPROFILEW_draw(A)      U_EMRNOTIMPLEMENTED_draw("U_EMRSETICMPROFILEW",A) //!< Not implemented.

    void U_EMRALPHABLEND_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRALPHABLEND_print(contents, states);}
    }

    void U_EMRSETLAYOUT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRSETLAYOUT_print(contents, states);}
    }

    void U_EMRTRANSPARENTBLT_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRTRANSPARENTBLT_print(contents, states);}
    }

    // U_EMRUNDEF117            117  Not implemented
#define U_EMRUNDEF117_draw(A)    U_EMRNOTIMPLEMENTED_draw("U_EMRUNDEF117",A) //!< Not implemented.

    void U_EMRGRADIENTFILL_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRGRADIENTFILL_print(contents, states);}
        unsigned int i;
        PU_EMRGRADIENTFILL pEmr = (PU_EMRGRADIENTFILL)(contents);
    }

    // U_EMRSETLINKEDUFIS       119  Not implemented
#define U_EMRSETLINKEDUFIS_draw(A)        U_EMRNOTIMPLEMENTED_draw("U_EMR_SETLINKEDUFIS",A) //!< Not implemented.
    // U_EMRSETTEXTJUSTIFICATION120  Not implemented (denigrated)
#define U_EMRSETTEXTJUSTIFICATION_draw(A) U_EMRNOTIMPLEMENTED_draw("U_EMR_SETTEXTJUSTIFICATION",A) //!< Not implemented.
    // U_EMRCOLORMATCHTOTARGETW 121  Not implemented  
#define U_EMRCOLORMATCHTOTARGETW_draw(A)  U_EMRNOTIMPLEMENTED_draw("U_EMR_COLORMATCHTOTARGETW",A) //!< Not implemented.

    void U_EMRCREATECOLORSPACEW_draw(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED;
        if (states->verbose){U_EMRCREATECOLORSPACEW_print(contents, states);}
        unsigned int i;
        PU_EMRCREATECOLORSPACEW pEmr = (PU_EMRCREATECOLORSPACEW)(contents);
    }

    int U_emf_onerec_draw(const char *contents, const char *blimit, int recnum, size_t off, FILE *out, drawingStates *states){
        PU_ENHMETARECORD  lpEMFR  = (PU_ENHMETARECORD)(contents + off);
        unsigned int size;
        if (states->verbose){U_emf_onerec_print(contents, blimit, recnum, off, states);}
        size      = lpEMFR->nSize;
        contents += off;

        /* Check that the record size is OK, abort if not.
           Pointer math might wrap, so check both sides of the range */
        if(size < sizeof(U_EMR)           ||
                contents + size - 1 >= blimit  || 
                contents + size - 1 < contents)return(-1);

        switch (lpEMFR->iType)
        {
            case U_EMR_HEADER:                  U_EMRHEADER_draw(contents, out, states);                  break;
            case U_EMR_POLYBEZIER:              U_EMRPOLYBEZIER_draw(contents, out, states);              break;
            case U_EMR_POLYGON:                 U_EMRPOLYGON_draw(contents, out, states);                 break;
            case U_EMR_POLYLINE:                U_EMRPOLYLINE_draw(contents, out, states);                break;
            case U_EMR_POLYBEZIERTO:            U_EMRPOLYBEZIERTO_draw(contents, out, states);            break;
            case U_EMR_POLYLINETO:              U_EMRPOLYLINETO_draw(contents, out, states);              break;
            case U_EMR_POLYPOLYLINE:            U_EMRPOLYPOLYLINE_draw(contents, out, states);            break;
            case U_EMR_POLYPOLYGON:             U_EMRPOLYPOLYGON_draw(contents, out, states);             break;
            case U_EMR_SETWINDOWEXTEX:          U_EMRSETWINDOWEXTEX_draw(contents, out, states);          break;
            case U_EMR_SETWINDOWORGEX:          U_EMRSETWINDOWORGEX_draw(contents, out, states);          break;
            case U_EMR_SETVIEWPORTEXTEX:        U_EMRSETVIEWPORTEXTEX_draw(contents, out, states);        break;
            case U_EMR_SETVIEWPORTORGEX:        U_EMRSETVIEWPORTORGEX_draw(contents, out, states);        break;
            case U_EMR_SETBRUSHORGEX:           U_EMRSETBRUSHORGEX_draw(contents, out, states);           break;
            case U_EMR_EOF:                     U_EMREOF_draw(contents, out, states);          size=0;    break;
            case U_EMR_SETPIXELV:               U_EMRSETPIXELV_draw(contents, out, states);               break;
            case U_EMR_SETMAPPERFLAGS:          U_EMRSETMAPPERFLAGS_draw(contents, out, states);          break;
            case U_EMR_SETMAPMODE:              U_EMRSETMAPMODE_draw(contents, out, states);              break;
            case U_EMR_SETBKMODE:               U_EMRSETBKMODE_draw(contents, out, states);               break;
            case U_EMR_SETPOLYFILLMODE:         U_EMRSETPOLYFILLMODE_draw(contents, out, states);         break;
            case U_EMR_SETROP2:                 U_EMRSETROP2_draw(contents, out, states);                 break;
            case U_EMR_SETSTRETCHBLTMODE:       U_EMRSETSTRETCHBLTMODE_draw(contents, out, states);       break;
            case U_EMR_SETTEXTALIGN:            U_EMRSETTEXTALIGN_draw(contents, out, states);            break;
            case U_EMR_SETCOLORADJUSTMENT:      U_EMRSETCOLORADJUSTMENT_draw(contents, out, states);      break;
            case U_EMR_SETTEXTCOLOR:            U_EMRSETTEXTCOLOR_draw(contents, out, states);            break;
            case U_EMR_SETBKCOLOR:              U_EMRSETBKCOLOR_draw(contents, out, states);              break;
            case U_EMR_OFFSETCLIPRGN:           U_EMROFFSETCLIPRGN_draw(contents, out, states);           break;
            case U_EMR_MOVETOEX:                U_EMRMOVETOEX_draw(contents, out, states);                break;
            case U_EMR_SETMETARGN:              U_EMRSETMETARGN_draw(contents, out, states);              break;
            case U_EMR_EXCLUDECLIPRECT:         U_EMREXCLUDECLIPRECT_draw(contents, out, states);         break;
            case U_EMR_INTERSECTCLIPRECT:       U_EMRINTERSECTCLIPRECT_draw(contents, out, states);       break;
            case U_EMR_SCALEVIEWPORTEXTEX:      U_EMRSCALEVIEWPORTEXTEX_draw(contents, out, states);      break;
            case U_EMR_SCALEWINDOWEXTEX:        U_EMRSCALEWINDOWEXTEX_draw(contents, out, states);        break;
            case U_EMR_SAVEDC:                  U_EMRSAVEDC_draw(contents, out, states);                  break;
            case U_EMR_RESTOREDC:               U_EMRRESTOREDC_draw(contents, out, states);               break;
            case U_EMR_SETWORLDTRANSFORM:       U_EMRSETWORLDTRANSFORM_draw(contents, out, states);       break;
            case U_EMR_MODIFYWORLDTRANSFORM:    U_EMRMODIFYWORLDTRANSFORM_draw(contents, out, states);    break;
            case U_EMR_SELECTOBJECT:            U_EMRSELECTOBJECT_draw(contents, out, states);            break;
            case U_EMR_CREATEPEN:               U_EMRCREATEPEN_draw(contents, out, states);               break;
            case U_EMR_CREATEBRUSHINDIRECT:     U_EMRCREATEBRUSHINDIRECT_draw(contents, out, states);     break;
            case U_EMR_DELETEOBJECT:            U_EMRDELETEOBJECT_draw(contents, out, states);            break;
            case U_EMR_ANGLEARC:                U_EMRANGLEARC_draw(contents, out, states);                break;
            case U_EMR_ELLIPSE:                 U_EMRELLIPSE_draw(contents, out, states);                 break;
            case U_EMR_RECTANGLE:               U_EMRRECTANGLE_draw(contents, out, states);               break;
            case U_EMR_ROUNDRECT:               U_EMRROUNDRECT_draw(contents, out, states);               break;
            case U_EMR_ARC:                     U_EMRARC_draw(contents, out, states);                     break;
            case U_EMR_CHORD:                   U_EMRCHORD_draw(contents, out, states);                   break;
            case U_EMR_PIE:                     U_EMRPIE_draw(contents, out, states);                     break;
            case U_EMR_SELECTPALETTE:           U_EMRSELECTPALETTE_draw(contents, out, states);           break;
            case U_EMR_CREATEPALETTE:           U_EMRCREATEPALETTE_draw(contents, out, states);           break;
            case U_EMR_SETPALETTEENTRIES:       U_EMRSETPALETTEENTRIES_draw(contents, out, states);       break;
            case U_EMR_RESIZEPALETTE:           U_EMRRESIZEPALETTE_draw(contents, out, states);           break;
            case U_EMR_REALIZEPALETTE:          U_EMRREALIZEPALETTE_draw(contents, out, states);          break;
            case U_EMR_EXTFLOODFILL:            U_EMREXTFLOODFILL_draw(contents, out, states);            break;
            case U_EMR_LINETO:                  U_EMRLINETO_draw(contents, out, states);                  break;
            case U_EMR_ARCTO:                   U_EMRARCTO_draw(contents, out, states);                   break;
            case U_EMR_POLYDRAW:                U_EMRPOLYDRAW_draw(contents, out, states);                break;
            case U_EMR_SETARCDIRECTION:         U_EMRSETARCDIRECTION_draw(contents, out, states);         break;
            case U_EMR_SETMITERLIMIT:           U_EMRSETMITERLIMIT_draw(contents, out, states);           break;
            case U_EMR_BEGINPATH:               U_EMRBEGINPATH_draw(contents, out, states);               break;
            case U_EMR_ENDPATH:                 U_EMRENDPATH_draw(contents, out, states);                 break;
            case U_EMR_CLOSEFIGURE:             U_EMRCLOSEFIGURE_draw(contents, out, states);             break;
            case U_EMR_FILLPATH:                U_EMRFILLPATH_draw(contents, out, states);                break;
            case U_EMR_STROKEANDFILLPATH:       U_EMRSTROKEANDFILLPATH_draw(contents, out, states);       break;
            case U_EMR_STROKEPATH:              U_EMRSTROKEPATH_draw(contents, out, states);              break;
            case U_EMR_FLATTENPATH:             U_EMRFLATTENPATH_draw(contents, out, states);             break;
            case U_EMR_WIDENPATH:               U_EMRWIDENPATH_draw(contents, out, states);               break;
            case U_EMR_SELECTCLIPPATH:          U_EMRSELECTCLIPPATH_draw(contents, out, states);          break;
            case U_EMR_ABORTPATH:               U_EMRABORTPATH_draw(contents, out, states);               break;
                                                //        case U_EMR_UNDEF69:                 U_EMRUNDEF69_draw(contents, out, states);                 break;
            case U_EMR_COMMENT:                 U_EMRCOMMENT_draw(contents, out, states, blimit, off);    break;
            case U_EMR_FILLRGN:                 U_EMRFILLRGN_draw(contents, out, states);                 break;
            case U_EMR_FRAMERGN:                U_EMRFRAMERGN_draw(contents, out, states);                break;
            case U_EMR_INVERTRGN:               U_EMRINVERTRGN_draw(contents, out, states);               break;
            case U_EMR_PAINTRGN:                U_EMRPAINTRGN_draw(contents, out, states);                break;
            case U_EMR_EXTSELECTCLIPRGN:        U_EMREXTSELECTCLIPRGN_draw(contents, out, states);        break;
            case U_EMR_BITBLT:                  U_EMRBITBLT_draw(contents, out, states);                  break;
            case U_EMR_STRETCHBLT:              U_EMRSTRETCHBLT_draw(contents, out, states);              break;
            case U_EMR_MASKBLT:                 U_EMRMASKBLT_draw(contents, out, states);                 break;
            case U_EMR_PLGBLT:                  U_EMRPLGBLT_draw(contents, out, states);                  break;
            case U_EMR_SETDIBITSTODEVICE:       U_EMRSETDIBITSTODEVICE_draw(contents, out, states);       break;
            case U_EMR_STRETCHDIBITS:           U_EMRSTRETCHDIBITS_draw(contents, out, states);           break;
            case U_EMR_EXTCREATEFONTINDIRECTW:  U_EMREXTCREATEFONTINDIRECTW_draw(contents, out, states);  break;
            case U_EMR_EXTTEXTOUTA:             U_EMREXTTEXTOUTA_draw(contents, out, states);             break;
            case U_EMR_EXTTEXTOUTW:             U_EMREXTTEXTOUTW_draw(contents, out, states);             break;
            case U_EMR_POLYBEZIER16:            U_EMRPOLYBEZIER16_draw(contents, out, states);            break;
            case U_EMR_POLYGON16:               U_EMRPOLYGON16_draw(contents, out, states);               break;
            case U_EMR_POLYLINE16:              U_EMRPOLYLINE16_draw(contents, out, states);              break;
            case U_EMR_POLYBEZIERTO16:          U_EMRPOLYBEZIERTO16_draw(contents, out, states);          break;
            case U_EMR_POLYLINETO16:            U_EMRPOLYLINETO16_draw(contents, out, states);            break;
            case U_EMR_POLYPOLYLINE16:          U_EMRPOLYPOLYLINE16_draw(contents, out, states);          break;
            case U_EMR_POLYPOLYGON16:           U_EMRPOLYPOLYGON16_draw(contents, out, states);           break;
            case U_EMR_POLYDRAW16:              U_EMRPOLYDRAW16_draw(contents, out, states);              break;
            case U_EMR_CREATEMONOBRUSH:         U_EMRCREATEMONOBRUSH_draw(contents, out, states);         break;
            case U_EMR_CREATEDIBPATTERNBRUSHPT: U_EMRCREATEDIBPATTERNBRUSHPT_draw(contents, out, states); break;
            case U_EMR_EXTCREATEPEN:            U_EMREXTCREATEPEN_draw(contents, out, states);            break;
          //case U_EMR_POLYTEXTOUTA:            U_EMRPOLYTEXTOUTA_draw(contents, out, states);            break;
          //case U_EMR_POLYTEXTOUTW:            U_EMRPOLYTEXTOUTW_draw(contents, out, states);            break;
            case U_EMR_SETICMMODE:              U_EMRSETICMMODE_draw(contents, out, states);              break;
            case U_EMR_CREATECOLORSPACE:        U_EMRCREATECOLORSPACE_draw(contents, out, states);        break;
            case U_EMR_SETCOLORSPACE:           U_EMRSETCOLORSPACE_draw(contents, out, states);           break;
            case U_EMR_DELETECOLORSPACE:        U_EMRDELETECOLORSPACE_draw(contents, out, states);        break;
          //case U_EMR_GLSRECORD:               U_EMRGLSRECORD_draw(contents, out, states);               break;
          //case U_EMR_GLSBOUNDEDRECORD:        U_EMRGLSBOUNDEDRECORD_draw(contents, out, states);        break;
            case U_EMR_PIXELFORMAT:             U_EMRPIXELFORMAT_draw(contents, out, states);             break;
          //case U_EMR_DRAWESCAPE:              U_EMRDRAWESCAPE_draw(contents, out, states);              break;
          //case U_EMR_EXTESCAPE:               U_EMREXTESCAPE_draw(contents, out, states);               break;
          //case U_EMR_UNDEF107:                U_EMRUNDEF107_draw(contents, out, states);                break;
            case U_EMR_SMALLTEXTOUT:            U_EMRSMALLTEXTOUT_draw(contents, out, states);            break;
          //case U_EMR_FORCEUFIMAPPING:         U_EMRFORCEUFIMAPPING_draw(contents, out, states);         break;
          //case U_EMR_NAMEDESCAPE:             U_EMRNAMEDESCAPE_draw(contents, out, states);             break;
          //case U_EMR_COLORCORRECTPALETTE:     U_EMRCOLORCORRECTPALETTE_draw(contents, out, states);     break;
          //case U_EMR_SETICMPROFILEA:          U_EMRSETICMPROFILEA_draw(contents, out, states);          break;
          //case U_EMR_SETICMPROFILEW:          U_EMRSETICMPROFILEW_draw(contents, out, states);          break;
            case U_EMR_ALPHABLEND:              U_EMRALPHABLEND_draw(contents, out, states);              break;
            case U_EMR_SETLAYOUT:               U_EMRSETLAYOUT_draw(contents, out, states);               break;
            case U_EMR_TRANSPARENTBLT:          U_EMRTRANSPARENTBLT_draw(contents, out, states);          break;
          //case U_EMR_UNDEF117:                U_EMRUNDEF117_draw(contents, out, states);                break;
            case U_EMR_GRADIENTFILL:            U_EMRGRADIENTFILL_draw(contents, out, states);            break;
          //case U_EMR_SETLINKEDUFIS:           U_EMRSETLINKEDUFIS_draw(contents, out, states);           break;
          //case U_EMR_SETTEXTJUSTIFICATION:    U_EMRSETTEXTJUSTIFICATION_draw(contents, out, states);    break;
          //case U_EMR_COLORMATCHTOTARGETW:     U_EMRCOLORMATCHTOTARGETW_draw(contents, out, states);     break;
            case U_EMR_CREATECOLORSPACEW:       U_EMRCREATECOLORSPACEW_draw(contents, out, states);       break;
            default:                            U_EMRNOTIMPLEMENTED_draw("?",contents, out, states);      break;
        }  //end of switch
        return(size);
    }

    int U_emf_onerec_analyse(const char *contents, const char *blimit, int recnum, size_t off, drawingStates *states){
        PU_ENHMETARECORD  lpEMFR  = (PU_ENHMETARECORD)(contents + off);
        unsigned int size;

        size      = lpEMFR->nSize;
        contents += off;

        /* Check that the record size is OK, abort if not.
           Pointer math might wrap, so check both sides of the range */
        if(size < sizeof(U_EMR)           ||
                contents + size - 1 >= blimit  || 
                contents + size - 1 < contents)return(-1);

        switch (lpEMFR->iType)
        {
            case U_EMR_HEADER:                  break;
            case U_EMR_POLYBEZIER:              break;
            case U_EMR_POLYGON:                 break;
            case U_EMR_POLYLINE:                break;
            case U_EMR_POLYBEZIERTO:            break;
            case U_EMR_POLYLINETO:              break;
            case U_EMR_POLYPOLYLINE:            break;
            case U_EMR_POLYPOLYGON:             break;
            case U_EMR_SETWINDOWEXTEX:          break;
            case U_EMR_SETWINDOWORGEX:          break;
            case U_EMR_SETVIEWPORTEXTEX:        break;
            case U_EMR_SETVIEWPORTORGEX:        break;
            case U_EMR_SETBRUSHORGEX:           break;
            case U_EMR_EOF:                     size=0; break;
            case U_EMR_SETPIXELV:               break;
            case U_EMR_SETMAPPERFLAGS:          break;
            case U_EMR_SETMAPMODE:              break;
            case U_EMR_SETBKMODE:               break;
            case U_EMR_SETPOLYFILLMODE:         break;
            case U_EMR_SETROP2:                 break;
            case U_EMR_SETSTRETCHBLTMODE:       break;
            case U_EMR_SETTEXTALIGN:            break;
            case U_EMR_SETCOLORADJUSTMENT:      break;
            case U_EMR_SETTEXTCOLOR:            break;
            case U_EMR_SETBKCOLOR:              break;
            case U_EMR_OFFSETCLIPRGN:           break;
            case U_EMR_MOVETOEX:                break;
            case U_EMR_SETMETARGN:              break;
            case U_EMR_EXCLUDECLIPRECT:         break;
            case U_EMR_INTERSECTCLIPRECT:       break;
            case U_EMR_SCALEVIEWPORTEXTEX:      break;
            case U_EMR_SCALEWINDOWEXTEX:        break;
            case U_EMR_SAVEDC:                  break;
            case U_EMR_RESTOREDC:               break;
            case U_EMR_SETWORLDTRANSFORM:       break;
            case U_EMR_MODIFYWORLDTRANSFORM:    break;
            case U_EMR_SELECTOBJECT:            break;
            case U_EMR_CREATEPEN:               break;
            case U_EMR_CREATEBRUSHINDIRECT:     break;
            case U_EMR_DELETEOBJECT:            break;
            case U_EMR_ANGLEARC:                break;
            case U_EMR_ELLIPSE:                 break;
            case U_EMR_RECTANGLE:               break;
            case U_EMR_ROUNDRECT:               break;
            case U_EMR_ARC:                     break;
            case U_EMR_CHORD:                   break;
            case U_EMR_PIE:                     break;
            case U_EMR_SELECTPALETTE:           break;
            case U_EMR_CREATEPALETTE:           break;
            case U_EMR_SETPALETTEENTRIES:       break;
            case U_EMR_RESIZEPALETTE:           break;
            case U_EMR_REALIZEPALETTE:          break;
            case U_EMR_EXTFLOODFILL:            break;
            case U_EMR_LINETO:                  break;
            case U_EMR_ARCTO:                   break;
            case U_EMR_POLYDRAW:                break;
            case U_EMR_SETARCDIRECTION:         break;
            case U_EMR_SETMITERLIMIT:           break;
            case U_EMR_BEGINPATH:               newPathStruct(states); break;
            case U_EMR_ENDPATH:                 break;
            case U_EMR_CLOSEFIGURE:             break;
            case U_EMR_FILLPATH:                
                if(states->emfStructure.pathStackLast != NULL){
                    states->emfStructure.pathStackLast->pathStruct.fillOffset = off;
                }
                break;
            case U_EMR_STROKEANDFILLPATH:
                if(states->emfStructure.pathStackLast != NULL){
                    states->emfStructure.pathStackLast->pathStruct.strokeFillOffset = off; 
                }
                break;
            case U_EMR_STROKEPATH:
                if(states->emfStructure.pathStackLast != NULL){
                    states->emfStructure.pathStackLast->pathStruct.strokeOffset = off; 
                }
                break;
            case U_EMR_FLATTENPATH:
                if(states->emfStructure.pathStackLast != NULL){
                    states->emfStructure.pathStackLast->pathStruct.flattenOffset = off; 
                }
                break;
            case U_EMR_WIDENPATH:
                if(states->emfStructure.pathStackLast != NULL){
                    states->emfStructure.pathStackLast->pathStruct.widdenOffset = off; 
                }
                break;
            case U_EMR_SELECTCLIPPATH:
                if(states->emfStructure.pathStackLast != NULL){
                    states->emfStructure.pathStackLast->pathStruct.clipOffset = off; 
                }
                break;
            case U_EMR_ABORTPATH:
                if(states->emfStructure.pathStackLast != NULL){
                    states->emfStructure.pathStackLast->pathStruct.abortOffset = off; 
                }
                break;
          //case U_EMR_UNDEF69:                 break;
            case U_EMR_COMMENT:                 break;
            case U_EMR_FILLRGN:                 break;
            case U_EMR_FRAMERGN:                break;
            case U_EMR_INVERTRGN:               break;
            case U_EMR_PAINTRGN:                break;
            case U_EMR_EXTSELECTCLIPRGN:        break;
            case U_EMR_BITBLT:                  break;
            case U_EMR_STRETCHBLT:              break;
            case U_EMR_MASKBLT:                 break;
            case U_EMR_PLGBLT:                  break;
            case U_EMR_SETDIBITSTODEVICE:       break;
            case U_EMR_STRETCHDIBITS:           break;
            case U_EMR_EXTCREATEFONTINDIRECTW:  break;
            case U_EMR_EXTTEXTOUTA:             break;
            case U_EMR_EXTTEXTOUTW:             break;
            case U_EMR_POLYBEZIER16:            break;
            case U_EMR_POLYGON16:               break;
            case U_EMR_POLYLINE16:              break;
            case U_EMR_POLYBEZIERTO16:          break;
            case U_EMR_POLYLINETO16:            break;
            case U_EMR_POLYPOLYLINE16:          break;
            case U_EMR_POLYPOLYGON16:           break;
            case U_EMR_POLYDRAW16:              break;
            case U_EMR_CREATEMONOBRUSH:         break;
            case U_EMR_CREATEDIBPATTERNBRUSHPT: break;
            case U_EMR_EXTCREATEPEN:            break;
          //case U_EMR_POLYTEXTOUTA:            break;
          //case U_EMR_POLYTEXTOUTW:            break;
            case U_EMR_SETICMMODE:              break;
            case U_EMR_CREATECOLORSPACE:        break;
            case U_EMR_SETCOLORSPACE:           break;
            case U_EMR_DELETECOLORSPACE:        break;
          //case U_EMR_GLSRECORD:               break;
          //case U_EMR_GLSBOUNDEDRECORD:        break;
            case U_EMR_PIXELFORMAT:             break;
          //case U_EMR_DRAWESCAPE:              break;
          //case U_EMR_EXTESCAPE:               break;
          //case U_EMR_UNDEF107:                break;
            case U_EMR_SMALLTEXTOUT:            break;
          //case U_EMR_FORCEUFIMAPPING:         break;
          //case U_EMR_NAMEDESCAPE:             break;
          //case U_EMR_COLORCORRECTPALETTE:     break;
          //case U_EMR_SETICMPROFILEA:          break;
          //case U_EMR_SETICMPROFILEW:          break;
            case U_EMR_ALPHABLEND:              break;
            case U_EMR_SETLAYOUT:               break;
            case U_EMR_TRANSPARENTBLT:          break;
          //case U_EMR_UNDEF117:                break;
            case U_EMR_GRADIENTFILL:            break;
          //case U_EMR_SETLINKEDUFIS:           break;
          //case U_EMR_SETTEXTJUSTIFICATION:    break;
          //case U_EMR_COLORMATCHTOTARGETW:     break;
            case U_EMR_CREATECOLORSPACEW:       break;
            default:                            break;
        }  //end of switch
        return(size);
    }



    int emf2svg(char *contents, size_t length, char **out, generatorOptions *options)
    {   
        size_t   off=0;
        size_t   result;
        int      OK =1;
        int      recnum=0;
        PU_ENHMETARECORD pEmr;
        char     *blimit;
        FILE *stream;
        size_t len;

        drawingStates * states = (drawingStates *)calloc(1,sizeof(drawingStates));
        states->verbose = options->verbose;
        states->emfplus = options->emfplus;
        states->imgWidth = options->imgWidth;
        states->imgHeight = options->imgHeight;
        if ((options->nameSpace != NULL) && (strlen(options->nameSpace) != 0)){
            states->nameSpace = options->nameSpace;
            states->nameSpaceString = (char *)calloc(strlen(options->nameSpace)+2, sizeof(char));
            sprintf(states->nameSpaceString, "%s%s", states->nameSpace, ":");
        }
        else{
            states->nameSpaceString = (char *)"";
        }

        states->svgDelimiter = options->svgDelimiter;
        states->currentDeviceContext.font_name = NULL;
        /* initialized to -1 because real size of states->objectTable is always
         * states->objectTableSize + 1 (for easier index manipulation since 
         * indexes in emf files start at 1 and not 0)*/
        states->objectTableSize = -1;
        setTransformIdentity(states);

        stream = open_memstream(out, &len);
        if (stream == NULL){
            if (states->verbose){printf("Failed to allocate output stream\n");}
            return(0);
        }

        blimit = contents + length;

        // analyze emf structure
        while(OK){
            if(off>=length){ //normally should exit from while after EMREOF sets OK to false, this is most likely a corrupt EMF
                if (states->verbose){printf("WARNING: record claims to extend beyond the end of the EMF file\n");}
                return(0);
            }

            pEmr = (PU_ENHMETARECORD)(contents + off);

            if(!recnum && (pEmr->iType != U_EMR_HEADER)){
                if (states->verbose){printf("WARNING: EMF file does not begin with an EMR_HEADER record\n");}
            }
            result = U_emf_onerec_analyse(contents, blimit, recnum, off, states);
            if(result == (size_t) -1){
                if (states->verbose){printf("ABORTING on invalid record - corrupt file?\n");}
                OK=0;
            }
            else if(!result){
                OK=0;
            }
            else { 
                off += result;
                recnum++;
            }
        }  //end of while
        FLAG_RESET;

        OK=1;
        off=0;
        int err=1;
        recnum=0;
        while(OK){
            if(off>=length){ //normally should exit from while after EMREOF sets OK to false, this is most likely a corrupt EMF
                if (states->verbose){printf("WARNING: record claims to extend beyond the end of the EMF file\n");}
                return(0);
            }

            pEmr = (PU_ENHMETARECORD)(contents + off);

            result = U_emf_onerec_draw(contents, blimit, recnum, off, stream, states);
            if(result == (size_t) -1){
                if (states->verbose){printf("ABORTING on invalid record - corrupt file?\n");}
                OK=0;
                err=0;
            }
            else if(!result){
                OK=0;
            }
            else { 
                off += result;
                recnum++;
            }
        }  //end of while
        FLAG_RESET;
        freeObjectTable(states);
        free(states->objectTable);
        freeDeviceContext(&(states->currentDeviceContext));
        freeDeviceContextStack(states);
        free(states);

        fflush(stream);
        fclose(stream);

        return err;
    }

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
