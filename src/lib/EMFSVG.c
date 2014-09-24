/**
  @file uemf_print.c

  @brief Functions for printing EMF records
  */

/*
File:      uemf_print.c
Version:   0.0.15
Date:      17-OCT-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#ifdef __cplusplus
extern "C" {
#endif

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() macro */
#include <string.h>
#include "uemf.h"
#include "EMFSVG.h"
#include "PMFSVG.h"

    //! \cond
#define UNUSED(x) (void)(x)

    /** 
      \brief save the current device context on the stack.
      \param states drawingStates object    
      */

    void saveDeviceContext(drawingStates * states){
        // create the new device context in the stack
        EMF_DEVICE_CONTEXT_STACK * new_entry = (EMF_DEVICE_CONTEXT_STACK *)malloc(sizeof(EMF_DEVICE_CONTEXT_STACK));

        copyDeviceContext(&(new_entry->DeviceContext), &(states->currentDeviceContext));

        // put the new entry on the stack
        new_entry->previous = states->DeviceContextStack;
        states->DeviceContextStack = new_entry;
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
            dest->font_name = (char *)malloc(strlen(src->font_name) + 1);
            strcpy(dest->font_name, src->font_name);    
        }
    }

    void freeDeviceContext(EMF_DEVICE_CONTEXT *dc){
        free(dc->font_name);
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
        copyDeviceContext(&(states->currentDeviceContext), &(stack_entry->DeviceContext));
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
      \brief Print some number of hex bytes
      \param buf pointer to the first byte
      \param num number of bytes
      */
    void hexbytes_print(drawingStates *states, uint8_t *buf,unsigned int num){
        for(; num; num--,buf++){
            verbose_printf("%2.2X",*buf);
        }
    }

    /* **********************************************************************************************
       These functions print standard objects used in the EMR records.
       The low level ones do not append EOL.
     *********************************************************************************************** */


    /** 
      \brief Print a U_COLORREF object.
      \param color  U_COLORREF object    
      */
    void colorref_print(
            drawingStates *states,
            U_COLORREF color
            ){
        verbose_printf("{%u,%u,%u} ",color.Red,color.Green,color.Blue);
    }


    /**
      \brief Print a U_RGBQUAD object.
      \param color  U_RGBQUAD object    
      */
    void rgbquad_print(
            drawingStates *states,
            U_RGBQUAD color
            ){
        verbose_printf("{%u,%u,%u,%u} ",color.Blue,color.Green,color.Red,color.Reserved);
    }

    /**
      \brief Print rect and rectl objects from Upper Left and Lower Right corner points.
      \param rect U_RECTL object
      */
    void rectl_print(
            drawingStates *states,
            U_RECTL rect
            ){
        verbose_printf("{%d,%d,%d,%d} ",rect.left,rect.top,rect.right,rect.bottom);
    }

    /**
      \brief Print a U_SIZEL object.
      \param sz U_SizeL object
      */
    void sizel_print(
            drawingStates *states,
            U_SIZEL sz
            ){
        verbose_printf("{%d,%d} ",sz.cx ,sz.cy);
    } 

    /**
      \brief Print a U_POINTL object
      \param pt U_POINTL object
      */
    void pointl_print(
            drawingStates *states,
            U_POINTL pt
            ){
        verbose_printf("{%d,%d} ",pt.x ,pt.y);
    } 

    /**
      \brief Print a pointer to a U_POINT16 object
      \param pt pointer to a U_POINT16 object
      Warning - WMF data may contain unaligned U_POINT16, do not call
      this routine with a pointer to such data!
      */
    void point16_print(
            drawingStates *states,
            U_POINT16 pt,
            FILE * out
            ){
        verbose_printf("{%d,%d} ",pt.x ,pt.y);
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
        verbose_printf("{%d,%d} ",pt.x ,pt.y);
        double x = (pt.x * states->currentDeviceContext.worldTransform.eM11 + \
                pt.y * states->currentDeviceContext.worldTransform.eM21 + \
                states->currentDeviceContext.worldTransform.eDx) * states->scaling;
        double y = (pt.x * states->currentDeviceContext.worldTransform.eM12 + \
                pt.y * states->currentDeviceContext.worldTransform.eM22 + \
                states->currentDeviceContext.worldTransform.eDy) * states->scaling;
        fprintf(out, "%f %f ", x ,y);
    } 

    void point_draw(
            drawingStates *states,
            U_POINT pt,
            FILE * out
            ){
        verbose_printf("{%d,%d} ",pt.x ,pt.y);
        double x = (pt.x * states->currentDeviceContext.worldTransform.eM11 + \
                pt.y * states->currentDeviceContext.worldTransform.eM21 + \
                states->currentDeviceContext.worldTransform.eDx) * states->scaling;
        double y = (pt.x * states->currentDeviceContext.worldTransform.eM12 + \
                pt.y * states->currentDeviceContext.worldTransform.eM22 + \
                states->currentDeviceContext.worldTransform.eDy) * states->scaling;
        fprintf(out, "%f %f ", x ,y);
    } 



    /**
      \brief Print a U_LCS_GAMMA object
      \param lg U_LCS_GAMMA object
      */
    void lcs_gamma_print(
            drawingStates *states,
            U_LCS_GAMMA lg
            ){
        uint8_t tmp;
        tmp = lg.ignoreHi; verbose_printf("ignoreHi:%u ",tmp);
        tmp = lg.intPart ; verbose_printf("intPart :%u ",tmp);
        tmp = lg.fracPart; verbose_printf("fracPart:%u ",tmp);
        tmp = lg.ignoreLo; verbose_printf("ignoreLo:%u ",tmp);
    }

    /**
      \brief Print a U_LCS_GAMMARGB object
      \param lgr U_LCS_GAMMARGB object
      */
    void lcs_gammargb_print(
            drawingStates *states,
            U_LCS_GAMMARGB lgr
            ){
        verbose_printf("lcsGammaRed:");   lcs_gamma_print(states, lgr.lcsGammaRed  );
        verbose_printf("lcsGammaGreen:"); lcs_gamma_print(states, lgr.lcsGammaGreen);
        verbose_printf("lcsGammaBlue:");  lcs_gamma_print(states, lgr.lcsGammaBlue );
    }

    /**
      \brief Print a U_TRIVERTEX object.
      \param tv U_TRIVERTEX object.
      */
    void trivertex_print(
            drawingStates *states,
            U_TRIVERTEX tv
            ){
        verbose_printf("{{%d,%d},{%u,%u,%u,%u}} ",tv.x,tv.y,tv.Red,tv.Green,tv.Blue,tv.Alpha);
    }

    /**
      \brief Print a U_GRADIENT3 object.
      \param g3 U_GRADIENT3 object.
      */
    void gradient3_print(
            drawingStates *states,
            U_GRADIENT3 g3
            ){
        verbose_printf("{%u,%u,%u} ",g3.Vertex1,g3.Vertex2,g3.Vertex3);
    }

    /**
      \brief Print a U_GRADIENT4 object.
      \param g4 U_GRADIENT4 object.
      */
    void gradient4_print(
            drawingStates *states,
            U_GRADIENT4 g4
            ){
        verbose_printf("{%u,%u} ",g4.UpperLeft,g4.LowerRight);
    }

    /**
      \brief Print a U_LOGBRUSH object.
      \param lb U_LOGBRUSH object.
      */
    void logbrush_print(
            drawingStates *states,
            U_LOGBRUSH lb  
            ){
        verbose_printf("lbStyle:0x%8.8X ",  lb.lbStyle);
        verbose_printf("lbColor:");         colorref_print(states, lb.lbColor);
        verbose_printf("lbHatch:0x%8.8X ",  lb.lbHatch);
    }

    /**
      \brief Print a U_XFORM object.
      \param xform U_XFORM object
      */
    void xform_print(
            drawingStates *states,
            U_XFORM xform
            ){
        verbose_printf("{%f,%f.%f,%f,%f,%f} ",xform.eM11,xform.eM12,xform.eM21,xform.eM22,xform.eDx,xform.eDy);
    }

    /**
      \brief Print a U_CIEXYZ object
      \param ciexyz U_CIEXYZ object
      */
    void ciexyz_print(
            drawingStates *states,
            U_CIEXYZ ciexyz
            ){
        verbose_printf("{%d,%d.%d} ",ciexyz.ciexyzX,ciexyz.ciexyzY,ciexyz.ciexyzZ);

    }

    /**
      \brief Print a U_CIEXYZTRIPLE object
      \param cie3 U_CIEXYZTRIPLE object
      */
    void ciexyztriple_print(
            drawingStates *states,
            U_CIEXYZTRIPLE cie3
            ){
        verbose_printf("{Red:");     ciexyz_print(states, cie3.ciexyzRed  );
        verbose_printf(", Green:");  ciexyz_print(states, cie3.ciexyzGreen);
        verbose_printf(", Blue:");   ciexyz_print(states, cie3.ciexyzBlue );
        verbose_printf("} ");
    }
    /**
      \brief Print a U_LOGCOLORSPACEA object.
      \param lcsa     U_LOGCOLORSPACEA object
      */
    void logcolorspacea_print(
            drawingStates *states,
            U_LOGCOLORSPACEA lcsa
            ){
        verbose_printf("lcsSignature:%u ",lcsa.lcsSignature);
        verbose_printf("lcsVersion:%u ",  lcsa.lcsVersion  );
        verbose_printf("lcsSize:%u ",     lcsa.lcsSize     );
        verbose_printf("lcsCSType:%d ",    lcsa.lcsCSType   );
        verbose_printf("lcsIntent:%d ",    lcsa.lcsIntent   );
        verbose_printf("lcsEndpoints:");   ciexyztriple_print(states, lcsa.lcsEndpoints);
        verbose_printf("lcsGammaRGB: ");   lcs_gammargb_print(states, lcsa.lcsGammaRGB );
        verbose_printf("filename:%s ",     lcsa.lcsFilename );
    }

    /**

      \brief Print a U_LOGCOLORSPACEW object.
      \param lcsa U_LOGCOLORSPACEW object                                               
      */
    void logcolorspacew_print(
            drawingStates *states,
            U_LOGCOLORSPACEW lcsa
            ){
        char *string;
        verbose_printf("lcsSignature:%d ",lcsa.lcsSignature);
        verbose_printf("lcsVersion:%d ",  lcsa.lcsVersion  );
        verbose_printf("lcsSize:%d ",     lcsa.lcsSize     );
        verbose_printf("lcsCSType:%d ",   lcsa.lcsCSType   );
        verbose_printf("lcsIntent:%d ",   lcsa.lcsIntent   );
        verbose_printf("lcsEndpoints:");   ciexyztriple_print(states, lcsa.lcsEndpoints);
        verbose_printf("lcsGammaRGB: ");   lcs_gammargb_print(states,lcsa.lcsGammaRGB );
        string = U_Utf16leToUtf8(lcsa.lcsFilename, U_MAX_PATH, NULL);
        verbose_printf("filename:%s ",   string );
        free(string);
    }

    /**
      \brief Print a U_PANOSE object.
      \param panose U_PANOSE object
      */
    void panose_print(
            drawingStates *states,
            U_PANOSE panose
            ){
        verbose_printf("bFamilyType:%u ",     panose.bFamilyType     );
        verbose_printf("bSerifStyle:%u ",     panose.bSerifStyle     );
        verbose_printf("bWeight:%u ",         panose.bWeight         );
        verbose_printf("bProportion:%u ",     panose.bProportion     );
        verbose_printf("bContrast:%u ",       panose.bContrast       );
        verbose_printf("bStrokeVariation:%u ",panose.bStrokeVariation);
        verbose_printf("bArmStyle:%u ",       panose.bArmStyle       );
        verbose_printf("bLetterform:%u ",     panose.bLetterform     );
        verbose_printf("bMidline:%u ",        panose.bMidline        );
        verbose_printf("bXHeight:%u ",        panose.bXHeight        );
    }

    /**
      \brief Print a U_LOGFONT object.
      \param lf   U_LOGFONT object
      */
    void logfont_print(
            drawingStates *states,
            U_LOGFONT lf
            ){
        char *string;
        verbose_printf("lfHeight:%d ",            lf.lfHeight        );
        verbose_printf("lfWidth:%d ",             lf.lfWidth         );
        verbose_printf("lfEscapement:%d ",        lf.lfEscapement    );
        verbose_printf("lfOrientation:%d ",       lf.lfOrientation   );
        verbose_printf("lfWeight:%d ",            lf.lfWeight        );
        verbose_printf("lfItalic:0x%2.2X ",         lf.lfItalic        );
        verbose_printf("lfUnderline:0x%2.2X ",      lf.lfUnderline     );
        verbose_printf("lfStrikeOut:0x%2.2X ",      lf.lfStrikeOut     );
        verbose_printf("lfCharSet:0x%2.2X ",        lf.lfCharSet       );
        verbose_printf("lfOutPrecision:0x%2.2X ",   lf.lfOutPrecision  );
        verbose_printf("lfClipPrecision:0x%2.2X ",  lf.lfClipPrecision );
        verbose_printf("lfQuality:0x%2.2X ",        lf.lfQuality       );
        verbose_printf("lfPitchAndFamily:0x%2.2X ", lf.lfPitchAndFamily);
        string = U_Utf16leToUtf8(lf.lfFaceName, U_LF_FACESIZE, NULL);
        verbose_printf("lfFaceName:%s ",   string );
        free(string);
    }

    /**
      \brief Print a U_LOGFONT_PANOSE object.
      \return U_LOGFONT_PANOSE object
      */
    void logfont_panose_print(
            drawingStates *states,
            U_LOGFONT_PANOSE lfp
            ){    
        char *string;
        verbose_printf("elfLogFont:");       logfont_print(states, lfp.elfLogFont);
        string = U_Utf16leToUtf8(lfp.elfFullName, U_LF_FULLFACESIZE, NULL);
        verbose_printf("elfFullName:%s ",    string );
        free(string);
        string = U_Utf16leToUtf8(lfp.elfStyle, U_LF_FACESIZE, NULL);
        verbose_printf("elfStyle:%s ",       string );
        free(string);
        verbose_printf("elfVersion:%u "      ,lfp.elfVersion  );
        verbose_printf("elfStyleSize:%u "    ,lfp.elfStyleSize);
        verbose_printf("elfMatch:%u "        ,lfp.elfMatch    );
        verbose_printf("elfReserved:%u "     ,lfp.elfReserved );
        verbose_printf("elfVendorId:");      hexbytes_print(states, (uint8_t *)lfp.elfVendorId,U_ELF_VENDOR_SIZE); verbose_printf(" ");
        verbose_printf("elfCulture:%u "      ,lfp.elfCulture  );
        verbose_printf("elfPanose:");        panose_print(states, lfp.elfPanose);
    }

    /**
      \brief Print a pointer to U_BITMAPINFOHEADER object.

      This may be called indirectly from WMF _print routines, where problems could occur
      if the data was passed as the struct or a pointer to the struct, as the struct may not
      be aligned in memory.

      \returns Actual number of color table entries.
      \param Bmih pointer to a U_BITMAPINFOHEADER object
      */
    int bitmapinfoheader_print(
            drawingStates *states,
            const char *Bmih
            ){
        uint32_t  utmp4;
        int32_t   tmp4;
        int16_t   tmp2;
        int       Colors, BitCount, Width, Height, RealColors;

        /* DIB from a WMF may not be properly aligned on a 4 byte boundary, will be aligned on a 2 byte boundary */

        memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER,biSize),          4);  verbose_printf("biSize:%u "            ,utmp4 );
        memcpy(&tmp4,  Bmih + offsetof(U_BITMAPINFOHEADER,biWidth),         4);  verbose_printf("biWidth:%d "           ,tmp4  );
        Width = tmp4;
        memcpy(&tmp4,  Bmih + offsetof(U_BITMAPINFOHEADER,biHeight),        4);  verbose_printf("biHeight:%d "          ,tmp4  );
        Height = tmp4;
        memcpy(&tmp2,  Bmih + offsetof(U_BITMAPINFOHEADER,biPlanes),        2);  verbose_printf("biPlanes:%u "          ,tmp2  );
        memcpy(&tmp2,  Bmih + offsetof(U_BITMAPINFOHEADER,biBitCount),      2);  verbose_printf("biBitCount:%u "        ,tmp2  );
        BitCount = tmp2;
        memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER,biCompression),   4);  verbose_printf("biCompression:%u "     ,utmp4 );
        memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER,biSizeImage),     4);  verbose_printf("biSizeImage:%u "       ,utmp4 );
        memcpy(&tmp4,  Bmih + offsetof(U_BITMAPINFOHEADER,biXPelsPerMeter), 4);  verbose_printf("biXPelsPerMeter:%d "   ,tmp4  );
        memcpy(&tmp4,  Bmih + offsetof(U_BITMAPINFOHEADER,biYPelsPerMeter), 4);  verbose_printf("biYPelsPerMeter:%d "   ,tmp4  );
        memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER,biClrUsed),       4);  verbose_printf("biClrUsed:%u "         ,utmp4 );
        Colors = utmp4;
        memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER,biClrImportant),  4);  verbose_printf("biClrImportant:%u "    ,utmp4 );
        RealColors = get_real_color_icount(Colors, BitCount, Width, Height);
        verbose_printf("ColorEntries:%d ",RealColors);
        return(RealColors);
    }


    /**
      \brief Print a Pointer to a U_BITMAPINFO object.
      \param Bmi Pointer to a U_BITMAPINFO object
      This may be called from WMF _print routines, where problems could occur
      if the data was passed as the struct or a pointer to the struct, as the struct may not
      be aligned in memory.
      */
    void bitmapinfo_print(
            drawingStates *states,
            const char *Bmi
            ){
        int       i,k;
        int       ClrUsed;
        U_RGBQUAD BmiColor;
        verbose_printf("BmiHeader: ");
        ClrUsed = bitmapinfoheader_print(states, Bmi + offsetof(U_BITMAPINFO,bmiHeader));
        if(ClrUsed){
            k= offsetof(U_BITMAPINFO,bmiColors);
            for(i=0; i<ClrUsed; i++, k+= sizeof(U_RGBQUAD)){
                memcpy(&BmiColor, Bmi+k, sizeof(U_RGBQUAD));
                verbose_printf("%d:",i); rgbquad_print(states, BmiColor);
            }
        }
    }

    /**
      \brief Print a U_BLEND object.
      \param blend a U_BLEND object
      */
    void blend_print(
            drawingStates *states,
            U_BLEND blend
            ){
        verbose_printf("Operation:%u " ,blend.Operation);
        verbose_printf("Flags:%u "     ,blend.Flags    );
        verbose_printf("Global:%u "    ,blend.Global   );
        verbose_printf("Op:%u "        ,blend.Op       );
    }

    /**
      \brief Print a pointer to a U_EXTLOGPEN object.
      \param elp   PU_EXTLOGPEN object
      */
    void extlogpen_print(
            drawingStates *states,
            PU_EXTLOGPEN elp
            ){
        unsigned int i;
        U_STYLEENTRY *elpStyleEntry;
        verbose_printf("elpPenStyle:0x%8.8X "   ,elp->elpPenStyle  );
        verbose_printf("elpWidth:%u "           ,elp->elpWidth     );
        verbose_printf("elpBrushStyle:0x%8.8X " ,elp->elpBrushStyle);
        verbose_printf("elpColor");              colorref_print(states, elp->elpColor);
        verbose_printf("elpHatch:%d "           ,elp->elpHatch     );
        verbose_printf("elpNumEntries:%u "      ,elp->elpNumEntries);
        if(elp->elpNumEntries){
            verbose_printf("elpStyleEntry:");
            elpStyleEntry = (uint32_t *) elp->elpStyleEntry;
            for(i=0;i<elp->elpNumEntries;i++){
                verbose_printf("%d:%u ",i,elpStyleEntry[i]);
            }
        }
    }

    /**
      \brief Print a U_LOGPEN object.
      \param lp  U_LOGPEN object

*/
    void logpen_print(
            drawingStates *states,
            U_LOGPEN lp
            ){
        verbose_printf("lopnStyle:0x%8.8X "    ,lp.lopnStyle  );
        verbose_printf("lopnWidth:");      pointl_print(states,  lp.lopnWidth  );
        verbose_printf("lopnColor:");      colorref_print(states, lp.lopnColor );
    } 

    /**
      \brief Print a U_LOGPLTNTRY object.
      \param lpny Ignore U_LOGPLTNTRY object.
      */
    void logpltntry_print(
            drawingStates *states,
            U_LOGPLTNTRY lpny
            ){
        verbose_printf("peReserved:%u " ,lpny.peReserved );
        verbose_printf("peRed:%u "      ,lpny.peRed      );
        verbose_printf("peGreen:%u "    ,lpny.peGreen    );
        verbose_printf("peBlue:%u "     ,lpny.peBlue     );
    }

    /**
      \brief Print a pointer to a U_LOGPALETTE object.
      \param lp  Pointer to a U_LOGPALETTE object.
      */
    void logpalette_print(
            drawingStates *states,
            PU_LOGPALETTE lp
            ){
        int            i;
        PU_LOGPLTNTRY palPalEntry;
        verbose_printf("palVersion:%u ",    lp->palVersion );
        verbose_printf("palNumEntries:%u ", lp->palNumEntries );
        if(lp->palNumEntries){
            palPalEntry = (PU_LOGPLTNTRY) &(lp->palPalEntry);
            for(i=0;i<lp->palNumEntries;i++){
                verbose_printf("%d:",i); logpltntry_print(states, palPalEntry[i]);
            }
        }
    }

    /**
      \brief Print a U_RGNDATAHEADER object.
      \param rdh  U_RGNDATAHEADER object
      */
    void rgndataheader_print(
            drawingStates *states,
            U_RGNDATAHEADER rdh
            ){
        verbose_printf("dwSize:%u ",   rdh.dwSize   );
        verbose_printf("iType:%u ",    rdh.iType    );
        verbose_printf("nCount:%u ",   rdh.nCount   );
        verbose_printf("nRgnSize:%u ", rdh.nRgnSize );
        verbose_printf("rclBounds:");  rectl_print(states, rdh.rclBounds  );
    }

    /**
      \brief Print a pointer to a U_RGNDATA object.
      \param rd  pointer to a U_RGNDATA object.
      */
    void rgndata_print(
            drawingStates *states,
            PU_RGNDATA rd
            ){
        unsigned int i;
        PU_RECTL rects;
        verbose_printf("rdh:");     rgndataheader_print(states, rd->rdh );
        if(rd->rdh.nCount){
            rects = (PU_RECTL) &(rd->Buffer);
            for(i=0;i<rd->rdh.nCount;i++){
                verbose_printf("%d:",i); rectl_print(states, rects[i]);
            }
        }
    }

    /**
      \brief Print a U_COLORADJUSTMENT object.
      \param ca U_COLORADJUSTMENT object.        
      */
    void coloradjustment_print(
            drawingStates *states,
            U_COLORADJUSTMENT ca
            ){
        verbose_printf("caSize:%u "            ,ca.caSize           );
        verbose_printf("caFlags:0x%4.4X "        ,ca.caFlags          );
        verbose_printf("caIlluminantIndex:%u " ,ca.caIlluminantIndex);
        verbose_printf("caRedGamma:%u "        ,ca.caRedGamma       );
        verbose_printf("caGreenGamma:%u "      ,ca.caGreenGamma     );
        verbose_printf("caBlueGamma:%u "       ,ca.caBlueGamma      );
        verbose_printf("caReferenceBlack:%u "  ,ca.caReferenceBlack );
        verbose_printf("caReferenceWhite:%u "  ,ca.caReferenceWhite );
        verbose_printf("caContrast:%d "         ,ca.caContrast       );
        verbose_printf("caBrightness:%d "       ,ca.caBrightness     );
        verbose_printf("caColorfulness:%d "     ,ca.caColorfulness   );
        verbose_printf("caRedGreenTint:%d "     ,ca.caRedGreenTint   );
    }

    /**
      \brief Print a U_PIXELFORMATDESCRIPTOR object.
      \param pfd  U_PIXELFORMATDESCRIPTOR object
      */
    void pixelformatdescriptor_print(
            drawingStates *states,
            U_PIXELFORMATDESCRIPTOR pfd
            ){
        verbose_printf("nSize:%u "           ,pfd.nSize           );
        verbose_printf("nVersion:%u "        ,pfd.nVersion        );
        verbose_printf("dwFlags:0x%8.8X "      ,pfd.dwFlags         );
        verbose_printf("iPixelType:%u "      ,pfd.iPixelType      );
        verbose_printf("cColorBits:%u "      ,pfd.cColorBits      );
        verbose_printf("cRedBits:%u "        ,pfd.cRedBits        );
        verbose_printf("cRedShift:%u "       ,pfd.cRedShift       );
        verbose_printf("cGreenBits:%u "      ,pfd.cGreenBits      );
        verbose_printf("cGreenShift:%u "     ,pfd.cGreenShift     );
        verbose_printf("cBlueBits:%u "       ,pfd.cBlueBits       );
        verbose_printf("cBlueShift:%u "      ,pfd.cBlueShift      );
        verbose_printf("cAlphaBits:%u "      ,pfd.cAlphaBits      );
        verbose_printf("cAlphaShift:%u "     ,pfd.cAlphaShift     );
        verbose_printf("cAccumBits:%u "      ,pfd.cAccumBits      );
        verbose_printf("cAccumRedBits:%u "   ,pfd.cAccumRedBits   );
        verbose_printf("cAccumGreenBits:%u " ,pfd.cAccumGreenBits );
        verbose_printf("cAccumBlueBits:%u "  ,pfd.cAccumBlueBits  );
        verbose_printf("cAccumAlphaBits:%u " ,pfd.cAccumAlphaBits );
        verbose_printf("cDepthBits:%u "      ,pfd.cDepthBits      );
        verbose_printf("cStencilBits:%u "    ,pfd.cStencilBits    );
        verbose_printf("cAuxBuffers:%u "     ,pfd.cAuxBuffers     );
        verbose_printf("iLayerType:%u "      ,pfd.iLayerType      );
        verbose_printf("bReserved:%u "       ,pfd.bReserved       );
        verbose_printf("dwLayerMask:%u "     ,pfd.dwLayerMask     );
        verbose_printf("dwVisibleMask:%u "   ,pfd.dwVisibleMask   );
        verbose_printf("dwDamageMask:%u "    ,pfd.dwDamageMask    );
    }

    /**
      \brief Print a Pointer to a U_EMRTEXT record
      \param emt      Pointer to a U_EMRTEXT record 
      \param record   Pointer to the start of the record which contains this U_ERMTEXT
      \param type     0 for 8 bit character, anything else for 16 
      */
    void emrtext_print(
            drawingStates *states,
            const char *emt,
            const char *record,
            int         type
            ){
        unsigned int i,off;
        char *string;
        PU_EMRTEXT pemt = (PU_EMRTEXT) emt;
        // constant part
        verbose_printf("ptlReference:");   pointl_print(states, pemt->ptlReference);
        verbose_printf("nChars:%u "       ,pemt->nChars      );
        verbose_printf("offString:%u "    ,pemt->offString   );
        if(pemt->offString){
            if(!type){
                verbose_printf("string8:<%s> ",record + pemt->offString);
            }
            else {
                string = U_Utf16leToUtf8((uint16_t *)(record + pemt->offString), pemt->nChars, NULL);
                verbose_printf("string16:<%s> ",string);
                free(string);
            }
        }
        verbose_printf("fOptions:0x%8.8X "     ,pemt->fOptions    );
        off = sizeof(U_EMRTEXT);
        if(!(pemt->fOptions & U_ETO_NO_RECT)){
            verbose_printf("rcl");   rectl_print(states, *((U_RECTL *)(emt+off)) );
            off += sizeof(U_RECTL);
        }
        verbose_printf("offDx:%u "        , *((U_OFFDX *)(emt+off))   ); off = *(U_OFFDX *)(emt+off);
        verbose_printf("Dx:");
        for(i=0; i<pemt->nChars; i++, off+=sizeof(uint32_t)){
            verbose_printf("%d:", *((uint32_t *)(record+off))  );
        }
    }




    // hide these from Doxygen
    //! @cond
    /* **********************************************************************************************
       These functions contain shared code used by various U_EMR*_print functions.  These should NEVER be called
       by end user code and to further that end prototypes are NOT provided and they are hidden from Doxygen.


       These are (mostly) ordered by U_EMR_* index number.

       The exceptions:
       void core3_print(const char *name, const char *label, const char *contents, FILE *out, drawingStates *states)
       void core7_print(const char *name, const char *field1, const char *field2, const char *contents, FILE *out, drawingStates *states)
       void core8_print(const char *name, const char *contents, FILE *out, drawingStates *states, int type)


     *********************************************************************************************** */


    // Functions with the same form starting with U_EMRPOLYBEZIER_print
    void core1_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        unsigned int i;
        UNUSED(name);
        PU_EMRPOLYLINETO pEmr = (PU_EMRPOLYLINETO) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   cptl:           %d\n",pEmr->cptl        );
        verbose_printf("   Points:         ");
        for(i=0;i<pEmr->cptl; i++){
            verbose_printf("[%d]:",i); pointl_print(states, pEmr->aptl[i]);
        }
        verbose_printf("\n");
    }

    // Functions with the same form starting with U_EMRPOLYPOLYLINE_print
    void core2_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        unsigned int i;
        UNUSED(name);
        PU_EMRPOLYPOLYGON pEmr = (PU_EMRPOLYPOLYGON) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   nPolys:         %d\n",pEmr->nPolys        );
        verbose_printf("   cptl:           %d\n",pEmr->cptl          );
        verbose_printf("   Counts:         ");
        for(i=0;i<pEmr->nPolys; i++){
            verbose_printf(" [%d]:%d ",i,pEmr->aPolyCounts[i] );
        }
        verbose_printf("\n");
        PU_POINTL paptl = (PU_POINTL)((char *)pEmr->aPolyCounts + sizeof(uint32_t)* pEmr->nPolys);
        verbose_printf("   Points:         ");
        for(i=0;i<pEmr->cptl; i++){
            verbose_printf(" [%d]:",i); pointl_print(states, paptl[i]);
        }
        verbose_printf("\n");
    }


    // Functions with the same form starting with U_EMRSETMAPMODE_print
    void core3_print(const char *name, const char *label, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents);
        if(!strcmp(label,"crColor:")){
            verbose_printf("   %-15s ",label); colorref_print(states, *(U_COLORREF *)&(pEmr->iMode)); verbose_printf("\n");
        }
        else if(!strcmp(label,"iMode:")){
            verbose_printf("   %-15s 0x%8.8X\n",label,pEmr->iMode     );
        }
        else {
            verbose_printf("   %-15s %d\n",label,pEmr->iMode        );
        }
    } 

    // Functions taking a single U_RECT or U_RECTL, starting with U_EMRELLIPSE_print, also U_EMRFILLPATH_print, 
    void core4_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRELLIPSE pEmr      = (PU_EMRELLIPSE)(   contents);
        verbose_printf("   rclBox:         ");  rectl_print(states, pEmr->rclBox);  verbose_printf("\n");
    } 

    // Functions with the same form starting with U_EMRPOLYBEZIER16_print
    void core6_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   cpts:           %d\n",pEmr->cpts        );
        verbose_printf("   Points:         ");
        PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
        for(i=0; i<pEmr->cpts; i++){
            verbose_printf(" [%d]:",i);  point16_print(states, papts[i], out);
        }
        verbose_printf("\n");
    } 

    // Functions with the same form starting with U_EMRPOLYBEZIER16_print
    void cubic_bezier_draw(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   cpts:           %d\n",pEmr->cpts        );
        verbose_printf("   Points:         ");
        PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
        for(i=0; i<pEmr->cpts; i++){
            switch ( i % 3 ) {
                case 0:
                    fprintf(out, "C ");
                    verbose_printf(" [%d]:",i);  point16_draw(states, papts[i], out);
                    break;
                case 1:
                    verbose_printf(" [%d]:",i);  point16_draw(states, papts[i], out);
                    break;
                case 2:
                    verbose_printf(" [%d]:",i);  point16_draw(states, papts[i], out);
                    break;
            }
        }
        verbose_printf("\n");
    } 



    // Functions drawing a polyline
    void polyline_draw(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   cpts:           %d\n",pEmr->cpts        );
        verbose_printf("   Points:         ");
        PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
        for(i=0; i<pEmr->cpts; i++){
            fprintf(out, "L ");
            verbose_printf(" [%d]:",i);  point16_draw(states, papts[i], out);
        }
        verbose_printf("\n");
    } 

    void moveto_draw(const char *name, const char *field1, const char *field2, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);
        //fprintf(out, "%d %d ", pEmr->pair.x, pEmr->pair.y);
        point_draw(states,pEmr->pair,out);
        if(*field2){
            verbose_printf("   %-15s %d\n",field1,pEmr->pair.x);
            verbose_printf("   %-15s %d\n",field2,pEmr->pair.y);
        }
        else {
            verbose_printf("   %-15s {%d,%d}\n",field1,pEmr->pair.x,pEmr->pair.y);
        } 
    }



    void lineto_draw(const char *name, const char *field1, const char *field2, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);
        fprintf(out, "L ");
        point_draw(states,pEmr->pair,out);
        if(*field2){
            verbose_printf("   %-15s %d\n",field1,pEmr->pair.x);
            verbose_printf("   %-15s %d\n",field2,pEmr->pair.y);
        }
        else {
            verbose_printf("   %-15s {%d,%d}\n",field1,pEmr->pair.x,pEmr->pair.y);
        } 
    }

    // Records with the same form starting with U_EMRSETWINDOWEXTEX_print
    // CAREFUL, in the _set equivalents all functions with two uint32_t values are mapped here, and member names differ, consequently
    //   print routines must supply the names of the two arguments.  These cannot be null.  If the second one is 
    //   empty the values are printed as a pair {x,y}, otherwise each is printed with its own label on a separate line.
    void core7_print(const char *name, const char *field1, const char *field2, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents);
        if(*field2){
            verbose_printf("   %-15s %d\n",field1,pEmr->pair.x);
            verbose_printf("   %-15s %d\n",field2,pEmr->pair.y);
        }
        else {
            verbose_printf("   %-15s {%d,%d}\n",field1,pEmr->pair.x,pEmr->pair.y);
        } 
    }



    // For U_EMREXTTEXTOUTA and U_EMREXTTEXTOUTW, type=0 for the first one
    void core8_print(const char *name, const char *contents, FILE *out, drawingStates *states, int type){
        UNUSED(name);
        PU_EMREXTTEXTOUTA pEmr = (PU_EMREXTTEXTOUTA) (contents);
        verbose_printf("   iGraphicsMode:  %u\n",pEmr->iGraphicsMode );
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);                              verbose_printf("\n");
        verbose_printf("   exScale:        %f\n",pEmr->exScale        );
        verbose_printf("   eyScale:        %f\n",pEmr->eyScale        );
        verbose_printf("   emrtext:        ");
        emrtext_print(states, contents + sizeof(U_EMREXTTEXTOUTA) - sizeof(U_EMRTEXT),contents,type);
        verbose_printf("\n");
    } 

    // Functions that take a rect and a pair of points, starting with U_EMRARC_print
    void core9_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRARC pEmr = (PU_EMRARC) (contents);
        verbose_printf("   rclBox:         ");    rectl_print(states, pEmr->rclBox);    verbose_printf("\n");
        verbose_printf("   ptlStart:       ");  pointl_print(states, pEmr->ptlStart);   verbose_printf("\n");
        verbose_printf("   ptlEnd:         ");  pointl_print(states, pEmr->ptlEnd);   verbose_printf("\n");
    }

    // Functions with the same form starting with U_EMRPOLYPOLYLINE16_print
    void core10_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        unsigned int i;
        PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   nPolys:         %d\n",pEmr->nPolys        );
        verbose_printf("   cpts:           %d\n",pEmr->cpts          );
        verbose_printf("   Counts:         ");
        for(i=0;i<pEmr->nPolys; i++){
            verbose_printf(" [%d]:%d ",i,pEmr->aPolyCounts[i] );
        }
        verbose_printf("\n");
        verbose_printf("   Points:         ");
        PU_POINT16 papts = (PU_POINT16)((char *)pEmr->aPolyCounts + sizeof(uint32_t)* pEmr->nPolys);
        for(i=0; i<pEmr->cpts; i++){
            verbose_printf(" [%d]:",i);  point16_print(states, papts[i], out);
        }
        verbose_printf("\n");

    } 

    // Functions with the same form starting with  U_EMRINVERTRGN_print and U_EMRPAINTRGN_print,
    void core11_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        unsigned int i,roff;
        PU_EMRINVERTRGN pEmr = (PU_EMRINVERTRGN) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   cbRgnData:      %d\n",pEmr->cbRgnData);
        // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
        roff=0;
        i=1; 
        char *prd = (char *) &(pEmr->RgnData);
        while(roff + sizeof(U_RGNDATAHEADER) < pEmr->cbRgnData){ // stop at end of the record 4*4 = header + 4*4=rect
            verbose_printf("   RegionData:%d",i);
            rgndata_print(states, (PU_RGNDATA) (prd + roff));
            roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
            verbose_printf("\n");
        }
    } 


    // common code for U_EMRCREATEMONOBRUSH_print and U_EMRCREATEDIBPATTERNBRUSHPT_print,
    void core12_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRCREATEMONOBRUSH pEmr = (PU_EMRCREATEMONOBRUSH) (contents);
        verbose_printf("   ihBrush:      %u\n",pEmr->ihBrush );
        verbose_printf("   iUsage :      %u\n",pEmr->iUsage  );
        verbose_printf("   offBmi :      %u\n",pEmr->offBmi  );
        verbose_printf("   cbBmi  :      %u\n",pEmr->cbBmi   );
        if(pEmr->cbBmi){
            verbose_printf("      bitmap:");
            bitmapinfo_print(states, contents + pEmr->offBmi);
            verbose_printf("\n");
        }
        verbose_printf("   offBits:      %u\n",pEmr->offBits );
        verbose_printf("   cbBits :      %u\n",pEmr->cbBits  );
    }

    // common code for U_EMRALPHABLEND_print and U_EMRTRANSPARENTBLT_print,
    void core13_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        PU_EMRALPHABLEND pEmr = (PU_EMRALPHABLEND) (contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);       verbose_printf("\n");
        verbose_printf("   Dest:           ");    pointl_print(states, pEmr->Dest);            verbose_printf("\n");
        verbose_printf("   cDest:          ");    pointl_print(states, pEmr->cDest);           verbose_printf("\n");
        verbose_printf("   Blend:          ");    blend_print(states, pEmr->Blend);            verbose_printf("\n");
        verbose_printf("   Src:            ");    pointl_print(states, pEmr->Src);             verbose_printf("\n");
        verbose_printf("   xformSrc:       ");    xform_print(states, pEmr->xformSrc);        verbose_printf("\n");
        verbose_printf("   crBkColorSrc:   ");    colorref_print(states, pEmr->crBkColorSrc); verbose_printf("\n");
        verbose_printf("   iUsageSrc:      %u\n",pEmr->iUsageSrc   );
        verbose_printf("   offBmiSrc:      %u\n",pEmr->offBmiSrc   );
        verbose_printf("   cbBmiSrc:       %u\n",pEmr->cbBmiSrc    );
        if(pEmr->cbBmiSrc){
            verbose_printf("      bitmap:");
            bitmapinfo_print(states, contents + pEmr->offBmiSrc);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsSrc:     %u\n",pEmr->offBitsSrc  );
        verbose_printf("   cbBitsSrc:      %u\n",pEmr->cbBitsSrc   );
    }
    //! @endcond

    /* **********************************************************************************************
       These are the core EMR functions, each creates a particular type of record.  
       All return these records via a char* pointer, which is NULL if the call failed.  
       They are listed in order by the corresponding U_EMR_* index number.  
     *********************************************************************************************** */

    /**
      \brief Print a pointer to a U_EMR_whatever record which has not been implemented.
      \param name       name of this type of record
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRNOTIMPLEMENTED_print(const char *name, const char *contents, FILE *out, drawingStates *states){
        UNUSED(name);
        UNUSED(contents);
        verbose_printf("   Not Implemented!\n");
    }

    // U_EMRHEADER                1
    /**
      \brief Print a pointer to a U_EMR_HEADER record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRHEADER_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL
            char *string;
        int  p1len;

        PU_EMRHEADER pEmr = (PU_EMRHEADER)(contents);
        verbose_printf("   rclBounds:      ");          rectl_print(states, pEmr->rclBounds);   verbose_printf("\n");
        verbose_printf("   rclFrame:       ");          rectl_print(states, pEmr->rclFrame);    verbose_printf("\n");
        verbose_printf("   dSignature:     0x%8.8X\n",  pEmr->dSignature    );
        verbose_printf("   nVersion:       0x%8.8X\n",  pEmr->nVersion      );
        verbose_printf("   nBytes:         %d\n",       pEmr->nBytes        );
        verbose_printf("   nRecords:       %d\n",       pEmr->nRecords      );
        verbose_printf("   nHandles:       %d\n",       pEmr->nHandles      );
        verbose_printf("   sReserved:      %d\n",       pEmr->sReserved     );
        verbose_printf("   nDescription:   %d\n",       pEmr->nDescription  );
        verbose_printf("   offDescription: %d\n",       pEmr->offDescription);
        if(pEmr->offDescription){
            string = U_Utf16leToUtf8((uint16_t *)((char *) pEmr + pEmr->offDescription), pEmr->nDescription, NULL);
            verbose_printf("      Desc. A:  %s\n",string);
            free(string);
            p1len = 2 + 2*wchar16len((uint16_t *)((char *) pEmr + pEmr->offDescription));
            string = U_Utf16leToUtf8((uint16_t *)((char *) pEmr + pEmr->offDescription + p1len), pEmr->nDescription, NULL);
            verbose_printf("      Desc. B:  %s\n",string);
            free(string);
        }
        verbose_printf("   nPalEntries:    %d\n",       pEmr->nPalEntries   );
        verbose_printf("   szlDevice:      {%d,%d} \n", pEmr->szlDevice.cx,pEmr->szlDevice.cy);
        verbose_printf("   szlMillimeters: {%d,%d} \n", pEmr->szlMillimeters.cx,pEmr->szlMillimeters.cy);
        if((pEmr->nDescription && (pEmr->offDescription >= 100)) || 
                (!pEmr->offDescription && pEmr->emr.nSize >= 100)
          ){
            verbose_printf("   cbPixelFormat:  %d\n",       pEmr->cbPixelFormat );
            verbose_printf("   offPixelFormat: %d\n",       pEmr->offPixelFormat);
            if(pEmr->cbPixelFormat){
                verbose_printf("      PFD:");
                pixelformatdescriptor_print(states, *(PU_PIXELFORMATDESCRIPTOR) (contents + pEmr->offPixelFormat));
                verbose_printf("\n");
            }
            verbose_printf("   bOpenGL:        %d\n",pEmr->bOpenGL       );
            if((pEmr->nDescription    && (pEmr->offDescription >= 108)) || 
                    (pEmr->cbPixelFormat   && (pEmr->offPixelFormat >=108)) ||
                    (!pEmr->offDescription && !pEmr->cbPixelFormat && pEmr->emr.nSize >= 108)
              ){
                verbose_printf("   szlMicrometers: {%d,%d} \n", pEmr->szlMicrometers.cx,pEmr->szlMicrometers.cy);
            }
        }

        // object table allocation
        // allocate one more to directly use object indexes (starts at 1 and not 0)
        states->objectTable = calloc(pEmr->nHandles + 1, sizeof(emfGraphObject));
        states->objectTableSize = pEmr->nHandles;

        // set scaling for original resolution
        if (states->scaling == 0)
            states->scaling = pEmr->szlDevice.cx / (pEmr->rclBounds.right  - pEmr->rclBounds.left);

        if (states->svgDelimiter){
            fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"); 
            fprintf(out, "<%ssvg version=\"1.1\" ", 
                    states->nameSpaceString);

            if ((states->nameSpace != NULL) && (strlen(states->nameSpace) != 0)){
                fprintf(out, "xmlns=\"http://www.w3.org/2000/svg\" ");
                fprintf(out, "xmlns:%s=\"http://www.w3.org/2000/svg\" ", states->nameSpace);
            }

            fprintf(out, "width=\"%f\" height=\"%f\">\n",
                    (pEmr->rclBounds.right  - pEmr->rclBounds.left) * states->scaling, 
                    (pEmr->rclBounds.bottom - pEmr->rclBounds.top)  * states->scaling);
        }

        fprintf(out, "<%sg>\n", states->nameSpaceString);
    }

    // U_EMRPOLYBEZIER                       2
    /**
      \brief Print a pointer to a U_EMR_POLYBEZIER record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYBEZIER_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core1_print("U_EMRPOLYBEZIER", contents, out, states);
    } 

    // U_EMRPOLYGON                          3
    /**
      \brief Print a pointer to a U_EMR_POLYGON record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYGON_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core1_print("U_EMRPOLYGON", contents, out, states);
    } 


    // U_EMRPOLYLINE              4
    /**
      \brief Print a pointer to a U_EMR_POLYLINE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYLINE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core1_print("U_EMRPOLYLINE", contents, out, states);
    } 

    // U_EMRPOLYBEZIERTO          5
    /**
      \brief Print a pointer to a U_EMR_POLYBEZIERTO record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYBEZIERTO_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core1_print("U_EMRPOLYBEZIERTO", contents, out, states);
    } 

    // U_EMRPOLYLINETO            6
    /**
      \brief Print a pointer to a U_EMR_POLYLINETO record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYLINETO_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core1_print("U_EMRPOLYLINETO", contents, out, states);
    } 

    // U_EMRPOLYPOLYLINE          7
    /**
      \brief Print a pointer to a U_EMR_POLYPOLYLINE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYPOLYLINE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core2_print("U_EMRPOLYPOLYLINE", contents, out, states);
    } 

    // U_EMRPOLYPOLYGON           8
    /**
      \brief Print a pointer to a U_EMR_POLYPOLYGON record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYPOLYGON_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core2_print("U_EMRPOLYPOLYGON", contents, out, states);
    } 

    // U_EMRSETWINDOWEXTEX        9
    /**
      \brief Print a pointer to a U_EMR_SETWINDOWEXTEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETWINDOWEXTEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core7_print("U_EMRSETWINDOWEXTEX", "szlExtent:","",contents, out, states);
    } 

    // U_EMRSETWINDOWORGEX       10
    /**
      \brief Print a pointer to a U_EMR_SETWINDOWORGEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETWINDOWORGEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core7_print("U_EMRSETWINDOWORGEX", "ptlOrigin:","",contents, out, states);
    } 

    // U_EMRSETVIEWPORTEXTEX     11
    /**
      \brief Print a pointer to a U_EMR_SETVIEWPORTEXTEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETVIEWPORTEXTEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core7_print("U_EMRSETVIEWPORTEXTEX", "szlExtent:","",contents, out, states);
    } 

    // U_EMRSETVIEWPORTORGEX     12
    /**
      \brief Print a pointer to a U_EMR_SETVIEWPORTORGEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETVIEWPORTORGEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core7_print("U_EMRSETVIEWPORTORGEX", "ptlOrigin:","",contents, out, states);
    } 

    // U_EMRSETBRUSHORGEX        13
    /**
      \brief Print a pointer to a U_EMR_SETBRUSHORGEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETBRUSHORGEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core7_print("U_EMRSETBRUSHORGEX", "ptlOrigin:","",contents, out, states);
    } 

    // U_EMREOF                  14
    /**
      \brief Print a pointer to a U_EMR_EOF record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREOF_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL
            PU_EMREOF pEmr = (PU_EMREOF)(contents);
        verbose_printf("   cbPalEntries:   %u\n",      pEmr->cbPalEntries );
        verbose_printf("   offPalEntries:  %u\n",      pEmr->offPalEntries);
        if(pEmr->cbPalEntries){
            verbose_printf("      PE:");
            logpalette_print(states, (PU_LOGPALETTE)(contents + pEmr->offPalEntries));
            verbose_printf("\n");
        }
        fprintf(out, "</%sg>\n", states->nameSpaceString);
        if(states->svgDelimiter)
            fprintf(out, "</%ssvg>\n", states->nameSpaceString);
    } 


    // U_EMRSETPIXELV            15
    /**
      \brief Print a pointer to a U_EMR_SETPIXELV record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETPIXELV_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRSETPIXELV pEmr = (PU_EMRSETPIXELV)(contents);
        verbose_printf("   ptlPixel:       ");  pointl_print(states,  pEmr->ptlPixel);  verbose_printf("\n");
        verbose_printf("   crColor:        ");  colorref_print(states, pEmr->crColor);   verbose_printf("\n");
    } 


    // U_EMRSETMAPPERFLAGS       16
    /**
      \brief Print a pointer to a U_EMR_SETMAPPERFLAGS record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETMAPPERFLAGS_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRSETMAPPERFLAGS pEmr = (PU_EMRSETMAPPERFLAGS)(contents);
        verbose_printf("   dwFlags:        0x%8.8X\n",pEmr->dwFlags);
    } 


    // U_EMRSETMAPMODE           17
    /**
      \brief Print a pointer to a U_EMR_SETMAPMODE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETMAPMODE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETMAPMODE", "iMode:", contents, out, states);
    }

    // U_EMRSETBKMODE            18
    /**
      \brief Print a pointer to a U_EMR_SETBKMODE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETBKMODE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETBKMODE", "iMode:", contents, out, states);
    }

    // U_EMRSETPOLYFILLMODE      19
    /**
      \brief Print a pointer to a U_EMR_SETPOLYFILLMODE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETPOLYFILLMODE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETPOLYFILLMODE", "iMode:", contents, out, states);
    }

    // U_EMRSETROP2              20
    /**
      \brief Print a pointer to a U_EMR_SETROP2 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETROP2_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETROP2", "dwRop:", contents, out, states);
    }

    // U_EMRSETSTRETCHBLTMODE    21
    /**
      \brief Print a pointer to a U_EMR_SETSTRETCHBLTMODE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETSTRETCHBLTMODE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETSTRETCHBLTMODE", "iMode:", contents, out, states);
    }

    // U_EMRSETTEXTALIGN         22
    /**
      \brief Print a pointer to a U_EMR_SETTEXTALIGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETTEXTALIGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETTEXTALIGN", "iMode:", contents, out, states);
    }

    // U_EMRSETCOLORADJUSTMENT   23
    /**
      \brief Print a pointer to a U_EMR_SETCOLORADJUSTMENT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETCOLORADJUSTMENT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRSETCOLORADJUSTMENT pEmr = (PU_EMRSETCOLORADJUSTMENT)(contents);
        verbose_printf("   ColorAdjustment:");
        coloradjustment_print(states, pEmr->ColorAdjustment);
        verbose_printf("\n");
    }

    // U_EMRSETTEXTCOLOR         24
    /**
      \brief Print a pointer to a U_EMR_SETTEXTCOLOR record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETTEXTCOLOR_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETTEXTCOLOR", "crColor:", contents, out, states);
    }

    // U_EMRSETBKCOLOR           25
    /**
      \brief Print a pointer to a U_EMR_SETBKCOLOR record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETBKCOLOR_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETBKCOLOR", "crColor:", contents, out, states);
    }

    // U_EMROFFSETCLIPRGN        26
    /**
      \brief Print a pointer to a U_EMR_OFFSETCLIPRGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMROFFSETCLIPRGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core7_print("U_EMROFFSETCLIPRGN", "ptl:","",contents, out, states);
    } 

    // U_EMRMOVETOEX             27
    /**
      \brief Print a pointer to a U_EMR_MOVETOEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRMOVETOEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            fprintf(out, "M ");
        moveto_draw("U_EMRMOVETOEX", "ptl:","",contents, out, states);
    } 

    // U_EMRSETMETARGN           28
    /**
      \brief Print a pointer to a U_EMR_SETMETARGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETMETARGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            UNUSED(contents);
    }

    // U_EMREXCLUDECLIPRECT      29
    /**
      \brief Print a pointer to a U_EMR_EXCLUDECLIPRECT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREXCLUDECLIPRECT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMREXCLUDECLIPRECT", contents, out, states);
    }

    // U_EMRINTERSECTCLIPRECT    30
    /**
      \brief Print a pointer to a U_EMR_INTERSECTCLIPRECT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRINTERSECTCLIPRECT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRINTERSECTCLIPRECT", contents, out, states);
    }

    // U_EMRSCALEVIEWPORTEXTEX   31
    /**
      \brief Print a pointer to a U_EMR_SCALEVIEWPORTEXTEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSCALEVIEWPORTEXTEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRSCALEVIEWPORTEXTEX", contents, out, states);
    }


    // U_EMRSCALEWINDOWEXTEX     32
    /**
      \brief Print a pointer to a U_EMR_SCALEWINDOWEXTEX record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSCALEWINDOWEXTEX_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRSCALEWINDOWEXTEX", contents, out, states);
    }

    // U_EMRSAVEDC               33
    /**
      \brief Print a pointer to a U_EMR_SAVEDC record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSAVEDC_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            saveDeviceContext(states);
        UNUSED(contents);
    }

    // U_EMRRESTOREDC            34
    /**
      \brief Print a pointer to a U_EMR_RESTOREDC record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRRESTOREDC_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents);
        restoreDeviceContext(states, pEmr->iMode);
        core3_print("U_EMRRESTOREDC", "iRelative:", contents, out, states);
    }

    // U_EMRSETWORLDTRANSFORM    35
    /**
      \brief Print a pointer to a U_EMR_SETWORLDTRANSFORM record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETWORLDTRANSFORM_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            PU_EMRSETWORLDTRANSFORM pEmr = (PU_EMRSETWORLDTRANSFORM)(contents);
        verbose_printf("   xform:");
        xform_print(states, pEmr->xform);
        verbose_printf("\n");
        states->currentDeviceContext.worldTransform = pEmr->xform;
    } 

    // U_EMRMODIFYWORLDTRANSFORM 36
    /**
      \brief Print a pointer to a U_EMR_MODIFYWORLDTRANSFORM record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRMODIFYWORLDTRANSFORM_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            PU_EMRMODIFYWORLDTRANSFORM pEmr = (PU_EMRMODIFYWORLDTRANSFORM)(contents);
        verbose_printf("   xform:");
        xform_print(states, pEmr->xform);
        verbose_printf("\n");

        switch (pEmr->iMode)
        {
            case U_MWT_IDENTITY:
                {

                    verbose_printf("   iMode:          U_MWT_IDENTITY\n");
                    setTransformIdentity(states);
                    break;
                }
            case U_MWT_LEFTMULTIPLY:
                {

                    verbose_printf("   iMode:          U_MWT_LEFTMULTIPLY\n");
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

                    break;
                }
            case U_MWT_RIGHTMULTIPLY:
                {
                    verbose_printf("   iMode:          U_MWT_RIGHTMULTIPLY\n");
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

                    break;
                }
            case U_MWT_SET:
                {
                    verbose_printf("   iMode:          U_MWT_SET\n");
                    states->currentDeviceContext.worldTransform = pEmr->xform;
                    break;
                }
            default:
                break;
        }
    } 

    // U_EMRSELECTOBJECT         37
    /**
      \brief Print a pointer to a U_EMR_SELECTOBJECT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSELECTOBJECT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL
        PU_EMRSELECTOBJECT pEmr = (PU_EMRSELECTOBJECT)(contents);
        uint32_t index = pEmr->ihObject;
        if(index & U_STOCK_OBJECT){
            verbose_printf("   StockObject:    0x%8.8X\n",  pEmr->ihObject );
            switch(index){
                case(U_WHITE_BRUSH):
                    break;
                case(U_LTGRAY_BRUSH):
                    break;
                case(U_GRAY_BRUSH):
                    break;
                case(U_DKGRAY_BRUSH):
                    break;
                case(U_BLACK_BRUSH):
                    break;
                case(U_NULL_BRUSH):
                    break;
                case(U_WHITE_PEN):
                    break;
                case(U_BLACK_PEN):
                    break;
                case(U_NULL_PEN):
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
            }
        }
        else {
            verbose_printf("   ihObject:       %u\n",     pEmr->ihObject );
            if(states->objectTable[index].fill_set){
                states->currentDeviceContext.fill_red  = states->objectTable[index].fill_red;
                states->currentDeviceContext.fill_blue = states->objectTable[index].fill_blue;
                states->currentDeviceContext.fill_green = states->objectTable[index].fill_green;
            }
            else if(states->objectTable[index].stroke_set){
                return;
            }
        }
    } 

    // U_EMRCREATEPEN            38
    /**
      \brief Print a pointer to a U_EMR_CREATEPEN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCREATEPEN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRCREATEPEN pEmr = (PU_EMRCREATEPEN)(contents);
        verbose_printf("   ihPen:          %u\n",      pEmr->ihPen );
        verbose_printf("   lopn:           ");    logpen_print(states, pEmr->lopn);  verbose_printf("\n");
    } 

    // U_EMRCREATEBRUSHINDIRECT  39
    /**
      \brief Print a pointer to a U_EMR_CREATEBRUSHINDIRECT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCREATEBRUSHINDIRECT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL
        PU_EMRCREATEBRUSHINDIRECT pEmr = (PU_EMRCREATEBRUSHINDIRECT)(contents);
        verbose_printf("   ihBrush:        %u\n",      pEmr->ihBrush );
        verbose_printf("   lb:             ");         logbrush_print(states, pEmr->lb);  verbose_printf("\n");

        uint16_t index = pEmr->ihBrush;
        if(pEmr->lb.lbStyle == U_BS_SOLID){
            states->objectTable[index].fill_red = pEmr->lb.lbColor.Red;
            states->objectTable[index].fill_green = pEmr->lb.lbColor.Green;
            states->objectTable[index].fill_blue = pEmr->lb.lbColor.Blue;
            states->objectTable[index].fill_mode    = U_BS_SOLID;
            states->objectTable[index].fill_set     = true;
        }
        else if(pEmr->lb.lbStyle == U_BS_HATCHED){
            //states->currentDeviceContext.fill_idx     = add_hatch(d, pEmr->lb.lbHatch, pEmr->lb.lbColor);
            states->objectTable[index].fill_recidx  = pEmr->ihBrush; // used if the hatch needs to be redone due to bkMode, textmode, etc. changes
            states->objectTable[index].fill_mode    = U_BS_HATCHED;
            states->objectTable[index].fill_set     = true;
        }

    } 

    // U_EMRDELETEOBJECT         40
    /**
      \brief Print a pointer to a U_EMR_DELETEOBJECT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRDELETEOBJECT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            PU_EMRDELETEOBJECT pEmr = (PU_EMRDELETEOBJECT)(contents);
        verbose_printf("   ihObject:       %u\n",      pEmr->ihObject );
        uint16_t index = pEmr->ihObject;
        states->objectTable[index] = (const emfGraphObject){ 0 };
    } 

    // U_EMRANGLEARC             41
    /**
      \brief Print a pointer to a U_EMR_ANGLEARC record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRANGLEARC_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRANGLEARC pEmr = (PU_EMRANGLEARC)(contents);
        verbose_printf("   ptlCenter:      "); pointl_print(states, pEmr->ptlCenter ); verbose_printf("\n");
        verbose_printf("   nRadius:        %u\n",      pEmr->nRadius );
        verbose_printf("   eStartAngle:    %f\n",       pEmr->eStartAngle );
        verbose_printf("   eSweepAngle:    %f\n",       pEmr->eSweepAngle );
    } 

    // U_EMRELLIPSE              42
    /**
      \brief Print a pointer to a U_EMR_ELLIPSE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRELLIPSE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRELLIPSE", contents, out, states);
    }

    // U_EMRRECTANGLE            43
    /**
      \brief Print a pointer to a U_EMR_RECTANGLE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRRECTANGLE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRRECTANGLE", contents, out, states);
    }

    // U_EMRROUNDRECT            44
    /**
      \brief Print a pointer to a U_EMR_ROUNDRECT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRROUNDRECT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRROUNDRECT pEmr = (PU_EMRROUNDRECT)(contents);
        verbose_printf("   rclBox:         "); rectl_print(states, pEmr->rclBox );     verbose_printf("\n");
        verbose_printf("   szlCorner:      "); sizel_print(states, pEmr->szlCorner );  verbose_printf("\n");
    }

    // U_EMRARC                  45
    /**
      \brief Print a pointer to a U_EMR_ARC record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRARC_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core9_print("U_EMRARC", contents, out, states);
    }

    // U_EMRCHORD                46
    /**
      \brief Print a pointer to a U_EMR_CHORD record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCHORD_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core9_print("U_EMRCHORD", contents, out, states);
    }

    // U_EMRPIE                  47
    /**
      \brief Print a pointer to a U_EMR_PIE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPIE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core9_print("U_EMRPIE", contents, out, states);
    }

    // U_EMRSELECTPALETTE        48
    /**
      \brief Print a pointer to a U_EMR_SELECTPALETTE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSELECTPALETTE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSELECTPALETTE", "ihPal:", contents, out, states);
    }

    // U_EMRCREATEPALETTE        49
    /**
      \brief Print a pointer to a U_EMR_CREATEPALETTE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCREATEPALETTE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRCREATEPALETTE pEmr = (PU_EMRCREATEPALETTE)(contents);
        verbose_printf("   ihPal:          %u\n",pEmr->ihPal);
        verbose_printf("   lgpl:           "); logpalette_print(states, (PU_LOGPALETTE)&(pEmr->lgpl) );  verbose_printf("\n");
    }

    // U_EMRSETPALETTEENTRIES    50
    /**
      \brief Print a pointer to a U_EMR_SETPALETTEENTRIES record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETPALETTEENTRIES_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            unsigned int i;
        PU_EMRSETPALETTEENTRIES pEmr = (PU_EMRSETPALETTEENTRIES)(contents);
        verbose_printf("   ihPal:          %u\n",pEmr->ihPal);
        verbose_printf("   iStart:         %u\n",pEmr->iStart);
        verbose_printf("   cEntries:       %u\n",pEmr->cEntries);
        if(pEmr->cEntries){
            verbose_printf("      PLTEntries:");
            PU_LOGPLTNTRY aPalEntries = (PU_LOGPLTNTRY) &(pEmr->aPalEntries);
            for(i=0; i<pEmr->cEntries; i++){
                verbose_printf("%d:",i); logpltntry_print(states, aPalEntries[i]);
            }
            verbose_printf("\n");
        }
    }

    // U_EMRRESIZEPALETTE        51
    /**
      \brief Print a pointer to a U_EMR_RESIZEPALETTE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRRESIZEPALETTE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core7_print("U_EMRRESIZEPALETTE", "ihPal:","cEntries",contents, out, states);
    } 

    // U_EMRREALIZEPALETTE       52
    /**
      \brief Print a pointer to a U_EMR_REALIZEPALETTE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRREALIZEPALETTE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            UNUSED(contents);
    }

    // U_EMREXTFLOODFILL         53
    /**
      \brief Print a pointer to a U_EMR_EXTFLOODFILL record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREXTFLOODFILL_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMREXTFLOODFILL pEmr = (PU_EMREXTFLOODFILL)(contents);
        verbose_printf("   ptlStart:       ");   pointl_print(states, pEmr->ptlStart);    verbose_printf("\n");
        verbose_printf("   crColor:        ");   colorref_print(states, pEmr->crColor);   verbose_printf("\n");
        verbose_printf("   iMode:          %u\n",pEmr->iMode);
    }

    // U_EMRLINETO               54
    /**
      \brief Print a pointer to a U_EMR_LINETO record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRLINETO_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            lineto_draw("U_EMRLINETO", "ptl:","",contents, out, states);
    } 

    // U_EMRARCTO                55
    /**
      \brief Print a pointer to a U_EMR_ARCTO record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRARCTO_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core9_print("U_EMRARCTO", contents, out, states);
    }

    // U_EMRPOLYDRAW             56
    /**
      \brief Print a pointer to a U_EMR_POLYDRAW record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYDRAW_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            unsigned int i;
        PU_EMRPOLYDRAW pEmr = (PU_EMRPOLYDRAW)(contents);
        verbose_printf("   rclBounds:      ");          rectl_print(states, pEmr->rclBounds);   verbose_printf("\n");
        verbose_printf("   cptl:           %d\n",pEmr->cptl        );
        verbose_printf("   Points:         ");
        for(i=0;i<pEmr->cptl; i++){
            verbose_printf(" [%d]:",i);
            pointl_print(states, pEmr->aptl[i]);
        }
        verbose_printf("\n");
        verbose_printf("   Types:          ");
        for(i=0;i<pEmr->cptl; i++){
            verbose_printf(" [%d]:%u ",i,pEmr->abTypes[i]);
        }
        verbose_printf("\n");
    }

    // U_EMRSETARCDIRECTION      57
    /**
      \brief Print a pointer to a U_EMR_SETARCDIRECTION record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETARCDIRECTION_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETARCDIRECTION","arcDirection:", contents, out, states);
    }

    // U_EMRSETMITERLIMIT        58
    /**
      \brief Print a pointer to a U_EMR_SETMITERLIMIT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETMITERLIMIT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETMITERLIMIT", "eMiterLimit:", contents, out, states);
    }


    // U_EMRBEGINPATH            59
    /**
      \brief Print a pointer to a U_EMR_BEGINPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRBEGINPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
        fprintf(out, "<%spath d=\"", states->nameSpaceString);
        states->inPath = 1;
        UNUSED(contents);
    }

    // U_EMRENDPATH              60
    /**
      \brief Print a pointer to a U_EMR_ENDPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRENDPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_PARTIAL
        fprintf(out, "\" fill=\"#%X%X%X\" stroke=\"blue\" stroke-width=\"1\" />\n", 
                states->currentDeviceContext.fill_red,
                states->currentDeviceContext.fill_green,
                states->currentDeviceContext.fill_blue
        );
        states->inPath = 0;
        UNUSED(contents);
    }

    // U_EMRCLOSEFIGURE          61
    /**
      \brief Print a pointer to a U_EMR_CLOSEFIGURE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCLOSEFIGURE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            fprintf(out, "Z ");
        UNUSED(contents);
    }

    // U_EMRFILLPATH             62
    /**
      \brief Print a pointer to a U_EMR_FILLPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRFILLPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRFILLPATH", contents, out, states);
    }

    // U_EMRSTROKEANDFILLPATH    63
    /**
      \brief Print a pointer to a U_EMR_STROKEANDFILLPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSTROKEANDFILLPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRSTROKEANDFILLPATH", contents, out, states);
    }

    // U_EMRSTROKEPATH           64
    /**
      \brief Print a pointer to a U_EMR_STROKEPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSTROKEPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core4_print("U_EMRSTROKEPATH", contents, out, states);
    }

    // U_EMRFLATTENPATH          65
    /**
      \brief Print a pointer to a U_EMR_FLATTENPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRFLATTENPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            UNUSED(contents);
    }

    // U_EMRWIDENPATH            66
    /**
      \brief Print a pointer to a U_EMR_WIDENPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRWIDENPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            UNUSED(contents);
    }

    // U_EMRSELECTCLIPPATH       67
    /**
      \brief Print a pointer to a U_EMR_SELECTCLIPPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSELECTCLIPPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSELECTCLIPPATH", "iMode:", contents, out, states);
    }

    // U_EMRABORTPATH            68
    /**
      \brief Print a pointer to a U_EMR_ABORTPATH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRABORTPATH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            UNUSED(contents);
    }

    // U_EMRUNDEF69                       69
#define U_EMRUNDEF69_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF69",A) //!< Not implemented.

    // U_EMRCOMMENT              70  Comment (any binary data, interpretation is program specific)
    /**
      \brief Print a pointer to a U_EMR_COMMENT record.
      \param contents   pointer to a location in memory holding the comment record
      \param blimit     size in bytes of the comment record
      \param off        offset in bytes to the first byte in this record

      EMF+ records, if any, are stored in EMF comment records.
      */
    void U_EMRCOMMENT_print(const char *contents, FILE *out, drawingStates *states, const char *blimit, size_t off){
        FLAG_IGNORED
            char *string;
        char *src;
        uint32_t cIdent,cIdent2,cbData;
        size_t loff;
        int    recsize;
        static int recnum=0;

        PU_EMRCOMMENT pEmr = (PU_EMRCOMMENT)(contents);


        /* There are several different types of comments */

        cbData = pEmr->cbData;
        verbose_printf("   cbData:         %d\n",cbData        );
        src = (char *)&(pEmr->Data);  // default
        if(cbData >= 4){
            /* Since the comment is just a big bag of bytes the emf endian code cannot safely touch
               any of its payload.  This is the only record type with that limitation.  Try to determine
               what the contents are even if more byte swapping is required. */
            cIdent = *(uint32_t *)(src);
            if(U_BYTE_SWAP){ U_swap4(&(cIdent),1); }
            if(     cIdent == U_EMR_COMMENT_PUBLIC       ){
                verbose_printf("   cIdent:  Public\n");
                PU_EMRCOMMENT_PUBLIC pEmrp = (PU_EMRCOMMENT_PUBLIC) pEmr;
                cIdent2 = pEmrp->pcIdent;
                if(U_BYTE_SWAP){ U_swap4(&(cIdent2),1); }
                verbose_printf("   pcIdent:        0x%8.8x\n",cIdent2);
                src = (char *)&(pEmrp->Data);
                cbData -= 8;
            }
            else if(cIdent == U_EMR_COMMENT_SPOOL        ){
                verbose_printf("   cIdent:  Spool\n");
                PU_EMRCOMMENT_SPOOL pEmrs = (PU_EMRCOMMENT_SPOOL) pEmr;
                cIdent2 = pEmrs->esrIdent;
                if(U_BYTE_SWAP){ U_swap4(&(cIdent2),1); }
                verbose_printf("   esrIdent:       0x%8.8x\n",cIdent2);
                src = (char *)&(pEmrs->Data);
                cbData -= 8;
            }
            else if(cIdent == U_EMR_COMMENT_EMFPLUSRECORD){
                verbose_printf("   cIdent:  EMF+\n");
                PU_EMRCOMMENT_EMFPLUS pEmrpl = (PU_EMRCOMMENT_EMFPLUS) pEmr;
                src = (char *)&(pEmrpl->Data);
                if (states->emfplus){
                    loff = 16;  /* Header size of the header part of an EMF+ comment record */
                    verbose_printf("\n   =====================%s START EMF+ RECORD ANALYSING %s=====================\n\n", KCYN, KNRM);
                    while(loff < cbData + 12){  // EMF+ records may not fill the entire comment, cbData value includes cIdent, but not U_EMR or cbData
                        recsize =  U_pmf_onerec_print(src, blimit, recnum, loff + off, out, states);
                        if(recsize<=0)break;
                        loff += recsize;
                        src  += recsize;
                        recnum++;
                    }
                    verbose_printf("\n   ======================%s END EMF+ RECORD ANALYSING %s======================\n", KBLU , KNRM);
                }
                //return;
            }
            else {
                verbose_printf("   cIdent:         not (Public or Spool or EMF+)\n");
            }
        }
        if(cbData){ // The data may not be printable, but try it just in case
            string = (char *)malloc(cbData + 1);
            (void)strncpy(string, src, cbData);
            string[cbData] = '\0'; // it might not be terminated - it might not even be text!
            //verbose_printf("   Data:           <%s>\n",string);
            free(string);
        }
    } 

    // U_EMRFILLRGN              71
    /**
      \brief Print a pointer to a U_EMR_FILLRGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRFILLRGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            int i,roff;
        PU_EMRFILLRGN pEmr = (PU_EMRFILLRGN)(contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   cbRgnData:      %u\n",pEmr->cbRgnData);
        verbose_printf("   ihBrush:        %u\n",pEmr->ihBrush);
        // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
        roff=0;
        i=1; 
        char *prd = (char *) &(pEmr->RgnData);
        while(roff  + sizeof(U_RGNDATAHEADER) < pEmr->emr.nSize){ // up to the end of the record
            verbose_printf("   RegionData[%d]: ",i);   rgndata_print(states, (PU_RGNDATA) (prd + roff));  verbose_printf("\n");
            roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
        }
    } 

    // U_EMRFRAMERGN             72
    /**
      \brief Print a pointer to a U_EMR_FRAMERGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRFRAMERGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            int i,roff;
        PU_EMRFRAMERGN pEmr = (PU_EMRFRAMERGN)(contents);
        verbose_printf("   rclBounds:      ");    rectl_print(states, pEmr->rclBounds);    verbose_printf("\n");
        verbose_printf("   cbRgnData:      %u\n",pEmr->cbRgnData);
        verbose_printf("   ihBrush:        %u\n",pEmr->ihBrush);
        verbose_printf("   szlStroke:      "); sizel_print(states, pEmr->szlStroke );      verbose_printf("\n");
        // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
        roff=0;
        i=1; 
        char *prd = (char *) &(pEmr->RgnData);
        while(roff  + sizeof(U_RGNDATAHEADER) < pEmr->emr.nSize){ // up to the end of the record
            verbose_printf("   RegionData[%d]: ",i);   rgndata_print(states, (PU_RGNDATA) (prd + roff));  verbose_printf("\n");
            roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
        }
    } 

    // U_EMRINVERTRGN            73
    /**
      \brief Print a pointer to a U_EMR_INVERTRGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRINVERTRGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core11_print("U_EMRINVERTRGN", contents, out, states);
    }

    // U_EMRPAINTRGN             74
    /**
      \brief Print a pointer to a U_EMR_PAINTRGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPAINTRGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core11_print("U_EMRPAINTRGN", contents, out, states);
    }

    // U_EMREXTSELECTCLIPRGN     75
    /**
      \brief Print a pointer to a U_EMR_EXTSELECTCLIPRGN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREXTSELECTCLIPRGN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            int i,roff;
        PU_EMREXTSELECTCLIPRGN pEmr = (PU_EMREXTSELECTCLIPRGN) (contents);
        verbose_printf("   cbRgnData:      %u\n",pEmr->cbRgnData);
        verbose_printf("   iMode:          %u\n",pEmr->iMode);
        // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
        char *prd = (char *) &(pEmr->RgnData);
        i=roff=0;
        while(roff + sizeof(U_RGNDATAHEADER) < pEmr->cbRgnData){ // stop at end of the record 4*4 = header + 4*4=rect
            verbose_printf("   RegionData[%d]: ",i++); rgndata_print(states, (PU_RGNDATA) (prd + roff)); verbose_printf("\n");
            roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
        }
    } 

    // U_EMRBITBLT               76
    /**
      \brief Print a pointer to a U_EMR_BITBLT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRBITBLT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRBITBLT pEmr = (PU_EMRBITBLT) (contents);
        verbose_printf("   rclBounds:      ");     rectl_print(states, pEmr->rclBounds);       verbose_printf("\n");
        verbose_printf("   Dest:           ");     pointl_print(states, pEmr->Dest);            verbose_printf("\n");
        verbose_printf("   cDest:          ");     pointl_print(states, pEmr->cDest);           verbose_printf("\n");
        verbose_printf("   dwRop :         0x%8.8X\n", pEmr->dwRop   );
        verbose_printf("   Src:            ");     pointl_print(states, pEmr->Src);             verbose_printf("\n");
        verbose_printf("   xformSrc:       ");     xform_print(states, pEmr->xformSrc);        verbose_printf("\n");
        verbose_printf("   crBkColorSrc:   ");     colorref_print(states, pEmr->crBkColorSrc); verbose_printf("\n");
        verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
        verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
        verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
        if(pEmr->cbBmiSrc){
            verbose_printf("      bitmap:      ");
            bitmapinfo_print(states, contents + pEmr->offBmiSrc);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
        verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
    }

    // U_EMRSTRETCHBLT           77
    /**
      \brief Print a pointer to a U_EMR_STRETCHBLT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSTRETCHBLT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRSTRETCHBLT pEmr = (PU_EMRSTRETCHBLT) (contents);
        verbose_printf("   rclBounds:      ");     rectl_print(states, pEmr->rclBounds);       verbose_printf("\n");
        verbose_printf("   Dest:           ");     pointl_print(states, pEmr->Dest);            verbose_printf("\n");
        verbose_printf("   cDest:          ");     pointl_print(states, pEmr->cDest);           verbose_printf("\n");
        verbose_printf("   dwRop :         0x%8.8X\n", pEmr->dwRop   );
        verbose_printf("   Src:            ");     pointl_print(states, pEmr->Src);             verbose_printf("\n");
        verbose_printf("   xformSrc:       ");     xform_print(states, pEmr->xformSrc);        verbose_printf("\n");
        verbose_printf("   crBkColorSrc:   ");     colorref_print(states, pEmr->crBkColorSrc); verbose_printf("\n");
        verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
        verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
        verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
        if(pEmr->cbBmiSrc){
            verbose_printf("      bitmap:      ");
            bitmapinfo_print(states, contents + pEmr->offBmiSrc);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
        verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
        verbose_printf("   cSrc:           ");     pointl_print(states, pEmr->cSrc);            verbose_printf("\n");
    }

    // U_EMRMASKBLT              78
    /**
      \brief Print a pointer to a U_EMR_MASKBLT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRMASKBLT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRMASKBLT pEmr = (PU_EMRMASKBLT) (contents);
        verbose_printf("   rclBounds:      ");     rectl_print(states, pEmr->rclBounds);       verbose_printf("\n");
        verbose_printf("   Dest:           ");     pointl_print(states, pEmr->Dest);            verbose_printf("\n");
        verbose_printf("   cDest:          ");     pointl_print(states, pEmr->cDest);           verbose_printf("\n");
        verbose_printf("   dwRop :         0x%8.8X\n",  pEmr->dwRop   );
        verbose_printf("   Src:            ");     pointl_print(states, pEmr->Src);             verbose_printf("\n");
        verbose_printf("   xformSrc:       ");     xform_print(states, pEmr->xformSrc);        verbose_printf("\n");
        verbose_printf("   crBkColorSrc:   ");     colorref_print(states, pEmr->crBkColorSrc); verbose_printf("\n");
        verbose_printf("   iUsageSrc:      %u\n",  pEmr->iUsageSrc   );
        verbose_printf("   offBmiSrc:      %u\n",  pEmr->offBmiSrc   );
        verbose_printf("   cbBmiSrc:       %u\n",  pEmr->cbBmiSrc    );
        if(pEmr->cbBmiSrc){
            verbose_printf("      Src bitmap:  ");
            bitmapinfo_print(states, contents + pEmr->offBmiSrc);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsSrc:     %u\n",  pEmr->offBitsSrc   );
        verbose_printf("   cbBitsSrc:      %u\n",  pEmr->cbBitsSrc    );
        verbose_printf("   Mask:           ");     pointl_print(states, pEmr->Mask);            verbose_printf("\n");
        verbose_printf("   iUsageMask:     %u\n",  pEmr->iUsageMask   );
        verbose_printf("   offBmiMask:     %u\n",  pEmr->offBmiMask   );
        verbose_printf("   cbBmiMask:      %u\n",  pEmr->cbBmiMask    );
        if(pEmr->cbBmiMask){
            verbose_printf("      Mask bitmap: ");
            bitmapinfo_print(states, contents + pEmr->offBmiMask);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsMask:    %u\n",  pEmr->offBitsMask   );
        verbose_printf("   cbBitsMask:     %u\n",  pEmr->cbBitsMask    );
    }

    // U_EMRPLGBLT               79
    /**
      \brief Print a pointer to a U_EMR_PLGBLT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPLGBLT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRPLGBLT pEmr = (PU_EMRPLGBLT) (contents);
        verbose_printf("   rclBounds:      ");     rectl_print(states, pEmr->rclBounds);       verbose_printf("\n");
        verbose_printf("   aptlDst(UL):    ");     pointl_print(states, pEmr->aptlDst[0]);      verbose_printf("\n");
        verbose_printf("   aptlDst(UR):    ");     pointl_print(states, pEmr->aptlDst[1]);      verbose_printf("\n");
        verbose_printf("   aptlDst(LL):    ");     pointl_print(states, pEmr->aptlDst[2]);      verbose_printf("\n");
        verbose_printf("   Src:            ");     pointl_print(states, pEmr->Src);             verbose_printf("\n");
        verbose_printf("   cSrc:           ");     pointl_print(states, pEmr->cSrc);            verbose_printf("\n");
        verbose_printf("   xformSrc:       ");     xform_print(states, pEmr->xformSrc);        verbose_printf("\n");
        verbose_printf("   crBkColorSrc:   ");     colorref_print(states, pEmr->crBkColorSrc); verbose_printf("\n");
        verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
        verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
        verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
        if(pEmr->cbBmiSrc){
            verbose_printf("      Src bitmap:  ");
            bitmapinfo_print(states, contents + pEmr->offBmiSrc);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
        verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
        verbose_printf("   Mask:           ");    pointl_print(states, pEmr->Mask);            verbose_printf("\n");
        verbose_printf("   iUsageMsk:      %u\n", pEmr->iUsageMask   );
        verbose_printf("   offBmiMask:     %u\n", pEmr->offBmiMask   );
        verbose_printf("   cbBmiMask:      %u\n", pEmr->cbBmiMask    );
        if(pEmr->cbBmiMask){
            verbose_printf("      Mask bitmap: ");
            bitmapinfo_print(states, contents + pEmr->offBmiMask);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsMask:    %u\n", pEmr->offBitsMask   );
        verbose_printf("   cbBitsMask:     %u\n", pEmr->cbBitsMask    );
    }

    // U_EMRSETDIBITSTODEVICE    80
    /**
      \brief Print a pointer to a U_EMRSETDIBITSTODEVICE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETDIBITSTODEVICE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRSETDIBITSTODEVICE pEmr = (PU_EMRSETDIBITSTODEVICE) (contents);
        verbose_printf("   rclBounds:      ");     rectl_print(states, pEmr->rclBounds);       verbose_printf("\n");
        verbose_printf("   Dest:           ");     pointl_print(states, pEmr->Dest);            verbose_printf("\n");
        verbose_printf("   Src:            ");     pointl_print(states, pEmr->Src);             verbose_printf("\n");
        verbose_printf("   cSrc:           ");     pointl_print(states, pEmr->cSrc);            verbose_printf("\n");
        verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
        verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
        if(pEmr->cbBmiSrc){
            verbose_printf("      Src bitmap:  ");
            bitmapinfo_print(states, contents + pEmr->offBmiSrc);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
        verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
        verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
        verbose_printf("   iStartScan:     %u\n", pEmr->iStartScan    );
        verbose_printf("   cScans :        %u\n", pEmr->cScans        );
    }

    // U_EMRSTRETCHDIBITS        81
    /**
      \brief Print a pointer to a U_EMR_STRETCHDIBITS record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSTRETCHDIBITS_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRSTRETCHDIBITS pEmr = (PU_EMRSTRETCHDIBITS) (contents);
        verbose_printf("   rclBounds:      ");     rectl_print(states, pEmr->rclBounds);       verbose_printf("\n");
        verbose_printf("   Dest:           ");     pointl_print(states, pEmr->Dest);            verbose_printf("\n");
        verbose_printf("   Src:            ");     pointl_print(states, pEmr->Src);             verbose_printf("\n");
        verbose_printf("   cSrc:           ");     pointl_print(states, pEmr->cSrc);            verbose_printf("\n");
        verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
        verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
        if(pEmr->cbBmiSrc){
            verbose_printf("      Src bitmap:  ");
            bitmapinfo_print(states, contents + pEmr->offBmiSrc);
            verbose_printf("\n");
        }
        verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
        verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
        verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
        verbose_printf("   dwRop :         0x%8.8X\n", pEmr->dwRop   );
        verbose_printf("   cDest:          ");     pointl_print(states, pEmr->cDest);           verbose_printf("\n");
    }

    // U_EMREXTCREATEFONTINDIRECTW_print    82
    /**
      \brief Print a pointer to a U_EMR_EXTCREATEFONTINDIRECTW record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREXTCREATEFONTINDIRECTW_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMREXTCREATEFONTINDIRECTW pEmr = (PU_EMREXTCREATEFONTINDIRECTW) (contents);
        verbose_printf("   ihFont:         %u\n",pEmr->ihFont );
        verbose_printf("   Font:           ");
        if(pEmr->emr.nSize == sizeof(U_EMREXTCREATEFONTINDIRECTW)){ // holds logfont_panose
            logfont_panose_print(states, pEmr->elfw);
        }
        else { // holds logfont
            logfont_print(states, *(PU_LOGFONT) &(pEmr->elfw));
        }
        verbose_printf("\n");
    }

    // U_EMREXTTEXTOUTA          83
    /**
      \brief Print a pointer to a U_EMR_EXTTEXTOUTA record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREXTTEXTOUTA_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core8_print("U_EMREXTTEXTOUTA", contents, out, states, 0);
    }

    // U_EMREXTTEXTOUTW          84
    /**
      \brief Print a pointer to a U_EMR_EXTTEXTOUTW record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREXTTEXTOUTW_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core8_print("U_EMREXTTEXTOUTW", contents, out, states, 1);
    }

    // U_EMRPOLYBEZIER16         85
    /**
      \brief Print a pointer to a U_EMR_POLYBEZIER16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYBEZIER16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core6_print("U_EMRPOLYBEZIER16", contents, out, states);
    }

    // U_EMRPOLYGON16            86
    /**
      \brief Print a pointer to a U_EMR_POLYGON16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYGON16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core6_print("U_EMRPOLYGON16", contents, out, states);
    }

    // U_EMRPOLYLINE16           87
    /**
      \brief Print a pointer to a U_EMR_POLYLINE16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYLINE16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core6_print("U_EMRPOLYLINE16", contents, out, states);
    }

    // U_EMRPOLYBEZIERTO16       88
    /**
      \brief Print a pointer to a U_EMR_POLYBEZIERTO16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYBEZIERTO16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            cubic_bezier_draw("U_EMRPOLYBEZIERTO16", contents, out, states);
    }

    // U_EMRPOLYLINETO16         89
    /**
      \brief Print a pointer to a U_EMR_POLYLINETO16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYLINETO16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_SUPPORTED
            polyline_draw("U_EMRPOLYLINETO16", contents, out, states);
    }

    // U_EMRPOLYPOLYLINE16       90
    /**
      \brief Print a pointer to a U_EMR_POLYPOLYLINE16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYPOLYLINE16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core10_print("U_EMRPOLYPOLYLINE16", contents, out, states);
    }

    // U_EMRPOLYPOLYGON16        91
    /**
      \brief Print a pointer to a U_EMR_POLYPOLYGON16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYPOLYGON16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core10_print("U_EMRPOLYPOLYGON16", contents, out, states);
    }


    // U_EMRPOLYDRAW16           92
    /**
      \brief Print a pointer to a U_EMR_POLYDRAW16 record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPOLYDRAW16_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            unsigned int i;
        PU_EMRPOLYDRAW16 pEmr = (PU_EMRPOLYDRAW16)(contents);
        verbose_printf("   rclBounds:      ");          rectl_print(states, pEmr->rclBounds);   verbose_printf("\n");
        verbose_printf("   cpts:           %d\n",pEmr->cpts        );
        verbose_printf("   Points:         ");
        for(i=0;i<pEmr->cpts; i++){
            verbose_printf(" [%d]:",i);
            point16_print(states, pEmr->apts[i], out);
        }
        verbose_printf("\n");
        verbose_printf("   Types:          ");
        for(i=0;i<pEmr->cpts; i++){
            verbose_printf(" [%d]:%u ",i,pEmr->abTypes[i]);
        }
        verbose_printf("\n");
    }

    // U_EMRCREATEMONOBRUSH      93
    /**
      \brief Print a pointer to a U_EMR_CREATEMONOBRUSH record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCREATEMONOBRUSH_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core12_print("U_EMRCREATEMONOBRUSH", contents, out, states);
    }

    // U_EMRCREATEDIBPATTERNBRUSHPT_print   94
    /**
      \brief Print a pointer to a U_EMR_CREATEDIBPATTERNBRUSHPT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCREATEDIBPATTERNBRUSHPT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core12_print("U_EMRCREATEDIBPATTERNBRUSHPT", contents, out, states);
        FLAG_IGNORED
    }


    // U_EMREXTCREATEPEN         95
    /**
      \brief Print a pointer to a U_EMR_EXTCREATEPEN record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMREXTCREATEPEN_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMREXTCREATEPEN pEmr = (PU_EMREXTCREATEPEN)(contents);
        verbose_printf("   ihPen:          %u\n", pEmr->ihPen );
        verbose_printf("   offBmi:         %u\n", pEmr->offBmi   );
        verbose_printf("   cbBmi:          %u\n", pEmr->cbBmi    );
        if(pEmr->cbBmi){
            verbose_printf("      bitmap:      ");
            bitmapinfo_print(states, contents + pEmr->offBmi);
            verbose_printf("\n");
        }
        verbose_printf("   offBits:        %u\n", pEmr->offBits   );
        verbose_printf("   cbBits:         %u\n", pEmr->cbBits    );
        verbose_printf("   elp:            ");     extlogpen_print(states, (PU_EXTLOGPEN) &(pEmr->elp));  verbose_printf("\n");
    } 

    // U_EMRPOLYTEXTOUTA         96 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTA_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRPOLYTEXTOUTA",A) //!< Not implemented.
    // U_EMRPOLYTEXTOUTW         97 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTW_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRPOLYTEXTOUTW",A) //!< Not implemented.

    // U_EMRSETICMMODE           98
    /**
      \brief Print a pointer to a U_EMR_SETICMMODE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETICMMODE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_UNUSED
            core3_print("U_EMRSETICMMODE", "iMode:", contents, out, states);
    }

    // U_EMRCREATECOLORSPACE     99
    /**
      \brief Print a pointer to a U_EMR_CREATECOLORSPACE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCREATECOLORSPACE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRCREATECOLORSPACE pEmr = (PU_EMRCREATECOLORSPACE)(contents);
        verbose_printf("   ihCS:           %u\n", pEmr->ihCS    );
        verbose_printf("   ColorSpace:     "); logcolorspacea_print(states, pEmr->lcs);  verbose_printf("\n");
    }

    // U_EMRSETCOLORSPACE       100
    /**
      \brief Print a pointer to a U_EMR_SETCOLORSPACE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETCOLORSPACE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETCOLORSPACE", "ihCS:", contents, out, states);
    }

    // U_EMRDELETECOLORSPACE    101
    /**
      \brief Print a pointer to a U_EMR_DELETECOLORSPACE record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRDELETECOLORSPACE_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRDELETECOLORSPACE", "ihCS:", contents, out, states);
    }

    // U_EMRGLSRECORD           102  Not implemented
#define U_EMRGLSRECORD_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRGLSRECORD",A) //!< Not implemented.
    // U_EMRGLSBOUNDEDRECORD    103  Not implemented
#define U_EMRGLSBOUNDEDRECORD_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRGLSBOUNDEDRECORD",A) //!< Not implemented.

    // U_EMRPIXELFORMAT         104
    /**
      \brief Print a pointer to a U_EMR_PIXELFORMAT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRPIXELFORMAT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            PU_EMRPIXELFORMAT pEmr = (PU_EMRPIXELFORMAT)(contents);
        verbose_printf("   Pfd:            ");  pixelformatdescriptor_print(states, pEmr->pfd);  verbose_printf("\n");
    }

    // U_EMRDRAWESCAPE          105  Not implemented
#define U_EMRDRAWESCAPE_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRDRAWESCAPE",A) //!< Not implemented.
    // U_EMREXTESCAPE           106  Not implemented
#define U_EMREXTESCAPE_print(A) U_EMRNOTIMPLEMENTED_print("U_EMREXTESCAPE",A) //!< Not implemented.
    // U_EMRUNDEF107            107  Not implemented
#define U_EMRUNDEF107_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF107",A) //!< Not implemented.

    // U_EMRSMALLTEXTOUT        108
    /**
      \brief Print a pointer to a U_EMR_SMALLTEXTOUT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSMALLTEXTOUT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            int roff;
        char *string;
        PU_EMRSMALLTEXTOUT pEmr = (PU_EMRSMALLTEXTOUT)(contents);
        verbose_printf("   Dest:           ");         pointl_print(states, pEmr->Dest);            verbose_printf("\n");
        verbose_printf("   cChars:         %u\n",      pEmr->cChars          );
        verbose_printf("   fuOptions:      0x%8.8X\n", pEmr->fuOptions       );
        verbose_printf("   iGraphicsMode:  0x%8.8X\n", pEmr->iGraphicsMode   );
        verbose_printf("   exScale:        %f\n",      pEmr->exScale         );
        verbose_printf("   eyScale:        %f\n",      pEmr->eyScale         );
        roff = sizeof(U_EMRSMALLTEXTOUT);  //offset to the start of the variable fields
        if(!(pEmr->fuOptions & U_ETO_NO_RECT)){
            verbose_printf("   rclBounds:      ");      rectl_print(states, *(PU_RECTL) (contents + roff));       verbose_printf("\n");
            roff += sizeof(U_RECTL);
        }
        if(pEmr->fuOptions & U_ETO_SMALL_CHARS){
            verbose_printf("   Text8:          <%.*s>\n",pEmr->cChars,contents+roff);  /* May not be null terminated */
        }
        else {
            string = U_Utf16leToUtf8((uint16_t *)(contents+roff), pEmr->cChars, NULL);
            verbose_printf("   Text16:         <%s>\n",contents+roff);
            free(string);
        }
    }

    // U_EMRFORCEUFIMAPPING     109  Not implemented
