/**
 Example progam used for exercising the libUEMF functions.  
 Produces a single output file: test_libuemf.emf
 Single command line parameter, hexadecimal bit flag.
   1  Disable tests that block EMF import into PowerPoint (dotted lines)
   2  Enable tests that block EMF being displayed in Windows Preview (currently, GradientFill)
   4  Use a rotated, scaled, offset world transform
   8  Disable clipping tests.
   Default is 0, no option set.

 Compile with 
 
    gcc -g -O0 -o testbed_emf -Wall -I. testbed_emf.c uemf.c uemf_endian.c uemf_utf.c -lm 

 or

    gcc -g -O0 -o testbed_emf -DU_VALGRIND -Wall -I. testbed_emf.c uemf.c uemf_endian.c uemf_utf.c  -lm 


 The latter from enables code which lets valgrind check each record for
 uninitialized data.
 
*/

/* If Version or Date are changed also edit the text labels for the output.

File:      testbed_emf.c
Version:   0.0.23
Date:      25-JUL-2014
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2014 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "uemf.h"

#define PPT_BLOCKERS     1
#define PREVIEW_BLOCKERS 2
#define WORLDXFORM_TEST  4
#define NO_CLIP_TEST     8

// noa special conditions:
// 1 then s2 is expected to have zero in the "a" channel
// 2 then s2 is expected to have zero in the "a" channel AND only top 5 bits are meaningful
int rgba_diff(char *s1, char *s2, uint32_t size, int noa){
   for( ; size ; size--, s1++, s2++){
     if(noa==1){
       if(*s1 != *s2){
         if(noa && !(*s2) && (1 == size % 4))continue;
         return(1);
       }
     }
     if(noa==2){
       if((0xF8 & *s1) != (0xF8 &*s2)){
         if(noa && !(*s2) && (1 == size % 4))continue;
         return(1);
       }
     }
     else {
       if(*s1 != *s2)return(1);
     }
   }
   return(0);
}

void taf(char *rec,EMFTRACK *et, char *text){  // Test, append, free
    if(!rec){ printf("%s failed",text);                     }
    else {    printf("%s recsize: %d",text,U_EMRSIZE(rec)); }
    (void) emf_append((PU_ENHMETARECORD)rec, et, 1);
    printf("\n");
#ifdef U_VALGRIND
    fflush(stdout);  // helps keep lines ordered within Valgrind
#endif
}

/* Call this to figure out where a missing piece of memory is */
void findhole(char *rec, char *text){  // Test
    int i,length;
    unsigned char uc;
    if(!rec){ printf("%s failed",text);                     }
    else { 
        length = U_EMRSIZE(rec);
        printf("%s recsize: %d",text,length);
        for(i=0;i<length;i++){
          uc = (unsigned char) rec[i];
          printf("byte:%d value:%X\n",i,uc); 
          fflush(stdout);  // helps keep lines ordered within Valgrind
        }   
    }
}


uint32_t  makeindex(int i, int j, int w, int h, uint32_t colortype){
uint32_t index = 0;
float scale;
   switch(colortype){
      case U_BCBM_MONOCHROME: //! 2 colors.    bmiColors array has two entries 
        scale=1;               
        break;           
      case U_BCBM_COLOR4:     //! 2^4 colors.  bmiColors array has 16 entries                 
        scale=15;               
        break;           
      case U_BCBM_COLOR8:     //! 2^8 colors.  bmiColors array has 256 entries                
        scale=255;               
        break;           
      case U_BCBM_COLOR16:    //! 2^16 colors. (Several different index methods)) 
        scale=65535;               
      break;           
      case U_BCBM_COLOR24:    //! 2^24 colors. bmiColors is not used. Pixels are U_RGBTRIPLE. 
      case U_BCBM_COLOR32:    //! 2^32 colors. bmiColors is not used. Pixels are U_RGBQUAD.
      case U_BCBM_EXPLICIT:   //! Derinved from JPG or PNG compressed image or ?   
      default:
        exit(EXIT_FAILURE);     
   }
   if(scale > w*h-1)scale=w*h-1; // color table will not have more entries than the size of the image
   index = U_ROUND(scale * ( (float) i * (float)w + (float)j ) / ((float)w * (float)h));
   return(index);
}


/* 
Fill a rectangular RGBA image with gradients.  Used for testing DIB operations.
*/
void FillImage(char *px, int w, int h, int stride){
int        i,j;
int        xp,xm,yp,ym;   // color weighting factors
int        r,g,b,a;
int        pad;
U_COLORREF color;

    pad = stride - w*4;  // end of row padding in bytes (may be zero)
    for(j=0; j<h; j++){
       yp = (255 * j) / (h-1);
       ym = 255 - yp;
       for(i=0; i<w; i++, px+=4){
          xp = (255 * i)/ (w-1);
          xm = 255 - xp;
          r = (xm > ym ? ym : xm);
          g = (xp > ym ? ym : xp);
          b = (xp > yp ? yp : xp);
          a = (xm > yp ? yp : xm);
          color = U_RGBA(r,g,b,a);
          memcpy(px,&color,4);
       }
       px += pad;
    }
}

void spintext(uint32_t x, uint32_t y, uint32_t textalign, uint32_t *font, EMFTRACK *et, EMFHANDLES *eht){
    char               *rec;
    char               *rec2;
    int                 i;
    char               *string;
    uint16_t            *FontName;
    uint16_t            *FontStyle;
    uint16_t            *text16;
    U_LOGFONT            lf;
    U_LOGFONT_PANOSE     elfw;
    int                  slen;
    uint32_t            *dx;
    
    
    rec = U_EMRSETTEXTALIGN_set(textalign);
    taf(rec,et,"U_EMRSETTEXTALIGN_set");
    string = (char *) malloc(32);
    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    FontStyle = U_Utf8ToUtf16le("Normal", 0, NULL);
    for(i=0; i<3600; i+=300){
       rec = selectobject_set(U_DEVICE_DEFAULT_FONT, eht); // Release current font
       taf(rec,et,"selectobject_set");

       if(*font){
          rec  = deleteobject_set(font, eht);  // then delete it
          taf(rec,et,"deleteobject_set");
       }

       // set escapement and orientation in tenths of a degree counter clockwise rotation
       lf   = logfont_set( -30, 0, i, i, 
                         U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                         U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                         U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
       elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
       rec  = extcreatefontindirectw_set(font, eht,  NULL, (char *) &elfw);
       taf(rec,et,"extcreatefontindirectw_set");
       rec = selectobject_set(*font, eht); // make this font active
       taf(rec,et,"selectobject_set");

       sprintf(string,"....Degrees:%d",i/10);
       text16 = U_Utf8ToUtf16le(string, 0, NULL);
       slen   = wchar16len(text16);
       dx = dx_set(-30,  U_FW_NORMAL, slen);
       rec2 = emrtext_set( pointl_set(x,y), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
       free(text16);
       free(dx);
       rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
       taf(rec,et,"U_EMREXTTEXTOUTW_set");
       free(rec2);
    }
    free(FontName);
    free(FontStyle);
    free(string);
}
    
void draw_textrect(int xul, int yul, int width, int height, char *string, int size, EMFTRACK *et){
    char               *rec;
    char               *rec2;
    uint16_t            *text16;
    int                  slen;
    uint32_t            *dx;
    U_RECTL              rclBox;

    rclBox = rectl_set(pointl_set(xul,yul),pointl_set(xul + width, yul + height));
    rec = U_EMRRECTANGLE_set(rclBox);                  taf(rec,et,"U_EMRRECTANGLE_set");

    text16 = U_Utf8ToUtf16le(string, 0, NULL);
    slen   = wchar16len(text16);
    dx = dx_set(-size,  U_FW_NORMAL, slen);
    rec2 = emrtext_set( pointl_set(xul+width/2,yul + height/2), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
    free(text16);
    free(dx);
    rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTW_set");
    free(rec2);
}   



void textlabel(uint32_t size, const char *string, uint32_t x, uint32_t y, uint32_t *font, EMFTRACK *et, EMFHANDLES *eht){
    char               *rec;
    char               *rec2;
    uint16_t            *FontName;
    uint16_t            *FontStyle;
    uint16_t            *text16;
    U_LOGFONT            lf;
    U_LOGFONT_PANOSE     elfw;
    int                  slen;
    uint32_t            *dx;
    
    
    rec = U_EMRSETTEXTALIGN_set(U_TA_DEFAULT);
    taf(rec,et,"U_EMRSETTEXTALIGN_set");
    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    FontStyle = U_Utf8ToUtf16le("Normal", 0, NULL);
    rec = selectobject_set(U_DEVICE_DEFAULT_FONT, eht); // Release current font
    taf(rec,et,"selectobject_set");

    if(*font){
       rec  = deleteobject_set(font, eht);  // then delete it
       taf(rec,et,"deleteobject_set");
    }

    // set escapement and orientation in tenths of a degree counter clockwise rotation
    lf   = logfont_set( -size, 0, 0, 0, 
                      U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
    elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
    rec  = extcreatefontindirectw_set(font, eht,  NULL, (char *) &elfw);
    taf(rec,et,"extcreatefontindirectw_set");
    rec = selectobject_set(*font, eht); // make this font active
    taf(rec,et,"selectobject_set");

    text16 = U_Utf8ToUtf16le(string, 0, NULL);
    slen   = wchar16len(text16);
    dx = dx_set(-size,  U_FW_NORMAL, slen);
    rec2 = emrtext_set( pointl_set(x,y), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
    free(text16);
    free(dx);
    rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTW_set");
    free(rec2);
    free(FontName);
    free(FontStyle);
}

void label_column(int x1, int y1, uint32_t *font, EMFTRACK *et, EMFHANDLES *eht){
    textlabel(40, "STRETCHDIBITS 1", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "BITBLT        1", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "STRETCHBLT    1", x1, y1, font, et, eht);          y1 += 240;
    textlabel(40, "STRETCHDIBITS 2", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "BITBLT        2", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "STRETCHBLT    2", x1, y1, font, et, eht);          y1 += 240;
    textlabel(40, "STRETCHDIBITS 3", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "BITBLT        3", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "STRETCHBLT    3", x1, y1, font, et, eht);          y1 += 240;
    textlabel(40, "STRETCHDIBITS 4", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "BITBLT        4", x1, y1, font, et, eht);          y1 += 220;
    textlabel(40, "STRETCHBLT    4", x1, y1, font, et, eht);          y1 += 220;
    return;
}

void label_row(int x1, int y1, uint32_t *font, EMFTRACK *et, EMFHANDLES *eht){
    textlabel(30, "+COLOR32 ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+COLOR24 ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+COLOR16 ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "-COLOR16 ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+COLOR8  ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+COLOR4  ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+MONO    ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "-MONO    ", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+COLOR8 0", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+COLOR4 0", x1, y1, font, et, eht);          x1 += 220;
    textlabel(30, "+MONO   0", x1, y1, font, et, eht);          x1 += 220;
    return;
}

void image_column(EMFTRACK *et, int x1, int y1, int w, int h, PU_BITMAPINFO Bmi, uint32_t cbPx, char *px){
    char *rec;
    int   step=0;

    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(0,0), 
       pointl_set(w,h),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    step += 220;

    rec = U_EMRBITBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(0,0),
       xform_set(1, 0, 0, 1, 0, 0),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRBITBLT_set");
    step += 220;

    rec = U_EMRSTRETCHBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(0,0), 
       pointl_set(w,h),
       xform_set(1, 0, 0, 1, 0, 0),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHBLT_set");
    step += 240;

    /* offset in src, not all of src, nothing ambiguous about piece requested */
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(2,2), 
       pointl_set(w-2,h-2),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    step += 220;

    /* Because there is no w,h, so offset must go back to 0,0*/
    rec = U_EMRBITBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(2,2),
       xform_set(1, 0, 0, 1, -2, -2),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRBITBLT_set");
    step += 220;

    rec = U_EMRSTRETCHBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(3,3), 
       pointl_set(w-2,h-2),
       xform_set(1, 0, 0, 1, -1, -1),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHBLT_set");
    step += 240;

    /* offset in src, not all of src, both offset and w/h are reduced by 2, so some of the selected area is outside of the array */
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(-2,-2), 
       pointl_set(w-2,h-2),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    step += 220;

    /* There is no w,h so size is what, whole bitmap?*/
    rec = U_EMRBITBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(-3,-3),
       xform_set(1, 0, 0, 1, 1, 1),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRBITBLT_set");
    step += 220;

    rec = U_EMRSTRETCHBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(-3,-3), 
       pointl_set(w-2,h-2),
       xform_set(1, 0, 0, 1, 1, 1),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHBLT_set");
    step += 240;

    /* offset in src,no w,h change, so selected region starts in bitmap and extends outside  */
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(2,2), 
       pointl_set(w,h),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    step += 220;

    /* There is no w,h so size is what, whole bitmap?*/
    rec = U_EMRBITBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(1,1),
       xform_set(1, 0, 0, 1, 1, 1),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRBITBLT_set");
    step += 220;

    rec = U_EMRSTRETCHBLT_set(
       U_RCL_DEF,
       pointl_set(x1,y1 + step),
       pointl_set(200,200),
       pointl_set(1,1), 
       pointl_set(w,h),
       xform_set(1, 0, 0, 1, 1, 1),
       colorref_set(64,64,64),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHBLT_set");
    step += 220;

}

void draw_star(EMFTRACK *et, EMFHANDLES *eht, uint32_t *font, U_RECTL rclFrame, int x, int y){
   U_POINT Star[] = {
      {593,688},
      {381,501},
      {166,684},
      {278,425},
      { 38,276},
      {319,303},
      {386, 28},
      {447,304},
      {729,283},
      {486,427}
   };
   U_POINT *points;
   char *rec;

   textlabel(40, "TextBeneathStarTest1", x -30 , y + 60, font, et, eht);
   points = points_transform( Star, 10, xform_alt_set(0.5, 1.0, 0.0, 0.0, x, y));
   rec = selectobject_set(U_BLACK_PEN, eht);                       taf(rec,et,"selectobject_set");
   rec = selectobject_set(U_GRAY_BRUSH, eht);                      taf(rec,et,"selectobject_set");
   rec = U_EMRSETPOLYFILLMODE_set(U_WINDING);                      taf(rec,et,"U_EMRSETPOLYFILLMODE_set");
   rec = U_EMRBEGINPATH_set();                                     taf(rec,et,"U_EMRBEGINPATH_set");
   rec = U_EMRPOLYGON_set(findbounds(10, points, 0), 10, points);  taf(rec,et,"U_EMRPOLYGON_set");
   rec = U_EMRENDPATH_set();                                       taf(rec,et,"U_EMRENDPATH_set");
   rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                     taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
   textlabel(40, "TextAbove__StarTest2", x -30 , y + 210, font, et, eht);
   free(points);
}

void test_clips(int x, int y, uint32_t *font, U_RECTL rclFrame, EMFTRACK *et, EMFHANDLES *eht){
   char *rec;
   uint32_t      redpen;
   U_EXTLOGPEN  *elp;
   U_COLORREF    cr;
   U_POINT blob7[] = {
      {  0,  0},
      {200, 10},
      {500,100},
      {600,800},
      {350,700},
      {150,400},
      {100,100},
   };
   U_POINT *points;

   /* define a red pen */
   cr = colorref_set(255, 0, 0);
   elp = extlogpen_set(U_PS_SOLID|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|U_PS_GEOMETRIC, 2, U_BS_SOLID, cr, U_HS_HORIZONTAL,0,NULL);
   rec = extcreatepen_set(&redpen, eht,  NULL, 0, NULL, elp );      taf(rec,et,"emrextcreatepen_set");
   free(elp);

   textlabel(40, "NoClip", x , y - 60, font, et, eht);
   draw_star(et,eht,font, rclFrame, x,y);

   /* rectangle clipping */
   y += 500;
   textlabel(40, "Rect (include)", x , y - 60, font, et, eht);
   rec = selectobject_set(redpen, eht);                             taf(rec,et,"selectobject_set");
   rec = selectobject_set(U_NULL_BRUSH, eht);                       taf(rec,et,"selectobject_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y, x+200, y+400});         taf(rec,et,"U_EMRRECTANGLE_set");

   rec = U_EMRSAVEDC_set();                                         taf(rec,et,"U_EMRSAVEDC_set");
   rec = U_EMRINTERSECTCLIPRECT_set((U_RECTL){x, y, x+200, y+400}); taf(rec,et,"U_EMRINTERSECTCLIPRECT_set");
   draw_star(et,eht,font, rclFrame, x,y);
   rec = U_EMRRESTOREDC_set(-1);                                    taf(rec,et,"U_EMRSAVEDC_set");

   /* double rectangle clipping */
   y += 500;
   textlabel(40, "Rects (include,include)", x , y - 60, font, et, eht);
   rec = selectobject_set(redpen, eht);                             taf(rec,et,"selectobject_set");
   rec = selectobject_set(U_NULL_BRUSH, eht);                       taf(rec,et,"selectobject_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y, x+200, y+400});         taf(rec,et,"U_EMRRECTANGLE_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y+200, x+400, y+400});     taf(rec,et,"U_EMRRECTANGLE_set");

   rec = U_EMRSAVEDC_set();                                         taf(rec,et,"U_EMRSAVEDC_set");
   rec = U_EMRINTERSECTCLIPRECT_set((U_RECTL){x, y, x+200, y+400}); taf(rec,et,"U_EMRINTERSECTCLIPRECT_set");
   rec = U_EMRINTERSECTCLIPRECT_set((U_RECTL){x,y+200,x+400,y+400});taf(rec,et,"U_EMRINTERSECTCLIPRECT_set");
   draw_star(et,eht,font, rclFrame, x,y);
   rec = U_EMRRESTOREDC_set(-1);                                    taf(rec,et,"U_EMRSAVEDC_set");

   /* excluded rectangle clipping */
   y += 500;
   textlabel(40, "Rect (exclude)", x , y - 60, font, et, eht);
   rec = selectobject_set(redpen, eht);                             taf(rec,et,"selectobject_set");
   rec = selectobject_set(U_NULL_BRUSH, eht);                       taf(rec,et,"selectobject_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y, x+200, y+400});         taf(rec,et,"U_EMRRECTANGLE_set");

   rec = U_EMRSAVEDC_set();                                         taf(rec,et,"U_EMRSAVEDC_set");
   rec = U_EMREXCLUDECLIPRECT_set((U_RECTL){x, y, x+200, y+400});   taf(rec,et,"U_EMREXCLUDECLIPRECT_set");
   draw_star(et,eht,font, rclFrame, x,y);
   rec = U_EMRRESTOREDC_set(-1);                                    taf(rec,et,"U_EMRSAVEDC_set");

   /* double excluded rectangle clipping */
   y += 500;
   textlabel(40, "Rects (exclude,exclude)", x , y - 60, font, et, eht);
   rec = selectobject_set(redpen, eht);                             taf(rec,et,"selectobject_set");
   rec = selectobject_set(U_NULL_BRUSH, eht);                       taf(rec,et,"selectobject_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y, x+200, y+400});         taf(rec,et,"U_EMRRECTANGLE_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y+200, x+400, y+400});     taf(rec,et,"U_EMRRECTANGLE_set");

   rec = U_EMRSAVEDC_set();                                         taf(rec,et,"U_EMRSAVEDC_set");
   rec = U_EMREXCLUDECLIPRECT_set((U_RECTL){x, y, x+200, y+400});   taf(rec,et,"U_EMREXCLUDECLIPRECT_set");
   rec = U_EMREXCLUDECLIPRECT_set((U_RECTL){x,y+200,x+400,y+400});  taf(rec,et,"U_EMREXCLUDECLIPRECT_set");
   draw_star(et,eht,font, rclFrame, x,y);
   rec = U_EMRRESTOREDC_set(-1);                                    taf(rec,et,"U_EMRSAVEDC_set");

   /* rectangle clipping then path LOGIC */
   int i;
   for(i=U_RGN_MIN;i<=U_RGN_MAX;i++){
      y += 500;
      switch(i){
         case U_RGN_AND:
            textlabel(40, "Rect (include) AND path", x , y - 60, font, et, eht); break;
         case U_RGN_OR:
            textlabel(40, "Rect (include) OR path", x , y - 60, font, et, eht); break;
         case U_RGN_XOR: 
            textlabel(40, "Rect (include) XOR path", x , y - 60, font, et, eht); break;
         case U_RGN_DIFF:
            textlabel(40, "Rect (include) DIFF path", x , y - 60, font, et, eht); break;
         case U_RGN_COPY:
            textlabel(40, "Rect (include) COPY path", x , y - 60, font, et, eht); break;
      }
      rec = selectobject_set(redpen, eht);                             taf(rec,et,"selectobject_set");
      rec = selectobject_set(U_NULL_BRUSH, eht);                       taf(rec,et,"selectobject_set");
      rec = U_EMRRECTANGLE_set((U_RECTL){x, y, x+200, y+400});         taf(rec,et,"U_EMRRECTANGLE_set");
      points = points_transform( blob7, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, x, y));
      rec = U_EMRSETPOLYFILLMODE_set(U_WINDING);                       taf(rec,et,"U_EMRSETPOLYFILLMODE_set");
      rec = U_EMRBEGINPATH_set();                                      taf(rec,et,"U_EMRBEGINPATH_set");
      rec = U_EMRPOLYGON_set(findbounds(7, points, 0), 7, points);     taf(rec,et,"U_EMRPOLYGON_set");
      rec = U_EMRENDPATH_set();                                        taf(rec,et,"U_EMRENDPATH_set");
      rec = U_EMRSTROKEPATH_set(rclFrame);                             taf(rec,et,"U_EMRSTROKEPATH_set");

      rec = U_EMRSAVEDC_set();                                         taf(rec,et,"U_EMRSAVEDC_set");
      rec = U_EMRINTERSECTCLIPRECT_set((U_RECTL){x, y, x+200, y+400}); taf(rec,et,"U_EMRINTERSECTCLIPRECT_set");
      rec = U_EMRBEGINPATH_set();                                      taf(rec,et,"U_EMRBEGINPATH_set");
      rec = U_EMRPOLYGON_set(findbounds(7, points, 0), 7, points);     taf(rec,et,"U_EMRPOLYGON_set");
      rec = U_EMRENDPATH_set();                                        taf(rec,et,"U_EMRENDPATH_set");
      rec = U_EMRSELECTCLIPPATH_set(i);                                taf(rec,et,"U_EMRSELECTCLIPPATH_set");
      draw_star(et,eht,font, rclFrame, x,y);
      rec = U_EMRRESTOREDC_set(-1);                                    taf(rec,et,"U_EMRSAVEDC_set");
      free(points);
   }

   /* rectangle clipping then offset */
   y += 500;
   int ox = 100;
   int oy = 20;
   textlabel(40, "Rect (include,offset)", x , y - 60, font, et, eht);
   rec = selectobject_set(redpen, eht);                             taf(rec,et,"selectobject_set");
   rec = selectobject_set(U_NULL_BRUSH, eht);                       taf(rec,et,"selectobject_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y, x+200, y+400});         taf(rec,et,"U_EMRRECTANGLE_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x+ox, y+oy, x+200+ox, y+400+oy});         taf(rec,et,"U_EMRRECTANGLE_set");

   rec = U_EMRSAVEDC_set();                                         taf(rec,et,"U_EMRSAVEDC_set");
   rec = U_EMRINTERSECTCLIPRECT_set((U_RECTL){x, y, x+200, y+400}); taf(rec,et,"U_EMRINTERSECTCLIPRECT_set");
   rec = U_EMROFFSETCLIPRGN_set((U_POINTL){ox,oy});                 taf(rec,et,"U_EMROFFSETCLIPRGN_set");
   draw_star(et,eht,font, rclFrame, x,y);
   rec = U_EMRRESTOREDC_set(-1);                                    taf(rec,et,"U_EMRSAVEDC_set");

   /* double rectangle clipping  then offset */
   y += 500;
   textlabel(40, "Rects (include,include,offset)", x , y - 60, font, et, eht);
   rec = selectobject_set(redpen, eht);                             taf(rec,et,"selectobject_set");
   rec = selectobject_set(U_NULL_BRUSH, eht);                       taf(rec,et,"selectobject_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y, x+200, y+400});         taf(rec,et,"U_EMRRECTANGLE_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x, y+200, x+400, y+400});     taf(rec,et,"U_EMRRECTANGLE_set");
   rec = U_EMRRECTANGLE_set((U_RECTL){x+ox, y+200+oy, x+200+ox, y+400+oy});
                                                                    taf(rec,et,"U_EMRRECTANGLE_set");

   rec = U_EMRSAVEDC_set();                                         taf(rec,et,"U_EMRSAVEDC_set");
   rec = U_EMRINTERSECTCLIPRECT_set((U_RECTL){x, y, x+200, y+400}); taf(rec,et,"U_EMRINTERSECTCLIPRECT_set");
   rec = U_EMRINTERSECTCLIPRECT_set((U_RECTL){x,y+200,x+400,y+400});taf(rec,et,"U_EMRINTERSECTCLIPRECT_set");
   rec = U_EMROFFSETCLIPRGN_set((U_POINTL){ox,oy});                 taf(rec,et,"U_EMROFFSETCLIPRGN_set");
   draw_star(et,eht,font, rclFrame, x,y);
   rec = U_EMRRESTOREDC_set(-1);                                    taf(rec,et,"U_EMRSAVEDC_set");

}

int main(int argc, char *argv[]){
    EMFTRACK            *et;
    EMFHANDLES          *eht;
    U_POINT              pl1;
    U_POINT              pl12[12];
    uint32_t             plc[12];
    U_POINT              plarray[7]   = { { 200, 0 }, { 400, 200 }, { 400, 400 }, { 200, 600 } , { 0, 400 }, { 0, 200 }, { 200, 0 }};
    U_POINT16            plarray16[7] = { { 200, 0 }, { 400, 200 }, { 400, 400 }, { 200, 600 } , { 0, 400 }, { 0, 200 }, { 200, 0 }};
    U_POINT              star5[5]     = { { 100, 0 }, { 200, 300 }, { 0, 100 }, { 200, 100 } , { 0, 300 }};
    PU_POINT             points;
    PU_POINT16           point16;
    int                  status;
    char                *rec;
    char                *rec2;
    char                *string;
    char                *px;
    char                *rgba_px;
    char                *rgba_px2;
    U_RECTL              rclBounds,rclFrame,rclBox;
    U_SIZEL              szlDev,szlMm,corners;
    uint16_t            *Description;
    uint16_t            *FontStyle;
    uint16_t            *FontName;
    uint16_t            *text16;
    uint32_t            *dx;
    size_t               slen;
    size_t               len;
    uint32_t             pen=0;   //none of these have been defined yet
    uint32_t             brush=0;
    uint32_t             font=0;;
    U_XFORM              xform;
    U_LOGBRUSH           lb;
    U_EXTLOGPEN         *elp;
    U_COLORREF           cr;
    U_POINTL             ul,lr;
    U_BITMAPINFOHEADER   Bmih;
    PU_BITMAPINFO        Bmi;
    U_LOGFONT            lf;
    U_LOGFONT_PANOSE     elfw;
    U_NUM_STYLEENTRY     elpNumEntries;
    U_STYLEENTRY        *elpStyleEntry;
    uint32_t             elpBrushStyle;
    uint32_t             cbDesc;
    uint32_t             cbPx;
    uint32_t             colortype;
    PU_RGBQUAD           ct;         //color table
    int                  numCt;      //number of entries in the color table
    int                  i,j,k,m;
    int                  hatch;
    char                 cbuf[132];

    int                  mode = 0;   // conditional for some tests
    unsigned int         umode;
    double               sc;
    int                  offset;
    int                  cap, join, miter;
    uint8_t pngarray[138]= {
       0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,
       0x00,0x0D,0x49,0x48,0x44,0x52,0x00,0x00,0x00,0x0A,
       0x00,0x00,0x00,0x0A,0x08,0x02,0x00,0x00,0x00,0x02,
       0x50,0x58,0xEA,0x00,0x00,0x00,0x50,0x49,0x44,0x41,
       0x54,0x18,0x95,0x6D,0x8E,0xC1,0x0D,0xC0,0x30,0x08,
       0x03,0xAF,0x12,0x63,0xC0,0xFE,0x83,0x01,0x73,0xB4,
       0x8F,0x54,0x09,0x41,0xF8,0x85,0x30,0x67,0xF3,0xBC,
       0x10,0x4A,0x28,0x6E,0xC3,0x20,0x01,0xA0,0x8C,0x8A,
       0xDF,0x9E,0x2E,0x12,0x40,0xFC,0x2C,0xAC,0x72,0x4B,
       0x9B,0x2E,0x19,0x65,0xD5,0x6C,0xC8,0xAB,0xA5,0x86,
       0x6F,0xEE,0xB4,0x14,0xFA,0xCA,0xD1,0xDB,0xCE,0xFE,
       0xF8,0xBA,0x10,0xEF,0x5C,0x95,0x7D,0x4F,0x0D,0x1B,
       0x2C,0x7B,0x16,0x38,0x83,0x00,0x00,0x00,0x00,0x00,
       0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82
    };


    uint8_t jpgarray[676]= {
       0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,
       0x00,0x01,0x01,0x01,0x00,0x48,0x00,0x48,0x00,0x00,
       0xFF,0xDB,0x00,0x43,0x00,0x03,0x02,0x02,0x03,0x02,
       0x02,0x03,0x03,0x03,0x03,0x04,0x03,0x03,0x04,0x05,
       0x08,0x05,0x05,0x04,0x04,0x05,0x0A,0x07,0x07,0x06,
       0x08,0x0C,0x0A,0x0C,0x0C,0x0B,0x0A,0x0B,0x0B,0x0D,
       0x0E,0x12,0x10,0x0D,0x0E,0x11,0x0E,0x0B,0x0B,0x10,
       0x16,0x10,0x11,0x13,0x14,0x15,0x15,0x15,0x0C,0x0F,
       0x17,0x18,0x16,0x14,0x18,0x12,0x14,0x15,0x14,0xFF,
       0xDB,0x00,0x43,0x01,0x03,0x04,0x04,0x05,0x04,0x05,
       0x09,0x05,0x05,0x09,0x14,0x0D,0x0B,0x0D,0x14,0x14,
       0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
       0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
       0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
       0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
       0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0xFF,0xC2,
       0x00,0x11,0x08,0x00,0x0A,0x00,0x0A,0x03,0x01,0x11,
       0x00,0x02,0x11,0x01,0x03,0x11,0x01,0xFF,0xC4,0x00,
       0x16,0x00,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x06,
       0x07,0xFF,0xC4,0x00,0x15,0x01,0x01,0x01,0x00,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x05,0x07,0xFF,0xDA,0x00,0x0C,0x03,0x01,
       0x00,0x02,0x10,0x03,0x10,0x00,0x00,0x01,0x8E,0x21,
       0xA4,0x13,0x83,0x61,0x14,0x3A,0xAA,0xD6,0x43,0x3F,
       0xFF,0xC4,0x00,0x1C,0x10,0x00,0x01,0x03,0x05,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x03,0x00,0x01,0x05,0x02,0x04,0x12,0x13,0x21,
       0xFF,0xDA,0x00,0x08,0x01,0x01,0x00,0x01,0x05,0x02,
       0x89,0xBF,0x16,0x87,0x9E,0xE8,0x09,0x52,0xC9,0xD7,
       0xFF,0xC4,0x00,0x1F,0x11,0x00,0x02,0x01,0x02,0x07,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x11,0x15,
       0x21,0x22,0x31,0xFF,0xDA,0x00,0x08,0x01,0x03,0x01,
       0x01,0x3F,0x01,0xCC,0xD7,0x9B,0x8E,0xB1,0x8F,0xA4,
       0xF0,0xAE,0xCF,0x82,0xA8,0xD2,0x3F,0xFF,0xC4,0x00,
       0x1E,0x11,0x00,0x01,0x04,0x01,0x05,0x00,0x00,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
       0x02,0x03,0x21,0x05,0x06,0x11,0x14,0x16,0x62,0xFF,
       0xDA,0x00,0x08,0x01,0x02,0x01,0x01,0x3F,0x01,0x8B,
       0x09,0xCD,0xA4,0x43,0xA5,0x79,0x34,0xFB,0x52,0xA8,
       0x6C,0x6C,0xD9,0x28,0xFF,0xC4,0x00,0x1A,0x10,0x00,
       0x02,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x32,0x22,
       0x31,0x91,0xFF,0xDA,0x00,0x08,0x01,0x01,0x00,0x06,
       0x3F,0x02,0x52,0x93,0xC8,0xB1,0x67,0xD3,0x6C,0xFF,
       0xC4,0x00,0x19,0x10,0x00,0x02,0x03,0x01,0x00,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x01,0x11,0x21,0x51,0xF1,0xFF,0xDA,0x00,0x08,
       0x01,0x01,0x00,0x01,0x3F,0x21,0x49,0x4A,0x94,0xB4,
       0x81,0x82,0xB0,0x2E,0xF1,0xFF,0xDA,0x00,0x0C,0x03,
       0x01,0x00,0x02,0x00,0x03,0x00,0x00,0x00,0x10,0xD9,
       0x0F,0xFF,0xC4,0x00,0x1A,0x11,0x01,0x01,0x00,0x02,
       0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x01,0x00,0x11,0x21,0x41,0xC1,0xF0,0xFF,
       0xDA,0x00,0x08,0x01,0x03,0x01,0x01,0x3F,0x10,0xDC,
       0x9E,0x77,0x2A,0x8F,0x26,0x6B,0x95,0x94,0xAA,0x5F,
       0xFF,0xC4,0x00,0x1A,0x11,0x01,0x00,0x02,0x03,0x01,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x01,0x00,0x21,0x31,0x41,0x91,0xC1,0xFF,0xDA,
       0x00,0x08,0x01,0x02,0x01,0x01,0x3F,0x10,0xAB,0x7E,
       0x0D,0x0F,0x31,0x6D,0x5C,0x88,0xAB,0x83,0x44,0xFF,
       0xC4,0x00,0x19,0x10,0x01,0x01,0x01,0x00,0x03,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x01,0x00,0x11,0x21,0x31,0x41,0xFF,0xDA,0x00,0x08,
       0x01,0x01,0x00,0x01,0x3F,0x10,0x31,0xB1,0xA5,0x69,
       0x84,0x00,0x28,0x03,0xD4,0x41,0x08,0x38,0x01,0x43,
       0x82,0xA2,0xFB,0xAB,0xFF,0xD9
    };
    
    
    if(argc<2){                                              mode = -1; }
    else if(argc==2){
       if(1 != sscanf(argv[1],"%X",&umode)){ mode = -1; }// note, undefined bits may be set without generating warnings
       else { mode = umode; }
    }
    else {                                                   mode = -1; }
    if(mode<0){
      printf("Syntax: testbed_emf flags\n");
      printf("   Flags is a hexadecimal number composed of the following optional bits (use 0 for a standard run):\n");
      printf("   1  Disable tests that block EMF import into PowerPoint (dotted lines)\n");
      printf("   2  Enable tests that block EMF being displayed in Windows Preview (currently, none)\n");
      printf("   4  Rotate and scale the test image within the page.\n");
      printf("   8  Disable clipping tests.\n");
      exit(EXIT_FAILURE);
    }

    char     *step0 = U_strdup("Ever play telegraph?");
    uint16_t *step1 = U_Utf8ToUtf16le(     step0, 0, NULL);
    uint32_t *step2 = U_Utf16leToUtf32le(  step1, 0, NULL); 
    uint16_t *step3 = U_Utf32leToUtf16le(  step2, 0, NULL);  
    char     *step4 = U_Utf16leToUtf8(     step3, 0, NULL); 
    uint32_t *step5 = U_Utf8ToUtf32le(     step4, 0, NULL); 
    char     *step6 = U_Utf32leToUtf8(     step5, 0, &len); 
    printf("telegraph size: %d, string:<%s>\n", (int) len,step6);
    free(step0);  
    free(step1);  
    free(step2);  
    free(step3);  
    free(step4);  
    free(step5);  
    free(step6);  

    step0 = U_strdup("Ever play telegraph?");
    step1 = U_Utf8ToUtf16le(     step0,   4, &len);
    step2 = U_Utf16leToUtf32le(  step1, len, &len); 
    step3 = U_Utf32leToUtf16le(  step2, len, &len); 
    step4 = U_Utf16leToUtf8(     step3, len, &len); 
    step5 = U_Utf8ToUtf32le(     step4,   4, &len); 
    step6 = U_Utf32leToUtf8(     step5, len, &len); 
    printf("telegraph size: %d, string (should be just \"Ever\"):<%s>\n",(int) len,step6);
    free(step0);  
    free(step1);  
    free(step2);  
    free(step3);  
    free(step4);  
    free(step5);  
    free(step6);  
 
 
    /* ********************************************************************** */
    // set up and begin the EMF
 
    status=emf_start("test_libuemf.emf",1000000, 250000, &et);  // space allocation initial and increment 
    status=emf_htable_create(128, 128, &eht);

    (void) device_size(216, 279, 47.244094, &szlDev, &szlMm); // Example: device is Letter vertical, 1200 dpi = 47.244 DPmm
    (void) drawing_size(297, 210, 47.244094, &rclBounds, &rclFrame);  // Example: drawing is A4 horizontal,  1200 dpi = 47.244 DPmm
    Description = U_Utf8ToUtf16le("Test EMF\1produced by libUEMF testbed_emf program\1",0, NULL); 
    cbDesc = 2 + wchar16len(Description);  // also count the final terminator
    (void) U_Utf16leEdit(Description, U_Utf16le(1), 0);
    rec = U_EMRHEADER_set( rclBounds,  rclFrame,  NULL, cbDesc, Description, szlDev, szlMm, 0);
    taf(rec,et,"U_EMRHEADER_set");
    free(Description);

    rec = textcomment_set("First comment");
    taf(rec,et,"textcomment_set");

    rec = textcomment_set("Second comment");
    taf(rec,et,"textcomment_set");

    rec = U_EMRSETMAPMODE_set(U_MM_TEXT);
    taf(rec,et,"U_EMRSETMAPMODE_set");

    sc = 0.635659913;
    if(mode & WORLDXFORM_TEST){ xform = xform_set(sc*cos(0.52359877559), -sc*sin(0.52359877559), sc*sin(0.52359877559), sc*cos(0.52359877559), 
                                           0, 4459.5);} /* rotated and scaled to fit at an angle in same page.  Offset may be off a tiny fraction. */
    else {                      xform = xform_set(1, 0, 0, 1, 0, 0);  } 
    rec = U_EMRMODIFYWORLDTRANSFORM_set(xform, U_MWT_LEFTMULTIPLY);
    taf(rec,et,"U_EMRMODIFYWORLDTRANSFORM_set");

    rec = U_EMRSETBKMODE_set(U_TRANSPARENT); //! iMode uses BackgroundMode Enumeration
    taf(rec,et,"U_EMRSETBKMODE_set");

    rec = U_EMRSETMITERLIMIT_set(8);
    taf(rec,et,"U_EMRSETMITERLIMIT_set");

    rec = U_EMRSETPOLYFILLMODE_set(U_WINDING); //! iMode uses PolygonFillMode Enumeration
    taf(rec,et,"U_EMRSETPOLYFILLMODE_set");

    /* **********put a rectangle under everything, so that transparency is obvious ******************* */
    cr = colorref_set(255,255,173);  //yellowish color, only use here
    lb = logbrush_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
    rec = createbrushindirect_set(&brush, eht,lb);     taf(rec,et,"createbrushindirect_set");
    rec = selectobject_set(brush, eht);                taf(rec,et,"selectobject_set");
    rec = selectobject_set(U_BLACK_PEN, eht);          taf(rec,et,"selectobject_set");

    rclBox = rectl_set(pointl_set(0,0),pointl_set(14031,9921));
    rec = U_EMRRECTANGLE_set(rclBox);                  taf(rec,et,"U_EMRRECTANGLE_set");

    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);        taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    rec  = deleteobject_set(&brush, eht);              taf(rec,et,"deleteobject_set");

    /* ********** other drawing tests ******************* */

    cr = colorref_set(196, 127, 255);
    lb = logbrush_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
    rec = createbrushindirect_set(&brush, eht,lb);                 taf(rec,et,"createbrushindirect_set");
/* tested and works
    (void) emf_htable_insert(&ih, eht);
    rec = U_EMRCREATEBRUSHINDIRECT_set(ih,lb);
    taf(rec,et,"U_EMRCREATEBRUSHINDIRECT_set");
*/

    rec = selectobject_set(brush, eht); // make brush which is at handle 1 active
    taf(rec,et,"selectobject_set");

    rec=U_EMRSETPOLYFILLMODE_set(U_WINDING);
    taf(rec,et,"U_EMRSETPOLYFILLMODE_set");

    cr = colorref_set(127, 255, 196);
    elp = extlogpen_set(U_PS_SOLID|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|U_PS_GEOMETRIC, 50, U_BS_SOLID, cr, U_HS_HORIZONTAL,0,NULL);
    rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
    free(elp);

    rec = selectobject_set(pen, eht); // make pen just created active
    taf(rec,et,"selectobject_set");


    /* label the drawing */
    
    textlabel(400, "libUEMF v0.1.17",     9700, 200, &font, et, eht);
    textlabel(400, "July 25, 2014",       9700, 500, &font, et, eht);
    rec = malloc(128);
    (void)sprintf(rec,"EMF test: %2.2X",mode);
    textlabel(400, rec,                   9700, 800, &font, et, eht);
    free(rec);

    /* ********************************************************************** */
    // basic drawing operations

    ul     = pointl_set(100,1300);
    lr     = pointl_set(400,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRELLIPSE_set(rclBox);
    taf(rec,et,"U_EMRELLIPSE_set");

    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");


    ul     = pointl_set(500,1300);
    lr     = pointl_set(800,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRRECTANGLE_set(rclBox);
    taf(rec,et,"U_EMRRECTANGLE_set");

    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");


    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(900,1300));                taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRLINETO_set(pointl_set(1200,1500));                 taf(rec,et,"U_EMRLINETO_set");
    rec = U_EMRLINETO_set(pointl_set(1200,1700));                 taf(rec,et,"U_EMRLINETO_set");
    rec = U_EMRLINETO_set(pointl_set(900,1500));                  taf(rec,et,"U_EMRLINETO_set");
    rec = U_EMRCLOSEFIGURE_set();                                 taf(rec,et,"U_EMRCLOSEFIGURE_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");


    ul     = pointl_set(1300,1300);
    lr     = pointl_set(1600,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRARC_set(rclBox, ul, pointl_set(1450,1600));     taf(rec,et,"U_EMRARC_set");
 
    rec = U_EMRSETARCDIRECTION_set(U_AD_COUNTERCLOCKWISE);
    taf(rec,et,"U_SETARCDIRECTION_set");


    ul     = pointl_set(1600,1300);
    lr     = pointl_set(1900,1600);
    rclBox = rectl_set(ul,lr);
    // arcto initially draws a line from current position to its start.
    rec = U_EMRMOVETOEX_set(pointl_set(1800,1300));              taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRARCTO_set(rclBox, ul, pointl_set(1750,1600));     taf(rec,et,"U_EMRARCTO_set");
    rec = U_EMRLINETO_set(pointl_set(1750,1450));                taf(rec,et,"U_EMRLINETO_set");




    ul     = pointl_set(1900,1300);
    lr     = pointl_set(2200,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRCHORD_set(rclBox, ul,  pointl_set(2050,1600));   taf(rec,et,"U_EMRCHORD_set");

    rec = U_EMRSETARCDIRECTION_set(U_AD_CLOCKWISE);
    taf(rec,et,"U_SETARCDIRECTION_set");
    ul     = pointl_set(2200,1300);
    lr     = pointl_set(2500,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRPIE_set(rclBox, ul,  pointl_set(2350,1600));     taf(rec,et,"U_EMRPIE_set"); 

    ul      = pointl_set(2600,1300);
    lr      = pointl_set(2900,1600);
    rclBox  = rectl_set(ul,lr);
    corners = sizel_set(100,25);
    rec = U_EMRROUNDRECT_set(rclBox,corners);
    taf(rec,et,"U_EMRROUNDRECT_set");


    rec = selectobject_set(U_BLACK_PEN, eht); // make stock object BLACK_PEN active
    taf(rec,et,"selectobject_set");

    rec = selectobject_set(U_GRAY_BRUSH, eht); // make stock object GREY_BRUSH active
    taf(rec,et,"selectobject_set");

    if(brush){
       rec = deleteobject_set(&brush, eht); // disable brush which is at handle 1, it should not be selected when this happens!!!
       taf(rec,et,"deleteobject_set");
    }

    if(pen){
       rec = deleteobject_set(&pen, eht); // disable pen which is at handle 2, it should not be selected when this happens!!!
       taf(rec,et,"deleteobject_set");
    }

    points = points_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3000, 1300));
    rec = U_EMRSETPOLYFILLMODE_set(U_WINDING);                    taf(rec,et,"U_EMRSETPOLYFILLMODE_set");
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON_set(findbounds(5, points, 0), 5, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3300, 1300));
    rec = U_EMRSETPOLYFILLMODE_set(U_ALTERNATE);                  taf(rec,et,"U_EMRSETPOLYFILLMODE_set");
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON_set(findbounds(5, points, 0), 5, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    // various types of draw

    cr = colorref_set(255, 0, 0);
    elp = extlogpen_set(U_PS_SOLID|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|U_PS_GEOMETRIC, 10, U_BS_SOLID, cr, U_HS_HORIZONTAL,0,NULL);
    rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );       taf(rec,et,"emrextcreatepen_set");
    rec = selectobject_set(pen, eht);                              taf(rec,et,"selectobject_set");   // make this pen active
    free(elp);

    points = points_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 100, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYBEZIER_set(U_RCL_DEF, 7, points);              taf(rec,et,"U_EMRPOLYBEZIER_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 600, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON_set(findbounds(6, points, 0), 6, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);
 
    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1100, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYLINE_set(findbounds(6, points, 0), 6, points); taf(rec,et,"U_EMRPOLYLINE_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1600, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(1600,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO_set(U_RCL_DEF, 6, points);            taf(rec,et,"U_EMRPOLYBEZIERTO_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 2100, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(2100,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO_set(U_RCL_DEF, 6, points);              taf(rec,et,"U_EMRPOLYLINETO_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    rec = U_EMRABORTPATH_set();                                   taf(rec,et,"U_EMRABORTPATH_set");

    // same without begin/end path

    points = points_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3100, 1800));
    rec = U_EMRPOLYBEZIER_set(U_RCL_DEF, 7, points);              taf(rec,et,"U_EMRPOLYBEZIER_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3600, 1800));
    rec = U_EMRPOLYGON_set(findbounds(6, points, 0), 6, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);
 
    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4100, 1800));
    rec = U_EMRPOLYLINE_set(findbounds(6, points, 0), 6, points); taf(rec,et,"U_EMRPOLYLINE_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4600, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(4600,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO_set(U_RCL_DEF, 6, points);            taf(rec,et,"U_EMRPOLYBEZIERTO_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 5100, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(5100,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO_set(U_RCL_DEF, 6, points);              taf(rec,et,"U_EMRPOLYLINETO_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    // same without begin/end path or strokefillpath

    points = points_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6100, 1800));
    rec = U_EMRPOLYBEZIER_set(U_RCL_DEF, 7, points);              taf(rec,et,"U_EMRPOLYBEZIER_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6600, 1800));
    rec = U_EMRPOLYGON_set(findbounds(6, points, 0), 6, points);  taf(rec,et,"U_EMRPOLYGON_set");
    free(points);
 
    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7100, 1800));
    rec = U_EMRPOLYLINE_set(findbounds(6, points, 0), 6, points); taf(rec,et,"U_EMRPOLYLINE_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7600, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(7600,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO_set(U_RCL_DEF, 6, points);            taf(rec,et,"U_EMRPOLYBEZIERTO_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 8100, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(8100,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO_set(U_RCL_DEF, 6, points);              taf(rec,et,"U_EMRPOLYLINETO_set");
    free(points);

    // 16 bit versions of the preceding
 

    point16 = point16_transform(plarray16, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 100, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYBEZIER16_set(U_RCL_DEF, 7, point16);           taf(rec,et,"U_EMRPOLYBEZIER16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0,  600, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON16_set(findbounds16(6, point16, 0), 6, point16);  
                                                                  taf(rec,et,"U_EMRPOLYGON16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);
 
    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1100, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYLINE16_set(findbounds16(6, point16, 0), 6, point16); 
                                                                  taf(rec,et,"U_EMRPOLYLINE16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1600, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(1600,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO16_set(U_RCL_DEF, 6, point16);         taf(rec,et,"U_EMRPOLYBEZIERTO16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 2100, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(2100,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO16_set(U_RCL_DEF, 6, point16);           taf(rec,et,"U_EMRPOLYLINETO16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);


    // same but without begin/end path

    point16 = point16_transform(plarray16, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3100, 2300));
    rec = U_EMRPOLYBEZIER16_set(U_RCL_DEF, 7, point16);           taf(rec,et,"U_EMRPOLYBEZIER16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3600, 2300));
    rec = U_EMRPOLYGON16_set(findbounds16(6, point16, 0), 6, point16);  
                                                                  taf(rec,et,"U_EMRPOLYGON16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);
 
    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4100, 2300));
    rec = U_EMRPOLYLINE16_set(findbounds16(6, point16, 0), 6, point16); 
                                                                  taf(rec,et,"U_EMRPOLYLINE16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4600, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(4600,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO16_set(U_RCL_DEF, 6, point16);         taf(rec,et,"U_EMRPOLYBEZIERTO16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 5100, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(5100,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO16_set(U_RCL_DEF, 6, point16);           taf(rec,et,"U_EMRPOLYLINETO16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    // same but without begin/end path or strokeandfillpath

    point16 = point16_transform(plarray16, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6100, 2300));
    rec = U_EMRPOLYBEZIER16_set(U_RCL_DEF, 7, point16);           taf(rec,et,"U_EMRPOLYBEZIER16_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6600, 2300));
    rec = U_EMRPOLYGON16_set(findbounds16(6, point16, 0), 6, point16);  
                                                                  taf(rec,et,"U_EMRPOLYGON16_set");
    free(point16);
 
    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7100, 2300));
    rec = U_EMRPOLYLINE16_set(findbounds16(6, point16, 0), 6, point16); 
                                                                  taf(rec,et,"U_EMRPOLYLINE16_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7600, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(7600,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO16_set(U_RCL_DEF, 6, point16);         taf(rec,et,"U_EMRPOLYBEZIERTO16_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 8100, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(8100,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO16_set(U_RCL_DEF, 6, point16);           taf(rec,et,"U_EMRPOLYLINETO16_set");
    free(point16);


     /* ********************************************************************** */
    // test transform routines, first generate a circle of points

    for(i=0; i<12; i++){
       pl1     = point_set(1,0);
       points  = points_transform(&pl1,1,xform_alt_set(100.0, 1.0, (float)(i*30), 0.0, 0.0, 0.0));
       pl12[i] = *points;
       free(points);
    }
    pl12[0].x = U_ROUND((float)pl12[0].x) * 1.5; // make one points stick out a bit

    // test scale (range 1->2) and axis ratio (range 1->0)
    for(i=0; i<12; i++){
       points = points_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 0.0, 30.0*((float)i), 200 + i*300, 2800));
       rec = U_EMRBEGINPATH_set();                                     taf(rec,et,"U_EMRBEGINPATH_set");
       rec = U_EMRPOLYGON_set(findbounds(12, points, 0), 12, points);  taf(rec,et,"U_EMRPOLYGON_set");
       rec = U_EMRENDPATH_set();                                       taf(rec,et,"U_EMRENDPATH_set");
       rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                     taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
       free(points);
    }

    // test scale (range 1->2) and axis ratio (range 1->0), rotate major axis to vertical (point lies on positive Y axis)
    for(i=0; i<12; i++){
       points = points_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 90.0, 30.0*((float)i), 200 + i*300, 3300));
       rec = U_EMRBEGINPATH_set();                                     taf(rec,et,"U_EMRBEGINPATH_set");
       rec = U_EMRPOLYGON_set(findbounds(12, points, 0), 12, points);  taf(rec,et,"U_EMRPOLYGON_set");
       rec = U_EMRENDPATH_set();                                       taf(rec,et,"U_EMRENDPATH_set");
       rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                     taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
       free(points);
    }
    
    // test polypolyline and polypolygon using this same array.

    plc[0]=3;
    plc[1]=4;
    plc[2]=5;

    points  = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4000, 2800));
    rec = U_EMRPOLYPOLYLINE_set(findbounds(12, points, 0), 3, plc, 12, points);
    taf(rec,et,"U_EMRPOLYPOLYLINE_set");
    free(points);

    points  = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4000, 3300));
    point16 = point_to_point16(points, 12);
    rec = U_EMRPOLYPOLYLINE16_set(findbounds16(12, point16, 0), 3, plc, 12, point16);
    taf(rec,et,"U_EMRPOLYPOLYLINE16_set");
    free(point16);
    free(points);

    points = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4300, 2800));
    rec = U_EMRPOLYPOLYGON_set(findbounds(12, points, 0), 3, plc, 12, points);
    taf(rec,et,"U_EMRPOLYPOLYGON_set");
    free(points);

    points  = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4300, 3300));
    point16 = point_to_point16(points, 12);
    rec = U_EMRPOLYPOLYGON16_set(findbounds16(12, point16, 0), 3, plc, 12, point16);
    taf(rec,et,"U_EMRPOLYPOLYGON16_set");
    free(point16);
    free(points);

    plc[0]=3;
    plc[1]=3;
    plc[2]=3;
    plc[3]=3;

    points = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4600, 2800));
    rec = U_EMRPOLYPOLYLINE_set(findbounds(12, points, 0), 4, plc, 12, points);
    taf(rec,et,"U_EMRPOLYPOLYLINE_set");
    free(points);

    points  = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4600, 3300));
    point16 = point_to_point16(points, 12);
    rec = U_EMRPOLYPOLYLINE16_set(findbounds16(12, point16, 0), 4, plc, 12, point16);
    taf(rec,et,"U_EMRPOLYPOLYLINE16_set");
    free(point16);
    free(points);

    points = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4900, 2800));
    rec = U_EMRPOLYPOLYGON_set(findbounds(12, points, 0), 4, plc, 12, points);
    taf(rec,et,"U_EMRPOLYPOLYGON_set");
    free(points);

    points  = points_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4900, 3300));
    point16 = point_to_point16(points, 12);
    rec = U_EMRPOLYPOLYGON16_set(findbounds16(12, point16, 0), 4, plc, 12, point16);
    taf(rec,et,"U_EMRPOLYPOLYGON16_set");
    free(point16);
    free(points);

    PU_TRIVERTEX         tvs;
    U_TRIVERTEX          tvrect[5]    = {{0,0,0xFFFF,0,0,0}, {100,100,0,0xFFFF,0,0}, {200,200,0,0,0xFFFF,0}, 
                                         {300,100,0xFFFF,0xFFFF,0,0}, {300,200,0x7FFF,0x7FFF,0x7FFF,0}};
    U_GRADIENT4          grrect[4]    = {{0,4},{0,1},{1,2},{2,3}};

    U_TRIVERTEX          tvtrig[3]    = {{0,0,0xFFFF,0,0,0}, {50,100,0,0,0xFFFF,0}, {100,0,0,0xFFFF,0,0}};
    U_GRADIENT3          grtrig[1]    = {{0,1,2}};

    U_TRIVERTEX          tvtrig4[4]   = {{0,0,0xFFFF,0,0,0}, {0,100,0xFFFF,0,0,0}, {100,100,0,0xFFFF,0,0}, {100,00,0,0xFFFF,0,0}};
    U_GRADIENT3          grrect3[2]   = {{0,1,2},{0,2,3}};

    /*  gradientfill
        It appears that when a record contains multiple gradients they are drawn in order with later ones
        overwriting overlapping earlier ones.
    */
    int nv=5;
    int ng=4;

    tvs = trivertex_transform(tvrect, nv, xform_alt_set(2.0, 1.0, 0.0, 0.0, 400, 4200));
    rec = U_EMRGRADIENTFILL_set(U_RCL_DEF, nv, ng, U_GRADIENT_FILL_RECT_V, tvs, (uint32_t *) grrect );
    taf(rec,et,"U_EMRGRADIENTFILL_set");
    free(tvs);

    tvs = trivertex_transform(tvrect, nv, xform_alt_set(2.0, 1.0, 0.0, 0.0, 400, 4650));
    rec = U_EMRGRADIENTFILL_set(U_RCL_DEF, nv, ng, U_GRADIENT_FILL_RECT_H, tvs, (uint32_t *) grrect );
    taf(rec,et,"U_EMRGRADIENTFILL_set");
    free(tvs);

    /* this one works and does not poison the EMF file */
    nv=3;
    ng=1;
    tvs = trivertex_transform(tvtrig, nv, xform_alt_set(2.0, 1.0, 0.0, 0.0, 100, 4200));
    rec = U_EMRGRADIENTFILL_set(U_RCL_DEF, nv, ng, U_GRADIENT_FILL_TRIANGLE, tvs, (uint32_t *) grtrig );
    taf(rec,et,"U_EMRGRADIENTFILL_set");
    free(tvs);

    nv=4;
    ng=2;
    tvs = trivertex_transform(tvtrig4, nv, xform_alt_set(2.0, 1.0, 0.0, 0.0, 100, 4650));
    rec = U_EMRGRADIENTFILL_set(U_RCL_DEF, nv, ng, U_GRADIENT_FILL_TRIANGLE, tvs, (uint32_t *) grrect3 );
    taf(rec,et,"U_EMRGRADIENTFILL_set");
    free(tvs);

    /* ********************************************************************** */
    // line types

 
    pl12[0] = point_set(0,   0  );
    pl12[1] = point_set(200, 200);
    pl12[2] = point_set(0,   400);
    pl12[3] = point_set(2,   1 ); // these next two are actually used as the pattern for U_PS_USERSTYLE
    pl12[4] = point_set(4,   3 ); // dash =2, gap=1, dash=4, gap =3
    if(!(mode & PPT_BLOCKERS)){
       for(i=0; i<=U_PS_DASHDOTDOT; i++){
          rec = selectobject_set(U_BLACK_PEN, eht); // make pen a stock object
          taf(rec,et,"deleteobject_set");
          if(pen){
             rec = deleteobject_set(&pen, eht); // delete current (custom) pen
             taf(rec,et,"deleteobject_set");
          }

          rec = createpen_set(&pen, eht,        
                   logpen_set(i, point_set(4+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|i, point_set(1,1),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|i, point_set(2+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_GEOMETRIC|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|i, point_set(2+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_GEOMETRIC | i, point_set(2+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_ENDCAP_SQUARE|i, point_set(2+i,0),colorref_set(255, 0, 255))
                );
          taf(rec,et,"createpen_set");

          rec = selectobject_set(pen, eht); // make pen just created active
          taf(rec,et,"selectobject_set");

          points = points_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
          rec = U_EMRPOLYLINE_set(findbounds(3, points, 0), 3, points); taf(rec,et,"U_EMRPOLYLINE_set");
          free(points);
       }

       rec = selectobject_set(U_BLACK_PEN, eht);  taf(rec,et,"selectobject_set");
       if(pen){
          rec = deleteobject_set(&pen, eht); // delete current (custom) pen
          taf(rec,et,"deleteobject_set");
       }


       // if width isn't 1 in the following then it is drawn solid, at width==1 anyway.
       elp = extlogpen_set(U_PS_USERSTYLE | U_PS_COSMETIC, 1, U_BS_SOLID, colorref_set(0, 0, 255), U_HS_HORIZONTAL,4,(uint32_t *)&(pl12[3]));
       rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
       free(elp);

       rec = selectobject_set(pen, eht); // make pen just created active
       taf(rec,et,"selectobject_set");

       points = points_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
       rec = U_EMRPOLYLINE_set(findbounds(3, points, 0), 3, points); taf(rec,et,"U_EMRPOLYLINE_set");
       free(points);

       rec = selectobject_set(U_BLACK_PEN, eht); // make pen a stock object
       taf(rec,et,"selectobject_set");
       if(pen){
          rec = deleteobject_set(&pen, eht); // delete current (custom) pen
          taf(rec,et,"deleteobject_set");
       }
       i++;

       // if width isn't 1 in the following then it is drawn solid, at width==1 anyway.
       elp = extlogpen_set(U_PS_USERSTYLE|U_PS_GEOMETRIC|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER, 4, U_BS_SOLID, colorref_set(255, 0, 0), U_HS_HORIZONTAL,4,(uint32_t *)&(pl12[3]));
       rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
       free(elp);
       rec = selectobject_set(pen, eht);   taf(rec,et,"selectobject_set");


       points = points_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
       rec = U_EMRPOLYLINE_set(findbounds(3, points, 0), 3, points); taf(rec,et,"U_EMRPOLYLINE_set");
       free(points);

       rec = selectobject_set(U_BLACK_PEN, eht);    taf(rec,et,"selectobject_set");
       if(pen){
          rec = deleteobject_set(&pen, eht); // delete current (custom) pen
          taf(rec,et,"deleteobject_set");
       }


       /* run over all combinations of cap(join(miter 1,5))), but draw as solid lines with */
       pl12[0] = point_set(0,   0  );
       pl12[1] = point_set(600, 200);
       pl12[2] = point_set(0,   400);
       for (cap=U_PS_ENDCAP_ROUND; cap<= U_PS_ENDCAP_FLAT; cap+= U_PS_ENDCAP_SQUARE){
          for(join=U_PS_JOIN_ROUND; join<=U_PS_JOIN_MITER; join+= U_PS_JOIN_BEVEL){
             for(miter=1;miter<=5;miter+=4){
                rec = U_EMRSETMITERLIMIT_set(miter); taf(rec,et,"U_EMRSETMITERLIMIT_set");

                elp = extlogpen_set(U_PS_SOLID| U_PS_GEOMETRIC | cap | join, 20, U_BS_SOLID, colorref_set(64*(5-miter), 0, 0), U_HS_HORIZONTAL,0,NULL);
                rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
                free(elp);

                rec = selectobject_set(pen, eht);  taf(rec,et,"selectobject_set");

                points = points_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 500 + (cap/U_PS_ENDCAP_SQUARE)*250 + miter*20, 5200 + (join/U_PS_JOIN_BEVEL)*450));
                rec = U_EMRPOLYLINE_set(findbounds(3, points, 0), 3, points); taf(rec,et,"U_EMRPOLYLINE_set");
                free(points);
                rec = deleteobject_set(&pen, eht);  taf(rec,et,"deleteobject_set");
             }
          }
       }
       rec = selectobject_set(U_BLACK_PEN, eht);    taf(rec,et,"selectobject_set");
       rec = U_EMRSETMITERLIMIT_set(8);             taf(rec,et,"U_EMRSETMITERLIMIT_set");



    } //PPT_BLOCKERS

    /* ********************************************************************** */
    // bitmaps
    
    offset = 5000;
    label_column(offset, 5000, &font, et, eht);
    label_row(offset + 400, 5000 - 30, &font, et,  eht);
    offset += 400;
   
    // Make the first test image, it is 10 x 10 and has various colors, R,G,B in each of 3 corners
    rgba_px = (char *) malloc(10*10*4);
    FillImage(rgba_px,10,10,40);

    rec =  U_EMRSETSTRETCHBLTMODE_set(U_STRETCH_DELETESCANS);
    taf(rec,et,"U_EMRSETSTRETCHBLTMODE_set");

    colortype = U_BCBM_COLOR32;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR32\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset ,5000,10,10,Bmi, cbPx, px);
    offset += 220;

    // we are going to step on this one with little rectangles using different binary raster operations
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(2900,5000),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");

    free(Bmi);
    free(px);


    colortype = U_BCBM_COLOR24;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 1))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR24\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, NULL);
    image_column(et, offset ,5000,10,10,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);
    free(px);

    // 16 bit, 5 bits per color, no table
    colortype = U_BCBM_COLOR16;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4,2))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR16\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset ,5000,10,10,Bmi, cbPx, px);
    offset += 220;
       
    // write a second copy next to it using the negative height method to indicate it should be upside down
    Bmi->bmiHeader.biHeight *= -1;
    image_column(et, offset ,5000,10,10,Bmi, cbPx, px);
    offset += 220;

    free(Bmi);
    free(px);


    colortype = U_BCBM_COLOR8;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR8\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset ,5000,10,10,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);

    // also test the numCt==0 form
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, 0, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset + 660 ,5000,10,10,Bmi, cbPx, px);
    free(Bmi);

    free(ct);
    free(px);

    // done with the first test image, make the 2nd

    free(rgba_px); 
    rgba_px = (char *) malloc(4*4*4);
    FillImage(rgba_px,4,4,16);

    colortype = U_BCBM_COLOR4;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 4, 4,     colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
    if(rgba_diff(rgba_px, rgba_px2, 4 * 4 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR4\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset ,5000,4,4,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);

    // also test the numCt==0 form
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, 0, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset + 660 ,5000,4,4,Bmi, cbPx, px);
    free(Bmi);

    free(ct);
    free(px);

    // make a two color image in the existing RGBA array
    memset(rgba_px, 0x55, 4*4*4);
    memset(rgba_px, 0xAA, 4*4*2);

    colortype = U_BCBM_MONOCHROME;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 4, 4,     colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
    if(rgba_diff(rgba_px, rgba_px2, 4 * 4 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_MONOCHROME\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset ,5000,4,4,Bmi, cbPx, px);
    offset += 220;

    // write a second copy next to it using the negative height method to indicate it should be upside down
    Bmi->bmiHeader.biHeight *= -1;
    image_column(et, offset ,5000,4,4,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);

    // also test the numCt==0 form
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, 0, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(et, offset + 660 - 220 ,5000,4,4,Bmi, cbPx, px);

    free(Bmi);
    free(ct);
    free(px);

    free(rgba_px); 

    // Make another sacrificial image, this one the inverse of the background color
    rec =  U_EMRBITBLT_set(
      U_RCL_DEF,
      pointl_set(2900,7020),
      pointl_set(2000,2000),
      pointl_set(5000,5000),
      xform_set(1.0, 0.0, 0.0, 1.0, 0.0, 0.0),
      colorref_set(0,0,0),
      U_DIB_RGB_COLORS,
      U_DSTINVERT,
      NULL,
      0,
      NULL);
    taf(rec,et,"U_EMRBITBLT_set");

if(!(mode & PPT_BLOCKERS)){
    /* Screen display in GDI apparently does not support either the PNG or JPG mode, some printers may. 
    When these are used the images will not be visible in Windows XP Preview and they will prevent the EMF
    from being ungrouped within PowerPoint 2003. */

    textlabel(40, "STRETCHDIBITS", 5000, 8000, &font, et, eht);

    px   = (char *) &pngarray[0];
    cbPx = 138; 
    Bmih = bitmapinfoheader_set(10, -10, 1, U_BCBM_EXPLICIT, U_BI_PNG, cbPx, 0, 0, 0, 0); /* PNG, fields 5 and 6 must be present! */
    Bmi  = bitmapinfo_set(Bmih, NULL);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(5400,8000),
       pointl_set(200,200),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    free(Bmi);
    textlabel(30, "PNG", 5400, 8000-30, &font, et, eht);

    px   = (char *) &jpgarray[0];
    cbPx = 676; 
    Bmih = bitmapinfoheader_set(10, -10, 1, U_BCBM_EXPLICIT, U_BI_JPEG, cbPx, 0, 0, 0, 0); /* JPG, fields 5 and 6 must be present! */
    Bmi  = bitmapinfo_set(Bmih, NULL);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(5620,8000),
       pointl_set(200,200),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    free(Bmi);
    textlabel(30, "JPG", 5620, 8000-30, &font, et, eht);

} //PPT_BLOCKERS

    // testing binary raster operations
    // make a series of rectangle draws with grey rectangles under various binary raster operations
    for(i=1;i<=16;i++){
       rec = U_EMRSETROP2_set(i);                                                taf(rec,et,"U_EMRSETROP2_set");
       ul     = pointl_set(2800 + i*100,5000);
       lr     = pointl_set(2800 + i*100+90,9019);
       rec = U_EMRRECTANGLE_set(rectl_set(ul,lr));                               taf(rec,et,"U_EMRRECTANGLE_set");
    }
    // make a series of white line draws under various binary raster operations
    rec = selectobject_set(U_WHITE_PEN, eht);
    taf(rec,et,"selectobject_set");
    for(i=1;i<=16;i++){
       rec = U_EMRSETROP2_set(i);                                                taf(rec,et,"U_EMRSETROP2_set");
       rec = U_EMRMOVETOEX_set(pointl_set(4520 + i*10,4950));                    taf(rec,et,"U_EMRMOVETOEX_set");
       rec = U_EMRLINETO_set(  pointl_set(4520 + i*10,9069));                    taf(rec,et,"U_EMRLINETO_set");
    }
    // make a series of black line draws under various binary raster operations, restore previous pen value
    rec = selectobject_set(U_BLACK_PEN, eht);
    taf(rec,et,"selectobject_set");
    for(i=1;i<=16;i++){
       rec = U_EMRSETROP2_set(i);                                                taf(rec,et,"U_EMRSETROP2_set");
       rec = U_EMRMOVETOEX_set(pointl_set(4720 + i*10,4950));                    taf(rec,et,"U_EMRMOVETOEX_set");
       rec = U_EMRLINETO_set(  pointl_set(4720 + i*10,9069));                    taf(rec,et,"U_EMRLINETO_set");
    }
    //restore the previous defaults
    rec = U_EMRSETROP2_set(U_R2_COPYPEN);
    taf(rec,et,"U_EMRSETROP2_set");


    FontName = U_Utf8ToUtf16le("Arial", 0, NULL);  // Helvetica originally, but that does not work
    lf   = logfont_set( -300, 0, 0, 0, 
                      U_FW_BOLD, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
    FontStyle = U_Utf8ToUtf16le("Bold", 0, NULL);
    elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
    free(FontName);
    free(FontStyle);
    rec  = extcreatefontindirectw_set(&font, eht,  NULL, (char *) &elfw);
    taf(rec,et,"extcreatefontindirectw_set");

    rec  = selectobject_set(font, eht);
    taf(rec,et,"selectobject_set");

    rec = U_EMRSETTEXTCOLOR_set(colorref_set(255,0,0));
    taf(rec,et,"U_EMRSETTEXTCOLOR_set");

    // On XP this apparently ignores the active font UNLESS it is followed by a U_EMREXTTEXTOUTA/W.  Strange.
    // PowerPoint 2003 drops this on input from EMF in any case
    
    string = U_strdup("Text8 from U_EMRSMALLTEXTOUT");
    slen = strlen(string);
    rec = U_EMRSMALLTEXTOUT_set( pointl_set(100,50),  slen,  U_ETO_SMALL_CHARS,  
       U_GM_COMPATIBLE,  1.0, 1.0,  U_RCL_DEF, string);
    taf(rec,et,"U_EMRSMALLTEXTOUT_set");
    free(string);

    rec = U_EMRSETTEXTCOLOR_set(colorref_set(0, 255,0));
    taf(rec,et,"U_EMRSETTEXTCOLOR_set");

    text16 = U_Utf8ToUtf16le("Text16 from U_EMRSMALLTEXTOUT", 0, NULL);
    slen   = wchar16len(text16);
    rec = U_EMRSMALLTEXTOUT_set( pointl_set(100,350),  slen,  U_ETO_NONE,
       U_GM_COMPATIBLE,  1.0, 1.0,  U_RCL_DEF, (char *) text16);
    taf(rec,et,"U_EMRSMALLTEXTOUT_set");
    free(text16);

    rec = U_EMRSETTEXTCOLOR_set(colorref_set(0,0,255));
    taf(rec,et,"U_EMRSETTEXTCOLOR_set");

    string = U_strdup("Text8 from U_EMREXTTEXTOUTA");
    slen = strlen(string);
    dx = dx_set(-300,  U_FW_NORMAL, slen);
    rec2 = emrtext_set( pointl_set(100,650), slen, 1, string, U_ETO_NONE, (U_RECTL){0,0,-1,-1}, dx);
    rec = U_EMREXTTEXTOUTA_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTA_set");
    free(dx);
    free(rec2);
    free(string);

    // In some monochrome modes these are used.  0 gets the bk color and !0 gets the text color.
    // Make these distinctive colors so it is obvious where it is happening.  Background is "Aqua", foreground is "Fuchsia"
    rec = U_EMRSETTEXTCOLOR_set(colorref_set(255,0,255));    taf(rec,et,"U_EMRSETTEXTCOLOR_set");
    rec = U_EMRSETBKCOLOR_set(colorref_set(0,255,255));      taf(rec,et,"U_EMRSETBKCOLOR_set");

    text16 = U_Utf8ToUtf16le("Text16 from U_EMREXTTEXTOUTW", 0, NULL);
    slen   = wchar16len(text16);
    dx = dx_set(-300,  U_FW_NORMAL, slen);
    rec2 = emrtext_set( pointl_set(100,950), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
    free(text16);
    rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTW_set");
    free(dx);
    free(rec2);

    /* ***************    test all text alignments  *************** */
    /* WNF documentation (section 2.1.2.3) says that the bounding rectangle should supply edges which are used as
       reference points.  This appears to not be true.  The following example draws two sets of aligned text,
       with the bounding rectangles indicated with a grey rectangle, using two different size bounding rectangles,
       and the text is in the same position in both cases.
       
       EMF documentation (section 2.2.5) says that the same rectangle is used for "clipping or opaquing" by ExtTextOutA/W.
       That does not seem to occur either.
    */
    
    // get rid of big font
    if(font){
       rec  = deleteobject_set(&font, eht);
       taf(rec,et,"deleteobject_set");
    }

    // Use a smaller font for text alignment tests.  Note that if the target system does
    // not have "Courier New" it will use some default font, which will not look right
    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    lf   = logfont_set( -30, 0, 0, 0, 
                      U_FW_BOLD, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
    FontStyle = U_Utf8ToUtf16le("Bold", 0, NULL);
    elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
    free(FontName);
    free(FontStyle);
    rec  = extcreatefontindirectw_set(&font, eht,  NULL, (char *) &elfw);
    taf(rec,et,"extcreatefontindirectw_set");
    rec  = selectobject_set(font, eht);
    taf(rec,et,"selectobject_set");

    // use NULL_PEN (no edge on drawn rectangle)
    rec = selectobject_set(U_NULL_PEN, eht); // make stock object NULL_PEN active
    taf(rec,et,"selectobject_set");
    // get rid of current (custom) pen, if defined
    if(pen){
        rec  = deleteobject_set(&pen, eht);
        taf(rec,et,"deleteobject_set");
     }

    text16 = U_Utf8ToUtf16le("textalignment:default", 0, NULL);
    slen   = wchar16len(text16);
    dx = dx_set(-30,  U_FW_NORMAL, slen);
    rec = U_EMRRECTANGLE_set(rectl_set(pointl_set(5500-20,100-20),pointl_set(5500+20,100+20)));
    taf(rec,et,"U_EMRRECTANGLE_set");
    rec2 = emrtext_set( pointl_set(5500,100), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
    free(text16);
    rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTW_set");
    free(dx);
    free(rec2);

    string = (char *) malloc(32);
    for(i=0;i<=0x18; i+=2){
       rec = U_EMRSETTEXTALIGN_set(i);
       taf(rec,et,"U_EMRSETTEXTALIGN_set");
       sprintf(string,"textalignment:0x%2.2X",i);
       text16 = U_Utf8ToUtf16le(string, 0, NULL);
       slen   = wchar16len(text16);
       dx = dx_set(-30,  U_FW_NORMAL, slen);
       rec = U_EMRRECTANGLE_set(rectl_set(pointl_set(6000-20,100+ i*50-20),pointl_set(6000+20,100+ i*50+20)));
       taf(rec,et,"U_EMRRECTANGLE_set");
       rec2 = emrtext_set( pointl_set(6000,100+ i*50), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
       free(text16);
       rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
       taf(rec,et,"U_EMREXTTEXTOUTW_set");
       free(dx);
       free(rec2);
    }
 
    for(i=0;i<=0x18; i+=2){
       rec = U_EMRSETTEXTALIGN_set(i);
       taf(rec,et,"U_EMRSETTEXTALIGN_set");
       sprintf(string,"textalignment:0x%2.2X",i);
       text16 = U_Utf8ToUtf16le(string, 0, NULL);
       slen   = wchar16len(text16);
       dx = dx_set(-30,  U_FW_NORMAL, slen);
       rec = U_EMRRECTANGLE_set(rectl_set(pointl_set(7000-40,100+ i*50-40),pointl_set(7000+40,100+ i*50+40)));
       taf(rec,et,"U_EMRRECTANGLE_set");
       rec2 = emrtext_set( pointl_set(7000,100+ i*50), slen, 2, text16, U_ETO_NONE, rectl_set(pointl_set(7000-40,100+ i*50-40),pointl_set(7000+40,100+ i*50+40)), dx);
       free(text16);
       rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
       taf(rec,et,"U_EMREXTTEXTOUTW_set");
       free(dx);
       free(rec2);
    }
    free(string);
    
    // restore the default text alignment
    rec = U_EMRSETTEXTALIGN_set(U_TA_DEFAULT);
    taf(rec,et,"U_EMRSETTEXTALIGN_set");
    
    /* ***************    test rotated text  *************** */
    
    spintext(8000, 300,U_TA_BASELINE,&font,et,eht);
    spintext(8600, 300,U_TA_TOP,     &font,et,eht);
    spintext(9200, 300,U_TA_BOTTOM,  &font,et,eht);
    rec = U_EMRSETTEXTALIGN_set(U_TA_BASELINE); // go back to baseline
    taf(rec,et,"U_EMRSETTEXTALIGN_set");
    

    /* ***************    test hatched fill and stroke (standard patterns)  *************** */
    
    // use BLACK_PEN (edge on drawn rectangle)
    rec = selectobject_set(U_BLACK_PEN, eht);    taf(rec,et,"selectobject_set");
    // get rid of current (custom) pen, if defined
    if(pen){ rec  = deleteobject_set(&pen, eht); taf(rec,et,"deleteobject_set"); }
    cr = colorref_set(63, 127, 255);
    ul = pointl_set(0,0);
    lr = pointl_set(300,300);
    rclBox = rectl_set(ul,lr);

    /* *** fill *** */
    for(i=0;i<=U_HS_DITHEREDBKCLR;i++){
      if(brush){
         rec = deleteobject_set(&brush, eht); // disable brush which is at handle 1, it should not be selected when this happens!!!
         taf(rec,et,"deleteobject_set");
      }
      lb = logbrush_set(U_BS_HATCHED, cr, i);
      rec = createbrushindirect_set(&brush, eht,lb); taf(rec,et,"createbrushindirect_set");
      rec = selectobject_set(brush, eht);            taf(rec,et,"selectobject_set");
      points = points_transform((PU_POINT) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 3500));
      rec = U_EMRRECTANGLE_set(*(U_RECTL*)points); taf(rec,et,"U_EMRRECTANGLE_set");
      free(points);
    }

    rec = selectobject_set(U_NULL_BRUSH, eht);    taf(rec,et,"selectobject_set");
    rec  = deleteobject_set(&brush, eht);         taf(rec,et,"deleteobject_set");

    /* *** stroke *** */
    for(i=0; i<=U_HS_DITHEREDBKCLR; i++){
      if(pen){ rec = deleteobject_set(&pen, eht);  taf(rec,et,"deleteobject_set"); }
      elp = extlogpen_set(U_PS_GEOMETRIC|U_PS_ENDCAP_FLAT, 300, U_BS_HATCHED, cr, i, 0, NULL);
      rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
      free(elp);

      rec = selectobject_set(pen, eht);    taf(rec,et,"selectobject_set");

      rec = U_EMRMOVETOEX_set(pointl_set(6300+i*330, 3600));               taf(rec,et,"U_EMRMOVETOEX_set");
      rec = U_EMRLINETO_set(pointl_set(6300+i*330, 4100));                 taf(rec,et,"U_EMRLINETO_set");
    }
    if(pen){ rec = deleteobject_set(&pen, eht);  taf(rec,et,"deleteobject_set"); }


    //repeat with red background, green text

    rec = U_EMRSETBKCOLOR_set(colorref_set(255, 0, 0));  taf(rec,et,"U_EMRSETBKCOLOR_set");
    rec = U_EMRSETTEXTCOLOR_set(colorref_set(0,255,0));  taf(rec,et,"U_EMRSETTEXTCOLOR_set");
    rec = selectobject_set(U_BLACK_PEN, eht);            taf(rec,et,"selectobject_set");
    if(pen){ rec  = deleteobject_set(&pen, eht);         taf(rec,et,"deleteobject_set"); }
    for(i=0;i<=U_HS_DITHEREDBKCLR;i++){
       if(brush){ rec = deleteobject_set(&brush, eht);  taf(rec,et,"deleteobject_set"); }
       lb = logbrush_set(U_BS_HATCHED, cr, i);
       rec = createbrushindirect_set(&brush, eht,lb); taf(rec,et,"createbrushindirect_set");
       rec = selectobject_set(brush, eht);            taf(rec,et,"selectobject_set");
       points = points_transform((PU_POINT) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 3830));
       rec = U_EMRRECTANGLE_set(*(U_RECTL*)points); taf(rec,et,"U_EMRRECTANGLE_set");
       free(points);
    }
    
    // restore original settings
    rec = U_EMRSETTEXTCOLOR_set(colorref_set(255,0,255));    taf(rec,et,"U_EMRSETTEXTCOLOR_set");
    rec = U_EMRSETBKCOLOR_set(colorref_set(0,255,255));       taf(rec,et,"U_EMRSETBKCOLOR_set");

    
    /* ***************    test image fill and stroke  *************** */
     
    // this will draw a series of squares of increasing size, all with the same color fill pattern  
    
    // Make the first test image, it is 5 x 5 and has various colors, R,G,B in each of 3 corners
    rgba_px = (char *) malloc(5*5*4);
    FillImage(rgba_px,5,5,20);

    colortype = U_BCBM_COLOR32;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  5, 5, 20, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    Bmih = bitmapinfoheader_set(5, 5, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    if(brush){ rec = deleteobject_set(&brush, eht);  taf(rec,et,"deleteobject_set"); }
    rec = createdibpatternbrushpt_set(&brush, eht, U_DIB_RGB_COLORS, Bmi, cbPx, px);
    taf(rec,et,"createdibpatternbrushpt_set");
    rec = selectobject_set(brush, eht);              taf(rec,et,"selectobject_set");

    ul = pointl_set(0,0);
    for(i=1;i<=10;i++){
      lr = pointl_set(30*i,30*i);
      rclBox = rectl_set(ul,lr);
      points = points_transform((PU_POINT) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4160));
      rec = U_EMRRECTANGLE_set(*(U_RECTL*)points); taf(rec,et,"U_EMRRECTANGLE_set");
      free(points);
    }

    /* also with createmonobrush_set on the same image
       Near as I can tell GDI acts as if this record was just a plain textColor brush no matter what is in the
       bitmap when the bitmap type is U_BCBM_COLOR32, this was true when pixels were specifically set to white,
       black, and a variety of other colors.
       So createmonobrush only does something useful when it is given a  monochrome bitmap.
    */

    if(brush){ rec = deleteobject_set(&brush, eht);  taf(rec,et,"deleteobject_set"); }
    rec = createmonobrush_set(&brush, eht, U_DIB_RGB_COLORS, Bmi, cbPx, px);
    taf(rec,et,"createmonobrush_set");
    rec = selectobject_set(brush, eht);              taf(rec,et,"selectobject_set");

    lr = pointl_set(30*i,30*i);
    rclBox = rectl_set(ul,lr);
    points = points_transform((PU_POINT) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4160));
    rec = U_EMRRECTANGLE_set(*(U_RECTL*)points); taf(rec,et,"U_EMRRECTANGLE_set");
    free(points);

    /* *** stroke *** */

    rec = selectobject_set(U_NULL_BRUSH, eht);    taf(rec,et,"selectobject_set");
    rec = deleteobject_set(&brush, eht);          taf(rec,et,"deleteobject_set");

    for(i=k=0;i<3;i++){
       if(     i==0){ elpBrushStyle = U_BS_DIBPATTERN;   } // Imports into PPT correctly.
       else if(i==1){ elpBrushStyle = U_BS_DIBPATTERNPT; } // Imports into PPT correctly.
       else if(i==2){ elpBrushStyle = U_BS_PATTERN;      } // Displays as Solid using TextColor
       for(j=0;j<2;j++,k++){
          if(j==0){    // solid line works
             elpNumEntries = 0;
             elpStyleEntry = NULL;
          }
          else {       // dashed line does not, nothing is drawn
             elpNumEntries = 4;
             elpStyleEntry = (uint32_t *)&(pl12[3]);
          }
          rec = selectobject_set(U_BLACK_PEN, eht);     taf(rec,et,"selectobject_set");
          if(pen){  rec = deleteobject_set(&pen, eht);  taf(rec,et,"deleteobject_set"); }
          rec = U_EMRMOVETOEX_set(pointl_set(6600+k*330, 4150));               taf(rec,et,"U_EMRMOVETOEX_set");
          rec = U_EMRLINETO_set(pointl_set(6600+k*330, 4870));                 taf(rec,et,"U_EMRLINETO_set");

          // NOTE, the 0,0,0 color is actually used to set U_DIB_RGB_COLORS.  (The types don't match.).  U_HS_HORIZONTAL is ignored.
          elp = extlogpen_set(U_PS_GEOMETRIC|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER, 150, elpBrushStyle, colorref_set(0, 0, 0), U_HS_HORIZONTAL,elpNumEntries,elpStyleEntry);
          rec = extcreatepen_set(&pen, eht,  Bmi, cbPx, px, elp );    taf(rec,et,"emrextcreatepen_set");
          free(elp);

          rec = selectobject_set(pen, eht);   taf(rec,et,"selectobject_set");
          rec = U_EMRMOVETOEX_set(pointl_set(6600+k*330, 4260));               taf(rec,et,"U_EMRMOVETOEX_set");
          rec = U_EMRLINETO_set(pointl_set(6600+k*330, 4760));                 taf(rec,et,"U_EMRLINETO_set");

       }
    }

    free(rgba_px);
    free(px);
    free(Bmi);
    /* ***************    test mono fill  *************** */
     
    rec = selectobject_set(U_BLACK_PEN, eht);            taf(rec,et,"selectobject_set");
    if(pen){ rec  = deleteobject_set(&pen, eht);         taf(rec,et,"deleteobject_set"); }

    // this will draw a series of squares of increasing size, all with the same monobrush fill pattern  
    
    // Make the first test image, it is 4 x 4 and has various colors, R,G,B in each of 3 corners
    rgba_px = (char *) malloc(4*4*4);
    FillImage(rgba_px,4,4,16);

    // make a two color image in the existing RGBA array.  The image is B/W with a color map having the two colors
    // in the following memset.  However, the color map is ignored when these are displayed and 1 goes to textcolor, 0 to bk color.
    memset(rgba_px, 0x55, 4*4*4);
    memset(rgba_px, 0xAA, 4*4*2);
    colortype = U_BCBM_MONOCHROME;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_BGRA, U_ROW_ORDER_SAME);  // Must use color tables!
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    free(ct);
    if(brush){ rec = deleteobject_set(&brush, eht);  taf(rec,et,"deleteobject_set"); }
    rec = createmonobrush_set(&brush, eht, U_DIB_RGB_COLORS, Bmi, cbPx, px);
    taf(rec,et,"createmonobrush_set");
    rec = selectobject_set(brush, eht);              taf(rec,et,"selectobject_set");

    ul = pointl_set(0,0);
    for(i=1;i<=10;i++){
      lr = pointl_set(30*i,30*i);
      rclBox = rectl_set(ul,lr);
      points = points_transform((PU_POINT) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4520));
      rec = U_EMRRECTANGLE_set(*(U_RECTL*)points); taf(rec,et,"U_EMRRECTANGLE_set");
      free(points);
    }
    
    /* also with createdibpatternbrushpt_set on the same image */

    if(brush){ rec = deleteobject_set(&brush, eht);  taf(rec,et,"deleteobject_set"); }
    rec = createdibpatternbrushpt_set(&brush, eht, U_DIB_RGB_COLORS, Bmi, cbPx, px);
    taf(rec,et,"createmonobrush_set");
    rec = selectobject_set(brush, eht);              taf(rec,et,"selectobject_set");

    lr = pointl_set(30*i,30*i);
    rclBox = rectl_set(ul,lr);
    points = points_transform((PU_POINT) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4520));
    rec = U_EMRRECTANGLE_set(*(U_RECTL*)points); taf(rec,et,"U_EMRRECTANGLE_set");
    free(points);

    /* *** stroke *** */
    rec = selectobject_set(U_NULL_BRUSH, eht);    taf(rec,et,"selectobject_set");
    rec = deleteobject_set(&brush, eht);          taf(rec,et,"deleteobject_set");
    if(pen){  rec = deleteobject_set(&pen, eht);  taf(rec,et,"deleteobject_set"); }

    for(i=k=0;i<3;i++){
       if(     i==0){ elpBrushStyle = U_BS_DIBPATTERN;   } // This uses the color map.  Imports into PPT that way too, which is odd, because the fill doesn't import.
       else if(i==1){ elpBrushStyle = U_BS_DIBPATTERNPT; } // This uses the color map.  Imports into PPT that way too, which is odd, because the fill doesn't import.
       else if(i==2){ elpBrushStyle = U_BS_PATTERN;      } // This uses Text/Bk colors instead of grey, like the fill tests above.
       for(j=0;j<2;j++,k++){
          if(j==0){    // solid line works
             elpNumEntries = 0;
             elpStyleEntry = NULL;
          }
          else {       // dashed line does not, nothing is drawn
             elpNumEntries = 4;
             elpStyleEntry = (uint32_t *)&(pl12[3]);
          }
          rec = selectobject_set(U_BLACK_PEN, eht);     taf(rec,et,"selectobject_set");
          if(pen){  rec = deleteobject_set(&pen, eht);  taf(rec,et,"deleteobject_set"); }
          rec = U_EMRMOVETOEX_set(pointl_set(9300+k*330, 4150));               taf(rec,et,"U_EMRMOVETOEX_set");
          rec = U_EMRLINETO_set(pointl_set(9300+k*330, 4870));                 taf(rec,et,"U_EMRLINETO_set");


          // NOTE, the 0,0,0 color is actually used to set U_DIB_RGB_COLORS.  (The types don't match.).  U_HS_HORIZONTAL is ignored.
          elp = extlogpen_set(U_PS_GEOMETRIC|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER, 150, elpBrushStyle, colorref_set(0, 0, 0), U_HS_HORIZONTAL,elpNumEntries,elpStyleEntry);
          rec = extcreatepen_set(&pen, eht,  Bmi, cbPx, px, elp );    taf(rec,et,"emrextcreatepen_set");
          free(elp);

          rec = selectobject_set(pen, eht);   taf(rec,et,"selectobject_set");
          rec = U_EMRMOVETOEX_set(pointl_set(9300+k*330, 4260));               taf(rec,et,"U_EMRMOVETOEX_set");
          rec = U_EMRLINETO_set(pointl_set(9300+k*330, 4760));                 taf(rec,et,"U_EMRLINETO_set");

       }
    }

    free(rgba_px);
    free(px);
    free(Bmi);

    /* ***************    test background variants with combined fill, stroke, text  *************** */
    
    if(brush){ rec = deleteobject_set(&brush, eht);      taf(rec,et,"deleteobject_set"); }
    lb = logbrush_set(U_BS_HATCHED, colorref_set(63, 127, 255), U_HS_CROSS);
    rec = createbrushindirect_set(&brush, eht,lb);       taf(rec,et,"createbrushindirect_set");
    rec = selectobject_set(brush, eht);                  taf(rec,et,"selectobject_set");

    if(pen){ rec = deleteobject_set(&pen, eht);          taf(rec,et,"deleteobject_set"); }
    elp = extlogpen_set(U_PS_GEOMETRIC|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER, 50, U_BS_HATCHED, colorref_set(255, 63, 127), U_HS_DIAGCROSS, 0, NULL);
    rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
    rec = selectobject_set(pen, eht);                    taf(rec,et,"selectobject_set");
    free(elp);

    if(font){ rec = deleteobject_set(&font, eht);        taf(rec,et,"deleteobject_set"); }
    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    lf   = logfont_set( -60, 0, 0, 0, 
                      U_FW_BOLD, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
    FontStyle = U_Utf8ToUtf16le("Bold", 0, NULL);
    elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
    free(FontName);
    free(FontStyle);
    rec  = extcreatefontindirectw_set(&font, eht,  NULL, (char *) &elfw);      taf(rec,et,"extcreatefontindirectw_set");
    rec  = selectobject_set(font, eht);                  taf(rec,et,"selectobject_set");

    rec = U_EMRSETTEXTALIGN_set(U_TA_CENTER | U_TA_BASEBIT);           taf(rec,et,"U_EMRSETTEXTALIGN_set");
    
    offset=5000;
    for(i=0; i<2; i++){         /* both bkmode*/
       if(i){ rec = U_EMRSETBKMODE_set(U_TRANSPARENT); cbuf[0]='\0'; strcat(cbuf,"bk:- "); }
       else { rec = U_EMRSETBKMODE_set(U_OPAQUE);      cbuf[0]='\0'; strcat(cbuf,"bk:+ "); }
       taf(rec,et,"U_EMRSETBKMODE_set");

       for(j=0; j<2; j++){      /* two bkcolors         R & turQuoise */
          if(j){ rec = U_EMRSETBKCOLOR_set(colorref_set(255, 0,   0)); cbuf[5]='\0'; strcat(cbuf,"bC:R ");  }
          else { rec = U_EMRSETBKCOLOR_set(colorref_set(0, 255, 255)); cbuf[5]='\0'; strcat(cbuf,"bC:Q ");  }
          taf(rec,et,"U_EMRSETBKCOLOR_set");

          for(k=0; k<2; k++){   /* two text colors      G & Black */
             if(k){ rec = U_EMRSETTEXTCOLOR_set(colorref_set(  0, 127,   0)); cbuf[10]='\0'; strcat(cbuf,"tC:G "); }
             else { rec = U_EMRSETTEXTCOLOR_set(colorref_set(  0,   0,   0)); cbuf[10]='\0'; strcat(cbuf,"tC:K "); }
             taf(rec,et,"U_EMRSETTEXTCOLOR_set");

             draw_textrect(8000,offset,700,300,cbuf, 60, et);  
             offset += 400;  
          }
       }
    }

    // restore original settings
    rec = U_EMRSETBKMODE_set(U_TRANSPARENT);                 taf(rec,et,"U_EMRSETBKMODE_set");
    rec = U_EMRSETTEXTCOLOR_set(colorref_set(0,0,0));        taf(rec,et,"U_EMRSETTEXTCOLOR_set");
    rec = U_EMRSETBKCOLOR_set(colorref_set(0,255,255));      taf(rec,et,"U_EMRSETBKCOLOR_set");
    rec = U_EMRSETTEXTALIGN_set(U_TA_DEFAULT);               taf(rec,et,"U_EMRSETTEXTALIGN_set");

    /* ***************    test variants of background, hatch pattern, hatchcolor on start/end path  *************** */
    
    /* Note, AFAIK this is not a valid operation and no tested application can actually draw this.  Applications should at least not explode
       when they see it.  */

    offset = 5000;
    textlabel(40, "Path contains invalid operations.",   8800, offset, &font, et, eht); offset+=50;
    textlabel(40, "Any graphic produced is acceptable.", 8800, offset, &font, et, eht); offset+=50;
    textlabel(40, "Rendering program should not crash.", 8800, offset, &font, et, eht); offset+=100;
    rec = U_EMRBEGINPATH_set();                                     taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(9100, offset));              taf(rec,et,"U_EMRMOVETOEX_set");
    
    for(i=0; i<2; i++){         /* both bkmode*/
       if(i){ rec = U_EMRSETBKMODE_set(U_TRANSPARENT); cbuf[0]='\0'; strcat(cbuf,"bk:- "); }
       else { rec = U_EMRSETBKMODE_set(U_OPAQUE);      cbuf[0]='\0'; strcat(cbuf,"bk:+ "); }
       taf(rec,et,"U_EMRSETBKMODE_set");

       for(j=0; j<2; j++){      /* two bkcolors         R & turQuoise */
          if(j){ rec = U_EMRSETBKCOLOR_set(colorref_set(255, 0,   0)); cbuf[5]='\0'; strcat(cbuf,"bC:R ");  }
          else { rec = U_EMRSETBKCOLOR_set(colorref_set(0, 255, 255)); cbuf[5]='\0'; strcat(cbuf,"bC:Q ");  }
          taf(rec,et,"U_EMRSETBKCOLOR_set");
          
          for(m=0; m<2; m++){      /* two foreground colors         B & G */
             if(j){ cr = colorref_set(255, 0,   0);  }
             else { cr = colorref_set(  0, 0, 255);  }

             for(k=0; k<2; k++){  /* over 2 types of hatch */
                if(k){ hatch = U_HS_DIAGCROSS;  }
                else { hatch = U_HS_CROSS;      }
                rec = deleteobject_set(&pen, eht);                              taf(rec,et,"deleteobject_set"); 
                elp = extlogpen_set(U_PS_GEOMETRIC|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER, 50, U_BS_HATCHED, cr, hatch, 0, NULL);
                rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
                rec = selectobject_set(pen, eht);                               taf(rec,et,"selectobject_set");
                free(elp);

                offset += 100;  
                rec = U_EMRLINETO_set(pointl_set(9000 + k*100, offset));         taf(rec,et,"U_EMRLINETO_set");
             }
          }
       }
    }
    rec = U_EMRENDPATH_set();                                       taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEPATH_set(rclFrame);                            taf(rec,et,"U_EMRSTROKEPATH_set");

    // restore original settings
    rec = U_EMRSETBKMODE_set(U_TRANSPARENT);                 taf(rec,et,"U_EMRSETBKMODE_set");
    rec = U_EMRSETTEXTCOLOR_set(colorref_set(255,0,255));    taf(rec,et,"U_EMRSETTEXTCOLOR_set");
    rec = U_EMRSETBKCOLOR_set(colorref_set(0,255,255));      taf(rec,et,"U_EMRSETBKCOLOR_set");
    rec = U_EMRSETTEXTALIGN_set(U_TA_DEFAULT);               taf(rec,et,"U_EMRSETTEXTALIGN_set");


    /* Test clipping regions */
    if(!(mode & NO_CLIP_TEST)){
       test_clips(13250, 1400, &font, rclFrame, et,eht);
    }