#define U_EMRFORCEUFIMAPPING_print(A)     U_EMRNOTIMPLEMENTED_print("U_EMRFORCEUFIMAPPING",A) //!< Not implemented.
    // U_EMRNAMEDESCAPE         110  Not implemented
#define U_EMRNAMEDESCAPE_print(A)         U_EMRNOTIMPLEMENTED_print("U_EMRNAMEDESCAPE",A) //!< Not implemented.
    // U_EMRCOLORCORRECTPALETTE 111  Not implemented
#define U_EMRCOLORCORRECTPALETTE_print(A) U_EMRNOTIMPLEMENTED_print("U_EMRCOLORCORRECTPALETTE",A) //!< Not implemented.
    // U_EMRSETICMPROFILEA      112  Not implemented
#define U_EMRSETICMPROFILEA_print(A)      U_EMRNOTIMPLEMENTED_print("U_EMRSETICMPROFILEA",A) //!< Not implemented.
    // U_EMRSETICMPROFILEW      113  Not implemented
#define U_EMRSETICMPROFILEW_print(A)      U_EMRNOTIMPLEMENTED_print("U_EMRSETICMPROFILEW",A) //!< Not implemented.

    // U_EMRALPHABLEND          114
    /**
      \brief Print a pointer to a U_EMR_ALPHABLEND record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRALPHABLEND_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core13_print("U_EMRALPHABLEND", contents, out, states);
    }

    // U_EMRSETLAYOUT           115
    /**
      \brief Print a pointer to a U_EMR_SETLAYOUT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRSETLAYOUT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core3_print("U_EMRSETLAYOUT", "iMode:", contents, out, states);
    }

    // U_EMRTRANSPARENTBLT      116
    /**
      \brief Print a pointer to a U_EMR_TRANSPARENTBLT record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRTRANSPARENTBLT_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            core13_print("U_EMRTRANSPARENTBLT", contents, out, states);
    }

    // U_EMRUNDEF117            117  Not implemented
#define U_EMRUNDEF117_print(A)    U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF117",A) //!< Not implemented.
    // U_EMRGRADIENTFILL        118
    /**
      \brief Print a pointer to a U_EMR_GRADIENTFILL record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRGRADIENTFILL_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            unsigned int i;
        PU_EMRGRADIENTFILL pEmr = (PU_EMRGRADIENTFILL)(contents);
        verbose_printf("   rclBounds:      ");      rectl_print(states, pEmr->rclBounds);   verbose_printf("\n");
        verbose_printf("   nTriVert:       %u\n",   pEmr->nTriVert   );
        verbose_printf("   nGradObj:       %u\n",   pEmr->nGradObj   );
        verbose_printf("   ulMode:         %u\n",   pEmr->ulMode     );
        contents += sizeof(U_EMRGRADIENTFILL);
        if(pEmr->nTriVert){
            verbose_printf("   TriVert:        ");
            for(i=0; i<pEmr->nTriVert; i++, contents+=sizeof(U_TRIVERTEX)){
                trivertex_print(states, *(PU_TRIVERTEX)(contents));
            }
            verbose_printf("\n");
        }
        if(pEmr->nGradObj){
            verbose_printf("   GradObj:        ");
            if(     pEmr->ulMode == U_GRADIENT_FILL_TRIANGLE){
                for(i=0; i<pEmr->nGradObj; i++, contents+=sizeof(U_GRADIENT3)){
                    gradient3_print(states, *(PU_GRADIENT3)(contents));
                }
            }
            else if(pEmr->ulMode == U_GRADIENT_FILL_RECT_H || 
                    pEmr->ulMode == U_GRADIENT_FILL_RECT_V){
                for(i=0; i<pEmr->nGradObj; i++, contents+=sizeof(U_GRADIENT4)){
                    gradient4_print(states, *(PU_GRADIENT4)(contents));
                }
            }
            else { verbose_printf("invalid ulMode value!"); }
            verbose_printf("\n");
        }
    }

    // U_EMRSETLINKEDUFIS       119  Not implemented
#define U_EMRSETLINKEDUFIS_print(A)        U_EMRNOTIMPLEMENTED_print("U_EMR_SETLINKEDUFIS",A) //!< Not implemented.
    // U_EMRSETTEXTJUSTIFICATION120  Not implemented (denigrated)
#define U_EMRSETTEXTJUSTIFICATION_print(A) U_EMRNOTIMPLEMENTED_print("U_EMR_SETTEXTJUSTIFICATION",A) //!< Not implemented.
    // U_EMRCOLORMATCHTOTARGETW 121  Not implemented  
#define U_EMRCOLORMATCHTOTARGETW_print(A)  U_EMRNOTIMPLEMENTED_print("U_EMR_COLORMATCHTOTARGETW",A) //!< Not implemented.

    // U_EMRCREATECOLORSPACEW   122
    /**
      \brief Print a pointer to a U_EMR_CREATECOLORSPACEW record.
      \param contents   pointer to a buffer holding all EMR records
      */
    void U_EMRCREATECOLORSPACEW_print(const char *contents, FILE *out, drawingStates *states){
        FLAG_IGNORED
            unsigned int i;
        PU_EMRCREATECOLORSPACEW pEmr = (PU_EMRCREATECOLORSPACEW)(contents);
        verbose_printf("   ihCS:           %u\n", pEmr->ihCS     );
        verbose_printf("   ColorSpace:     "); logcolorspacew_print(states, pEmr->lcs);  verbose_printf("\n");
        verbose_printf("   dwFlags:        0x%8.8X\n", pEmr->dwFlags  );
        verbose_printf("   cbData:         %u\n", pEmr->cbData   );
        verbose_printf("   Data(hexvalues):");
        if(pEmr->dwFlags & 1){
            for(i=0; i<pEmr->cbData; i++){
                verbose_printf("[%d]:%2.2X ",i,pEmr->Data[i]);
            }
        }
        verbose_printf("\n");
    }

    /**
      \brief Print any record in an emf
      \returns record length for a normal record, 0 for EMREOF, -1 for a bad record
      \param contents   pointer to a buffer holding all EMR records
      \param blimit     pointer to the byte after the last byte in the buffer holding all EMR records
      \param recnum     number of this record in contents
      \param off        offset to this record in contents
      */
    int U_emf_onerec_print(const char *contents, const char *blimit, int recnum, size_t off, FILE *out, drawingStates *states){
        PU_ENHMETARECORD  lpEMFR  = (PU_ENHMETARECORD)(contents + off);
        unsigned int size;
        FLAG_RESET
            verbose_printf("\n%-30srecord:%5d type:%-4d offset:%8d rsize:%8d\n",U_emr_names(lpEMFR->iType),recnum,lpEMFR->iType,(int) off,lpEMFR->nSize);
        //verbose_printf("\n%-30srecord:%5d type:%-4d offset:%8d rsize:%8d\n",U_emr_names(lpEMFR->iType),recnum,lpEMFR->iType,(int) off,lpEMFR->nSize);
        size      = lpEMFR->nSize;
        contents += off;

        /* Check that the record size is OK, abort if not.
           Pointer math might wrap, so check both sides of the range */
        if(size < sizeof(U_EMR)           ||
                contents + size - 1 >= blimit  || 
                contents + size - 1 < contents)return(-1);

        switch (lpEMFR->iType)
        {
            case U_EMR_HEADER:                  U_EMRHEADER_print(contents, out, states);                  break;
            case U_EMR_POLYBEZIER:              U_EMRPOLYBEZIER_print(contents, out, states);              break;
            case U_EMR_POLYGON:                 U_EMRPOLYGON_print(contents, out, states);                 break;
            case U_EMR_POLYLINE:                U_EMRPOLYLINE_print(contents, out, states);                break;
            case U_EMR_POLYBEZIERTO:            U_EMRPOLYBEZIERTO_print(contents, out, states);            break;
            case U_EMR_POLYLINETO:              U_EMRPOLYLINETO_print(contents, out, states);              break;
            case U_EMR_POLYPOLYLINE:            U_EMRPOLYPOLYLINE_print(contents, out, states);            break;
            case U_EMR_POLYPOLYGON:             U_EMRPOLYPOLYGON_print(contents, out, states);             break;
            case U_EMR_SETWINDOWEXTEX:          U_EMRSETWINDOWEXTEX_print(contents, out, states);          break;
            case U_EMR_SETWINDOWORGEX:          U_EMRSETWINDOWORGEX_print(contents, out, states);          break;
            case U_EMR_SETVIEWPORTEXTEX:        U_EMRSETVIEWPORTEXTEX_print(contents, out, states);        break;
            case U_EMR_SETVIEWPORTORGEX:        U_EMRSETVIEWPORTORGEX_print(contents, out, states);        break;
            case U_EMR_SETBRUSHORGEX:           U_EMRSETBRUSHORGEX_print(contents, out, states);           break;
            case U_EMR_EOF:                     U_EMREOF_print(contents, out, states);          size=0;    break;
            case U_EMR_SETPIXELV:               U_EMRSETPIXELV_print(contents, out, states);               break;
            case U_EMR_SETMAPPERFLAGS:          U_EMRSETMAPPERFLAGS_print(contents, out, states);          break;
            case U_EMR_SETMAPMODE:              U_EMRSETMAPMODE_print(contents, out, states);              break;
            case U_EMR_SETBKMODE:               U_EMRSETBKMODE_print(contents, out, states);               break;
            case U_EMR_SETPOLYFILLMODE:         U_EMRSETPOLYFILLMODE_print(contents, out, states);         break;
            case U_EMR_SETROP2:                 U_EMRSETROP2_print(contents, out, states);                 break;
            case U_EMR_SETSTRETCHBLTMODE:       U_EMRSETSTRETCHBLTMODE_print(contents, out, states);       break;
            case U_EMR_SETTEXTALIGN:            U_EMRSETTEXTALIGN_print(contents, out, states);            break;
            case U_EMR_SETCOLORADJUSTMENT:      U_EMRSETCOLORADJUSTMENT_print(contents, out, states);      break;
            case U_EMR_SETTEXTCOLOR:            U_EMRSETTEXTCOLOR_print(contents, out, states);            break;
            case U_EMR_SETBKCOLOR:              U_EMRSETBKCOLOR_print(contents, out, states);              break;
            case U_EMR_OFFSETCLIPRGN:           U_EMROFFSETCLIPRGN_print(contents, out, states);           break;
            case U_EMR_MOVETOEX:                U_EMRMOVETOEX_print(contents, out, states);                break;
            case U_EMR_SETMETARGN:              U_EMRSETMETARGN_print(contents, out, states);              break;
            case U_EMR_EXCLUDECLIPRECT:         U_EMREXCLUDECLIPRECT_print(contents, out, states);         break;
            case U_EMR_INTERSECTCLIPRECT:       U_EMRINTERSECTCLIPRECT_print(contents, out, states);       break;
            case U_EMR_SCALEVIEWPORTEXTEX:      U_EMRSCALEVIEWPORTEXTEX_print(contents, out, states);      break;
            case U_EMR_SCALEWINDOWEXTEX:        U_EMRSCALEWINDOWEXTEX_print(contents, out, states);        break;
            case U_EMR_SAVEDC:                  U_EMRSAVEDC_print(contents, out, states);                  break;
            case U_EMR_RESTOREDC:               U_EMRRESTOREDC_print(contents, out, states);               break;
            case U_EMR_SETWORLDTRANSFORM:       U_EMRSETWORLDTRANSFORM_print(contents, out, states);       break;
            case U_EMR_MODIFYWORLDTRANSFORM:    U_EMRMODIFYWORLDTRANSFORM_print(contents, out, states);    break;
            case U_EMR_SELECTOBJECT:            U_EMRSELECTOBJECT_print(contents, out, states);            break;
            case U_EMR_CREATEPEN:               U_EMRCREATEPEN_print(contents, out, states);               break;
            case U_EMR_CREATEBRUSHINDIRECT:     U_EMRCREATEBRUSHINDIRECT_print(contents, out, states);     break;
            case U_EMR_DELETEOBJECT:            U_EMRDELETEOBJECT_print(contents, out, states);            break;
            case U_EMR_ANGLEARC:                U_EMRANGLEARC_print(contents, out, states);                break;
            case U_EMR_ELLIPSE:                 U_EMRELLIPSE_print(contents, out, states);                 break;
            case U_EMR_RECTANGLE:               U_EMRRECTANGLE_print(contents, out, states);               break;
            case U_EMR_ROUNDRECT:               U_EMRROUNDRECT_print(contents, out, states);               break;
            case U_EMR_ARC:                     U_EMRARC_print(contents, out, states);                     break;
            case U_EMR_CHORD:                   U_EMRCHORD_print(contents, out, states);                   break;
            case U_EMR_PIE:                     U_EMRPIE_print(contents, out, states);                     break;
            case U_EMR_SELECTPALETTE:           U_EMRSELECTPALETTE_print(contents, out, states);           break;
            case U_EMR_CREATEPALETTE:           U_EMRCREATEPALETTE_print(contents, out, states);           break;
            case U_EMR_SETPALETTEENTRIES:       U_EMRSETPALETTEENTRIES_print(contents, out, states);       break;
            case U_EMR_RESIZEPALETTE:           U_EMRRESIZEPALETTE_print(contents, out, states);           break;
            case U_EMR_REALIZEPALETTE:          U_EMRREALIZEPALETTE_print(contents, out, states);          break;
            case U_EMR_EXTFLOODFILL:            U_EMREXTFLOODFILL_print(contents, out, states);            break;
            case U_EMR_LINETO:                  U_EMRLINETO_print(contents, out, states);                  break;
            case U_EMR_ARCTO:                   U_EMRARCTO_print(contents, out, states);                   break;
            case U_EMR_POLYDRAW:                U_EMRPOLYDRAW_print(contents, out, states);                break;
            case U_EMR_SETARCDIRECTION:         U_EMRSETARCDIRECTION_print(contents, out, states);         break;
            case U_EMR_SETMITERLIMIT:           U_EMRSETMITERLIMIT_print(contents, out, states);           break;
            case U_EMR_BEGINPATH:               U_EMRBEGINPATH_print(contents, out, states);               break;
            case U_EMR_ENDPATH:                 U_EMRENDPATH_print(contents, out, states);                 break;
            case U_EMR_CLOSEFIGURE:             U_EMRCLOSEFIGURE_print(contents, out, states);             break;
            case U_EMR_FILLPATH:                U_EMRFILLPATH_print(contents, out, states);                break;
            case U_EMR_STROKEANDFILLPATH:       U_EMRSTROKEANDFILLPATH_print(contents, out, states);       break;
            case U_EMR_STROKEPATH:              U_EMRSTROKEPATH_print(contents, out, states);              break;
            case U_EMR_FLATTENPATH:             U_EMRFLATTENPATH_print(contents, out, states);             break;
            case U_EMR_WIDENPATH:               U_EMRWIDENPATH_print(contents, out, states);               break;
            case U_EMR_SELECTCLIPPATH:          U_EMRSELECTCLIPPATH_print(contents, out, states);          break;
            case U_EMR_ABORTPATH:               U_EMRABORTPATH_print(contents, out, states);               break;
                                                //        case U_EMR_UNDEF69:                 U_EMRUNDEF69_print(contents, out, states);                 break;
            case U_EMR_COMMENT:                 U_EMRCOMMENT_print(contents, out, states, blimit, off);    break;
            case U_EMR_FILLRGN:                 U_EMRFILLRGN_print(contents, out, states);                 break;
            case U_EMR_FRAMERGN:                U_EMRFRAMERGN_print(contents, out, states);                break;
            case U_EMR_INVERTRGN:               U_EMRINVERTRGN_print(contents, out, states);               break;
            case U_EMR_PAINTRGN:                U_EMRPAINTRGN_print(contents, out, states);                break;
            case U_EMR_EXTSELECTCLIPRGN:        U_EMREXTSELECTCLIPRGN_print(contents, out, states);        break;
            case U_EMR_BITBLT:                  U_EMRBITBLT_print(contents, out, states);                  break;
            case U_EMR_STRETCHBLT:              U_EMRSTRETCHBLT_print(contents, out, states);              break;
            case U_EMR_MASKBLT:                 U_EMRMASKBLT_print(contents, out, states);                 break;
            case U_EMR_PLGBLT:                  U_EMRPLGBLT_print(contents, out, states);                  break;
            case U_EMR_SETDIBITSTODEVICE:       U_EMRSETDIBITSTODEVICE_print(contents, out, states);       break;
            case U_EMR_STRETCHDIBITS:           U_EMRSTRETCHDIBITS_print(contents, out, states);           break;
            case U_EMR_EXTCREATEFONTINDIRECTW:  U_EMREXTCREATEFONTINDIRECTW_print(contents, out, states);  break;
            case U_EMR_EXTTEXTOUTA:             U_EMREXTTEXTOUTA_print(contents, out, states);             break;
            case U_EMR_EXTTEXTOUTW:             U_EMREXTTEXTOUTW_print(contents, out, states);             break;
            case U_EMR_POLYBEZIER16:            U_EMRPOLYBEZIER16_print(contents, out, states);            break;
            case U_EMR_POLYGON16:               U_EMRPOLYGON16_print(contents, out, states);               break;
            case U_EMR_POLYLINE16:              U_EMRPOLYLINE16_print(contents, out, states);              break;
            case U_EMR_POLYBEZIERTO16:          U_EMRPOLYBEZIERTO16_print(contents, out, states);          break;
            case U_EMR_POLYLINETO16:            U_EMRPOLYLINETO16_print(contents, out, states);            break;
            case U_EMR_POLYPOLYLINE16:          U_EMRPOLYPOLYLINE16_print(contents, out, states);          break;
            case U_EMR_POLYPOLYGON16:           U_EMRPOLYPOLYGON16_print(contents, out, states);           break;
            case U_EMR_POLYDRAW16:              U_EMRPOLYDRAW16_print(contents, out, states);              break;
            case U_EMR_CREATEMONOBRUSH:         U_EMRCREATEMONOBRUSH_print(contents, out, states);         break;
            case U_EMR_CREATEDIBPATTERNBRUSHPT: U_EMRCREATEDIBPATTERNBRUSHPT_print(contents, out, states); break;
            case U_EMR_EXTCREATEPEN:            U_EMREXTCREATEPEN_print(contents, out, states);            break;
                                                //case U_EMR_POLYTEXTOUTA:            U_EMRPOLYTEXTOUTA_print(contents, out, states);            break;
                                                //case U_EMR_POLYTEXTOUTW:            U_EMRPOLYTEXTOUTW_print(contents, out, states);            break;
            case U_EMR_SETICMMODE:              U_EMRSETICMMODE_print(contents, out, states);              break;
            case U_EMR_CREATECOLORSPACE:        U_EMRCREATECOLORSPACE_print(contents, out, states);        break;
            case U_EMR_SETCOLORSPACE:           U_EMRSETCOLORSPACE_print(contents, out, states);           break;
            case U_EMR_DELETECOLORSPACE:        U_EMRDELETECOLORSPACE_print(contents, out, states);        break;
                                                //case U_EMR_GLSRECORD:               U_EMRGLSRECORD_print(contents, out, states);               break;
                                                //case U_EMR_GLSBOUNDEDRECORD:        U_EMRGLSBOUNDEDRECORD_print(contents, out, states);        break;
            case U_EMR_PIXELFORMAT:             U_EMRPIXELFORMAT_print(contents, out, states);             break;
                                                //case U_EMR_DRAWESCAPE:              U_EMRDRAWESCAPE_print(contents, out, states);              break;
                                                //case U_EMR_EXTESCAPE:               U_EMREXTESCAPE_print(contents, out, states);               break;
                                                //case U_EMR_UNDEF107:                U_EMRUNDEF107_print(contents, out, states);                break;
            case U_EMR_SMALLTEXTOUT:            U_EMRSMALLTEXTOUT_print(contents, out, states);            break;
                                                //case U_EMR_FORCEUFIMAPPING:         U_EMRFORCEUFIMAPPING_print(contents, out, states);         break;
                                                //case U_EMR_NAMEDESCAPE:             U_EMRNAMEDESCAPE_print(contents, out, states);             break;
                                                //case U_EMR_COLORCORRECTPALETTE:     U_EMRCOLORCORRECTPALETTE_print(contents, out, states);     break;
                                                //case U_EMR_SETICMPROFILEA:          U_EMRSETICMPROFILEA_print(contents, out, states);          break;
                                                //case U_EMR_SETICMPROFILEW:          U_EMRSETICMPROFILEW_print(contents, out, states);          break;
            case U_EMR_ALPHABLEND:              U_EMRALPHABLEND_print(contents, out, states);              break;
            case U_EMR_SETLAYOUT:               U_EMRSETLAYOUT_print(contents, out, states);               break;
            case U_EMR_TRANSPARENTBLT:          U_EMRTRANSPARENTBLT_print(contents, out, states);          break;
                                                //case U_EMR_UNDEF117:                U_EMRUNDEF117_print(contents, out, states);                break;
            case U_EMR_GRADIENTFILL:            U_EMRGRADIENTFILL_print(contents, out, states);            break;
                                                //case U_EMR_SETLINKEDUFIS:           U_EMRSETLINKEDUFIS_print(contents, out, states);           break;
                                                //case U_EMR_SETTEXTJUSTIFICATION:    U_EMRSETTEXTJUSTIFICATION_print(contents, out, states);    break;
                                                //case U_EMR_COLORMATCHTOTARGETW:     U_EMRCOLORMATCHTOTARGETW_print(contents, out, states);     break;
            case U_EMR_CREATECOLORSPACEW:       U_EMRCREATECOLORSPACEW_print(contents, out, states);       break;
            default:                            U_EMRNOTIMPLEMENTED_print("?",contents, out, states);      break;
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
        if ((options->nameSpace != NULL) && (strlen(options->nameSpace) != 0)){
            states->nameSpace = options->nameSpace;
            states->nameSpaceString = (char *)malloc(strlen(options->nameSpace)+2);
            sprintf(states->nameSpaceString, "%s%s", states->nameSpace, ":");
        }
        else{
            states->nameSpaceString = (char *)"";
        }

        states->svgDelimiter = options->svgDelimiter;
        states->currentDeviceContext.font_name = NULL;
        setTransformIdentity(states);

        stream = open_memstream(out, &len);
        if (stream == NULL){
            verbose_printf("Failed to allocate output stream\n");
            return(0);
        }

        blimit = contents + length;

        while(OK){
            if(off>=length){ //normally should exit from while after EMREOF sets OK to false, this is most likely a corrupt EMF
                verbose_printf("WARNING: record claims to extend beyond the end of the EMF file\n");
                return(0);
            }

            pEmr = (PU_ENHMETARECORD)(contents + off);

            if(!recnum && (pEmr->iType != U_EMR_HEADER)){
                verbose_printf("WARNING: EMF file does not begin with an EMR_HEADER record\n");
            }

            result = U_emf_onerec_print(contents, blimit, recnum, off, stream, states);
            if(result == (size_t) -1){
                verbose_printf("ABORTING on invalid record - corrupt file?\n");
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
        FLAG_RESET

        free(states->objectTable);
        free(states);
        freeDeviceContext(&(states->currentDeviceContext));
        freeDeviceContextStack(states);

        fflush(stream);
        fclose(stream);

        return 1;
    }

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