/* ************************************************* */

//
//  careful not to call anything after here twice!!! 
//

    rec = U_EMREOF_set(0,NULL,et);
    taf(rec,et,"U_EMREOF_set");

    /* Test the endian routines (on either Big or Little Endian machines).
    This must be done befoe the call to emf_finish, as that will swap the byte
    order of the EMF data before it writes it out on a BE machine.  */
    
#if 1
    string = (char *) malloc(et->used);
    if(!string){
       printf("Could not allocate enough memory to test u_emf_endian() function\n");
    }
    else {
       memcpy(string,et->buf,et->used);
       status = 0;
       if(!U_emf_endian(et->buf,et->used,1)){
          printf("Error in byte swapping of completed EMF, native -> reverse\n");
       }
       if(!U_emf_endian(et->buf,et->used,0)){
          printf("Error in byte swapping of completed EMF, reverse -> native\n");
       }
       if(rgba_diff(string, et->buf, et->used, 0)){ 
          printf("Error in u_emf_endian() function, round trip byte swapping does not match original\n");
       }
       free(string);
    }
#endif // swap testing

    status=emf_finish(et, eht);
    if(status){ printf("emf_finish failed: %d\n", status); }
    else {      printf("emf_finish sucess\n");             }

    emf_free(&et);
    emf_htable_free(&eht);

  exit(EXIT_SUCCESS);
}
