/**
 Example progam used for exercising the libUEMF functions.  
 Produces a single output file: test_libuemf.wmf
 Single command line parameter, hexadecimal bit flag.
   1  Disable tests that block WMF import into PowerPoint (JPG/PNG images)
   2  Enable tests that block WMF being displayed in Windows Preview (none currently)
   4  Disable tests that block WMF import into LODraw (JPG/PNG images, Bitmap16 images)
   8  Disable clipping tests.
   Default is 0, neither option set.

 Compile with 
 
    gcc -g -O0 -o testbed_wmf -Wall -I. testbed_wmf.c uemf.c uemf_endian.c uemf_utf.c uwmf.c uwmf_endian.c -lm 

 or

    gcc -g -O0 -o testbed_wmf -DU_VALGRIND -Wall -I. testbed_wmf.c uemf.c uemf_endian.c uemf_utf.c  uwmf.c uwmf_endian.c  -lm 


 The latter from enables code which lets valgrind check each record for
 uninitialized data.
 
*/

/* If Version or Date are changed also edit the text labels for the output.

File:      testbed_wmf.c
Version:   0.0.24
Date:      25-JUL-2014
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2014 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "uwmf.h"
#include "uwmf_endian.h"

#define PPT_BLOCKERS     1
#define PREVIEW_BLOCKERS 2
#define LODRAW_BLOCKERS  4
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

void taf(const char *rec,WMFTRACK *wt, char *text){  // Test, append, free
    if(!rec){ printf("%s failed",text);                     }
    else {    printf("%s recsize: %d",text,U_wmr_size((U_METARECORD *) rec)); }
    (void) wmf_append((U_METARECORD *)rec,wt, 1);
    printf("\n");
#ifdef U_VALGRIND
    fflush(stdout);  // helps keep lines ordered within Valgrind
#endif
}

/* WMF header record is nonstandard, do NOT check recsize! */
void htaf(char *rec,WMFTRACK *wt, char *text){  // Test, append, free
    if(!rec){ printf("%s failed",text);                     }
    else {    printf("%s success (HEADER, no size)",text); }
    (void) wmf_header_append((U_METARECORD *)rec,wt, 1);
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
        length = U_wmr_size((U_METARECORD *) rec);
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

void spintext(uint32_t x, uint32_t y, uint32_t textalign, WMFTRACK *wt, WMFHANDLES *wht){
    char               *rec;
    int                 i;
    char               *string;
    int                 slen;
    int16_t            *dx;
    uint32_t            font=0;
    U_FONT             *puf;
    
    
    
    rec = U_WMRSETTEXTALIGN_set(textalign);  taf(rec,wt,"U_WMRSETTEXTALIGN_set");
    string = (char *) malloc(32);
    for(i=0; i<3600; i+=300){

       // set escapement and orientation in tenths of a degree counter clockwise rotation
       puf = U_FONT_set(  -30, 0, i, i, 
                         U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                         U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                         U_DEFAULT_QUALITY, U_DEFAULT_PITCH, "Courier New");
       rec  = wcreatefontindirect_set( &font, wht, puf); taf(rec,wt,"wextcreatefontindirect_set");
       free(puf);
       rec = wselectobject_set(font, wht);  taf(rec,wt,"wselectobject_set");

       sprintf(string,"....Degrees:%d",i/10);
       slen=strlen(string);
       dx = dx16_set(-30,  U_FW_NORMAL, slen);
       rec = U_WMREXTTEXTOUT_set(point16_set(x,y), slen, U_ETO_NONE, string, dx, U_RCL16_DEF);
       taf(rec,wt,"U_WMREXTTEXTOUT_set");
       free(dx);
       rec = wdeleteobject_set(&font, wht);  taf(rec,wt,"wdeleteobject_set");
    }
    free(string);
}
    
void draw_textrect(int xul, int yul, int width, int height, char *string, int size, WMFTRACK *wt){
    char                *rec;
    int                  slen;
    int16_t             *dx16;
    U_RECT16             rclBox;

    rclBox = U_RECT16_set(point16_set(xul,yul),point16_set(xul + width, yul + height));
    rec = U_WMRRECTANGLE_set(rclBox);                  taf(rec,wt,"U_WMRRECTANGLE_set");

    slen=strlen(string);
    dx16 = dx16_set(-size,  U_FW_NORMAL, slen);
    rec = U_WMREXTTEXTOUT_set(point16_set(xul+width/2,yul + height/2), slen, U_ETO_NONE, string, dx16, U_RCL16_DEF);
    taf(rec,wt,"U_WMREXTTEXTOUT_set");
    free(dx16);
}   

void textlabel(uint32_t size, const char *string, uint32_t x, uint32_t y, uint32_t font, WMFTRACK *wt, WMFHANDLES *wht){
    char               *rec;
    int                 slen;
    int16_t            *dx16;
    
    
    rec = U_WMRSETTEXTALIGN_set(U_TA_DEFAULT);      taf(rec,wt,"U_WMRSETTEXTALIGN_set");

    rec  = wselectobject_set(font, wht);            taf(rec,wt,"wselectobject_set");

    slen=strlen(string);
    dx16 = dx16_set(-size,  U_FW_NORMAL, slen);
    rec = U_WMREXTTEXTOUT_set(point16_set(x,y), slen, U_ETO_NONE, string, dx16, U_RCL16_DEF);
    taf(rec,wt,"U_WMREXTTEXTOUT_set");
    free(dx16);
}

void label_column(int x1, int y1, uint32_t font, WMFTRACK *wt, WMFHANDLES *wht){
    textlabel(40, "STRETCHDIB   ", x1, y1, font, wt, wht);          y1 += 220;
    textlabel(40, "BITBLT       ", x1, y1, font, wt, wht);          y1 += 220;
    textlabel(40, "STRETCHBLT   ", x1, y1, font, wt, wht);          y1 += 240;
    textlabel(40, "DIBBITBLT    ", x1, y1, font, wt, wht);          y1 += 220;
    textlabel(40, "DIBSTRETCHBLT", x1, y1, font, wt, wht);          y1 += 220;
    return;
}

void label_row(int x1, int y1, uint32_t font, WMFTRACK *wt, WMFHANDLES *wht){
    textlabel(30, "+COLOR32 ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+COLOR24 ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+COLOR16 ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "-COLOR16 ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+COLOR8  ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+COLOR4  ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+MONO    ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "-MONO    ", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+COLOR8 0", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+COLOR4 0", x1, y1, font, wt, wht);          x1 += 220;
    textlabel(30, "+MONO   0", x1, y1, font, wt, wht);          x1 += 220;
    return;
}

void image_column(WMFTRACK *wt, int x1, int y1, int w, int h, U_BITMAPINFO *Bmi, uint32_t cbPx, char *px){
    char *rec;
    int   step=0;

    rec = U_WMRSTRETCHDIB_set(
       point16_set(x1,y1 + step),
       point16_set(200,200),
       point16_set(0,0), 
       point16_set(w,h),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,wt,"U_WMRSTRETCHDIB_set");
    step += 220;


    /* convert Bmi + px to Bitmap16 object*/
    U_BITMAP16 *Bm16 = U_BITMAP16_set(
        Bmi->bmiHeader.biBitCount,                             //!<  bitmap type.?????  No documentation on what this field should hold!
        Bmi->bmiHeader.biWidth,                                //!<  bitmap width in pixels.
        Bmi->bmiHeader.biHeight,                               //!<  bitmap height in scan lines.
        4,                                                     //!<  alignment of bytes per scan line, image_column is only called with a DIB, so 4.
        Bmi->bmiHeader.biBitCount,                             //!<  number of adjacent color bits on each plane (R bits + G bits + B bits ????)
        px                                                     //!<  bitmap pixel data. Bytes contained = (((Width * BitsPixel + 15) >> 4) << 1) * Height
    );

    rec = U_WMRBITBLT_set(
       point16_set(x1,y1 + step),
       point16_set(w,h),
       point16_set(0,0),
       U_SRCCOPY,
       Bm16);
    taf(rec,wt,"U_WMRBITBLT_set");
    step += 220;

    rec = U_WMRSTRETCHBLT_set(
       point16_set(x1,y1 + step),
       point16_set(200,200),
       point16_set(0,0), 
       point16_set(w,h),
       U_SRCCOPY,
       Bm16);
    taf(rec,wt,"U_WMRSTRETCHBLT_set");
    step += 240;
    
    free(Bm16);

    rec = U_WMRDIBBITBLT_set(
       point16_set(x1,y1 + step),
       point16_set(200,200),
       point16_set(0,0),
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,wt,"U_WMRDIBBITBLT_set");
    step += 220;

    rec = U_WMRDIBSTRETCHBLT_set(
       point16_set(x1,y1 + step),
       point16_set(200,200),
       point16_set(0,0), 
       point16_set(w,h),
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,wt,"U_WMRDIBSTRETCHBLT_set");
    step += 240;
}

void not_in_wmf(double scale, int x, int y, int pen, int brush, WMFTRACK *wt, WMFHANDLES *wht){
    char *rec;
    U_POINT16 *point16;
    U_POINT16 missing[2] = { { 0, 0 }, { 200, 200 } };

    point16 = point16_transform(missing, 2, xform_alt_set(scale, 1.0, 0.0, 0.0, x, y));
    rec = wselectobject_set(pen, wht);                    taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush, wht);                  taf(rec,wt,"wselectobject_set");   // make this pen active
    rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16);        taf(rec,wt,"U_WMRRECTANGLE_set");
    free(point16);
}

void draw_star(WMFTRACK *wt, WMFHANDLES *wht, int font, int pen_black_1, int brush_gray, int x, int y){
   U_POINT16  Star[] = {
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
   U_POINT16 *points;
   char *rec;

   textlabel(40, "TextBeneathStarTest1", x -30 , y + 60, font, wt, wht);
   points = point16_transform(Star, 10, xform_alt_set(0.5, 1.0, 0.0, 0.0, x, y));
   rec = wselectobject_set(pen_black_1, wht);         taf(rec,wt,"wselectobject_set");
   rec = wselectobject_set(brush_gray, wht);          taf(rec,wt,"wselectobject_set");
   rec = U_WMRSETPOLYFILLMODE_set(U_WINDING);         taf(rec,wt,"U_WMRSETPOLYFILLMODE_set");
   rec = wbegin_path_set();                           taf(rec,wt,"wbegin_path_set");
   rec = U_WMRPOLYGON_set(10, points);                taf(rec,wt,"U_WMRPOLYGON_set");
   rec = wend_path_set();                             taf(rec,wt,"wend_path_set");
   textlabel(40, "TextAbove__StarTest2", x -30 , y + 210, font, wt, wht);

   free(points);
}

void test_clips(int x, int y, const int font, int pen_red_1, int pen_black_1, int brush_gray, int brush_null, WMFTRACK *wt, WMFHANDLES *wht){
   char *rec;

   textlabel(40, "NoClip", x , y - 60, font, wt, wht);
   draw_star(wt, wht, font, pen_black_1, brush_gray, x, y);

   /* rectangle clipping */
   y += 500;
   textlabel(40, "Rect (include)", x , y - 60, font, wt, wht);
   rec = wselectobject_set(pen_red_1, wht);                              taf(rec,wt,"selectobject_set");
   rec = wselectobject_set(brush_null, wht);                             taf(rec,wt,"selectobject_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y, x+200, y+400});             taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRSAVEDC_set();                                              taf(rec,wt,"U_WMRSAVEDC_set");
   rec = U_WMRINTERSECTCLIPRECT_set((U_RECT16){x, y, x+200, y+400});     taf(rec,wt,"U_WMRINTERSECTCLIPRECT_set");
   draw_star(wt, wht, font, pen_black_1, brush_gray, x, y);
   rec = U_WMRRESTOREDC_set(-1);                                         taf(rec,wt,"U_WMRSAVEDC_set");

   /* double rectangle clipping */
   y += 500;
   textlabel(40, "Rect (include,inlude)", x , y - 60, font, wt, wht);
   rec = wselectobject_set(pen_red_1, wht);                              taf(rec,wt,"selectobject_set");
   rec = wselectobject_set(brush_null, wht);                             taf(rec,wt,"selectobject_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y, x+200, y+400});             taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y+200, x+400, y+400});         taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRSAVEDC_set();                                              taf(rec,wt,"U_WMRSAVEDC_set");
   rec = U_WMRINTERSECTCLIPRECT_set((U_RECT16){x, y, x+200, y+400});     taf(rec,wt,"U_WMRINTERSECTCLIPRECT_set");
   rec = U_WMRINTERSECTCLIPRECT_set((U_RECT16){x, y+200, x+400, y+400}); taf(rec,wt,"U_WMRINTERSECTCLIPRECT_set");
   draw_star(wt, wht, font, pen_black_1, brush_gray, x, y);
   rec = U_WMRRESTOREDC_set(-1);                                         taf(rec,wt,"U_WMRSAVEDC_set");

   /* excluded rectangle clipping */
   y += 500;
   textlabel(40, "Rect (exlude)", x , y - 60, font, wt, wht);
   rec = wselectobject_set(pen_red_1, wht);                              taf(rec,wt,"selectobject_set");
   rec = wselectobject_set(brush_null, wht);                             taf(rec,wt,"selectobject_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y, x+200, y+400});             taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRSAVEDC_set();                                              taf(rec,wt,"U_WMRSAVEDC_set");
   rec = U_WMREXCLUDECLIPRECT_set((U_RECT16){x, y, x+200, y+400});       taf(rec,wt,"U_WMRINTERSECTCLIPRECT_set");
   draw_star(wt, wht, font, pen_black_1, brush_gray, x, y);
   rec = U_WMRRESTOREDC_set(-1);                                         taf(rec,wt,"U_WMRSAVEDC_set");

   /* double excluded rectangle clipping */
   y += 500;
   textlabel(40, "Rect (include,inlude)", x , y - 60, font, wt, wht);
   rec = wselectobject_set(pen_red_1, wht);                              taf(rec,wt,"selectobject_set");
   rec = wselectobject_set(brush_null, wht);                             taf(rec,wt,"selectobject_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y, x+200, y+400});             taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y+200, x+400, y+400});         taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRSAVEDC_set();                                              taf(rec,wt,"U_WMRSAVEDC_set");
   rec = U_WMREXCLUDECLIPRECT_set((U_RECT16){x, y, x+200, y+400});       taf(rec,wt,"U_WMRINTERSECTCLIPRECT_set");
   rec = U_WMREXCLUDECLIPRECT_set((U_RECT16){x, y+200, x+400, y+400});   taf(rec,wt,"U_WMRINTERSECTCLIPRECT_set");
   draw_star(wt, wht, font, pen_black_1, brush_gray, x, y);
   rec = U_WMRRESTOREDC_set(-1);                                         taf(rec,wt,"U_WMRSAVEDC_set");



   /* rectangle clipping then path LOGIC - NOT implemented in WMF */
   int i;
   for(i=U_RGN_MIN;i<=U_RGN_MAX;i++){
      y += 500;
      textlabel(40, "not implemented", x , y - 60, font, wt, wht);
      not_in_wmf(1.5,x,y, pen_red_1, brush_null, wt, wht);  /* WMF has no bezierto */
   }

   /* rectangle clipping then offset */
   y += 500;
   int ox = 100;
   int oy = 20;
   textlabel(40, "Rect (include,offset)", x , y - 60, font, wt, wht);
   rec = wselectobject_set(pen_red_1, wht);                              taf(rec,wt,"selectobject_set");
   rec = wselectobject_set(brush_null, wht);                             taf(rec,wt,"selectobject_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y, x+200, y+400});             taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x+ox, y+oy, x+200+ox, y+400+oy}); taf(rec,wt,"U_WMRRECTANGLE_set");

   rec = U_WMRSAVEDC_set();                                              taf(rec,wt,"U_WMRSAVEDC_set");
   rec = U_WMRINTERSECTCLIPRECT_set((U_RECT16){x, y, x+200, y+400});     taf(rec,wt,"U_WWMRINTERSECTCLIPRECT_set");
   rec = U_WMROFFSETCLIPRGN_set((U_POINT16){ox,oy});                     taf(rec,wt,"U_WMROFFSETCLIPRGN_set");
   draw_star(wt, wht, font, pen_black_1, brush_gray, x, y);
   rec = U_WMRRESTOREDC_set(-1);                                         taf(rec,wt,"U_WMRSAVEDC_set");

   /* double rectangle clipping  then offset */
   y += 500;
   textlabel(40, "Rects (include,include,offset)", x , y - 60, font, wt, wht);
   rec = wselectobject_set(pen_red_1, wht);                              taf(rec,wt,"selectobject_set");
   rec = wselectobject_set(brush_null, wht);                             taf(rec,wt,"selectobject_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y, x+200, y+400});             taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x, y+200, x+400, y+400});         taf(rec,wt,"U_WMRRECTANGLE_set");
   rec = U_WMRRECTANGLE_set((U_RECT16){x+ox, y+200+oy, x+200+ox, y+400+oy}); 
                                                                         taf(rec,wt,"U_WMRRECTANGLE_set");

   rec = U_WMRSAVEDC_set();                                              taf(rec,wt,"U_WMRSAVEDC_set");
   rec = U_WMRINTERSECTCLIPRECT_set((U_RECT16){x, y, x+200, y+400});     taf(rec,wt,"U_WWMRINTERSECTCLIPRECT_set");
   rec = U_WMRINTERSECTCLIPRECT_set((U_RECT16){x, y+200, x+400, y+400}); taf(rec,wt,"U_WWMRINTERSECTCLIPRECT_set");
   rec = U_WMROFFSETCLIPRGN_set((U_POINT16){ox,oy});                     taf(rec,wt,"U_WMROFFSETCLIPRGN_set");
   draw_star(wt, wht, font, pen_black_1, brush_gray, x, y);
   rec = U_WMRRESTOREDC_set(-1);                                         taf(rec,wt,"U_WMRSAVEDC_set");
}

int main(int argc, char *argv[]){
    WMFTRACK            *wt;
    WMFHANDLES          *wht;
    int                  brush_yellow;
    int                  brush_purple;
    int                  brush_gray;
    int                  brush_darkred;
    int                  brush_null;
    int                  pen_black_1;
    int                  pen_white_1;
    int                  pen_turquoise_50;
    int                  pen_red_1;
    int                  pen_red_10;
    int                  pen_darkred_10;
    int                  pen_darkred_200;
    int                  pen_null;
    int                  font_courier_400;
    int                  font_courier_60;
    int                  font_courier_40;
    int                  font_courier_30;
    int                  font_arial_300;
    int                  reg_1;
    U_FONT              *puf;
    U_POINT16            pl1;
    U_POINT16            pl12[12];
    uint16_t             plc[12];
    U_POINT16            plarray[7] = { { 200, 0 }, { 400, 200 }, { 400, 400 }, { 200, 600 } , { 0, 400 }, { 0, 200 }, { 200, 0 }};
    U_POINT16            star5[5]   = { { 100, 0 }, { 200, 300 }, { 0, 100 }, { 200, 100 } , { 0, 300 }};
    U_POINT16           *point16;
    U_POINT16            ScanLines[50];
    U_SCAN              *scans;
    U_REGION            *Region;
    int                  status;
    char                *rec;
    char                *string;
    char                *px;
    char                *rgba_px;
    char                *rgba_px2;
    U_RECT16             rclBox;
    int16_t             *dx;
    size_t               slen;
    uint32_t             pen=0;   //none of these have been defined yet
    uint32_t             brush=0;
    uint32_t             font=0;
    uint32_t             reg=0;
    U_WLOGBRUSH          lb;
    U_PEN                up;
    U_COLORREF           cr;
    U_POINT16            ul,lr;
    U_BITMAPINFOHEADER   Bmih;
    U_BITMAPINFO        *Bmi;
    U_PAIRF             *ps;
    U_BITMAP16         *Bm16;
    uint32_t             cbPx;
    uint32_t             colortype;
    U_RGBQUAD           *ct;         //color table
    int                  numCt;      //number of entries in the color table
    int                  i,j,k;
    int                  offset;

    int                  mode = 0;   // conditional for some tests
    unsigned int         umode;
    int                  cap, join, miter;
    char                 cbuf[132];
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
      printf("Syntax: testbed_wmf flags\n");
      printf("   Flags is a hexadecimal number composed of the following optional bits (use 0 for a standard run):\n");
      printf("   1  Disable tests that block WMF import into PowerPoint (JPG/PNG compression)\n");
      printf("   2  Enable tests that block WMF being displayed in Windows Preview (none currently)\n");
      printf("   4  Disable tests that block WMF import into LibreOffice Draw (JPG/PNG compression, Bitmap16 images)\n");
      printf("   8  Disable clipping tests.\n");
      exit(EXIT_FAILURE);
    } 
 
    /* ********************************************************************** */
    // set up and begin the WMF
 
    status=wmf_start("test_libuemf.wmf",1000000, 250000, &wt);  // space allocation initial and increment 
    status=wmf_htable_create(128, 128, &wht);

    ps = U_PAIRF_set(11.692913,8.267717);
    rec = U_WMRHEADER_set(ps,1200); // Example: drawing is A4 horizontal,  1200 dpi
    htaf(rec,wt,"U_WMRHEADER_set");  /* Not really a record, use special form of taf */
    free(ps);


    rec = U_WMRSETWINDOWEXT_set(point16_set(14031,9921));   taf(rec,wt,"U_WMRSETWINDOWEXT_set");
    rec = U_WMRSETWINDOWORG_set(point16_set(0,0));          taf(rec,wt,"U_WMRSETWINDOWORG_set");
    rec = U_WMRSETMAPMODE_set(U_MM_ANISOTROPIC);            taf(rec,wt,"U_WMRSETMAPMODE_set");

    /* Section 3.1.1 of the WMF PDF says to use the preceding three records for best device independence.
      It also says:
    
      1. Some applications will not work with a mode other than ANISOTROPIC, 
      2. SETVIEWPORT[ORG|EXT] should not be used if the user may resize the image.
      3. Not to put REGION records in WMF files as they are device dependent.
      4. Not to use META_BITBLT (use STRETCHBLT or STRETCHDIB)
    
    */

    rec = U_WMRSETBKMODE_set(U_TRANSPARENT);                taf(rec,wt,"U_WMRSETBKMODE_set");
    rec = U_WMRSETPOLYFILLMODE_set(U_WINDING);              taf(rec,wt,"U_WMRSETPOLYFILLMODE_set");
    rec = U_WMRSETROP2_set(U_R2_COPYPEN);                   taf(rec,wt,"U_WMRSETROP2_set");
    
    rec = wlinecap_set(U_WPS_CAP_SQUARE);                   taf(rec,wt,"wlinecap_set");
    rec = wlinejoin_set(U_WPS_JOIN_MITER);                  taf(rec,wt,"wlinejoin_set");
    rec = wmiterlimit_set(5);                               taf(rec,wt,"wmiterlimit_set");
    

   /* **********define all objects and associate with short names through variables ******************* */

    cr  = colorref_set(255,255,173);  //yellowish color, only use here
    lb  = U_WLOGBRUSH_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
    rec = wcreatebrushindirect_set(&brush, wht, lb);   taf(rec,wt,"wcreatebrushindirect_set");
    brush_yellow = brush;

    cr  = colorref_set(196, 127, 255);
    lb  = U_WLOGBRUSH_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
    rec = wcreatebrushindirect_set(&brush, wht,lb);                taf(rec,wt,"wcreatebrushindirect_set");
    brush_purple=brush;

    cr  = colorref_set(128, 128, 128);
    lb  = U_WLOGBRUSH_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
    rec = wcreatebrushindirect_set(&brush, wht,lb);                taf(rec,wt,"wcreatebrushindirect_set");
    brush_gray=brush;

    cr  = colorref_set(0x8B, 0x00, 0x00);
    lb  = U_WLOGBRUSH_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
    rec = wcreatebrushindirect_set(&brush, wht,lb);                taf(rec,wt,"wcreatebrushindirect_set");
    brush_darkred=brush;

    cr  = colorref_set(0, 0, 0);
    lb  = U_WLOGBRUSH_set(U_BS_NULL, cr, U_HS_SOLIDCLR);
    rec = wcreatebrushindirect_set(&brush, wht,lb);                taf(rec,wt,"wcreatebrushindirect_set");
    brush_null=brush;

    /*  Note that a statement like this:
        up  = U_PEN_set(U_PS_SOLID|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER, 1, cr);
        has no affect on the endcap or join setting when the resulting WMF is viewed
        in Windows XP Preview, or when the lines are imported into PowerPoint.
    */

    cr  = colorref_set(0,0,0);        //black color
    up  = U_PEN_set(U_PS_SOLID | U_PS_JOIN_MITER | U_PS_ENDCAP_SQUARE, 1, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);       taf(rec,wt,"wcreatepenindirect_set");
    pen_black_1 = pen;

    cr  = colorref_set(255,255,255);        //white color
    up  = U_PEN_set(U_PS_SOLID | U_PS_JOIN_MITER | U_PS_ENDCAP_SQUARE, 1, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);       taf(rec,wt,"wcreatepenindirect_set");
    pen_white_1 = pen;

    cr  = colorref_set(127, 255, 196);
    up  = U_PEN_set(U_PS_SOLID | U_PS_JOIN_MITER | U_PS_ENDCAP_SQUARE, 50, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);                   taf(rec,wt,"wcreatepenindect_set");
    pen_turquoise_50 = pen;

    cr  = colorref_set(255, 0, 0);
    up  = U_PEN_set(U_PS_SOLID | U_PS_JOIN_MITER | U_PS_ENDCAP_SQUARE, 1, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);                   taf(rec,wt,"wcreatepenindect_set");
    pen_red_1 = pen;

    cr  = colorref_set(255, 0, 0);
    up  = U_PEN_set(U_PS_SOLID | U_PS_JOIN_MITER | U_PS_ENDCAP_SQUARE, 10, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);                   taf(rec,wt,"wcreatepenindect_set");
    pen_red_10 = pen;

    cr  = colorref_set(0x8B, 0x00, 0x00);
    up  = U_PEN_set(U_PS_SOLID | U_PS_JOIN_MITER | U_PS_ENDCAP_SQUARE, 10, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);                   taf(rec,wt,"wcreatepenindect_set");
    pen_darkred_10 = pen;

    cr  = colorref_set(0x8B, 0x00, 0x00);
    up  = U_PEN_set(U_PS_SOLID | U_PS_JOIN_MITER | U_PS_ENDCAP_SQUARE, 200, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);                   taf(rec,wt,"wcreatepenindect_set");
    pen_darkred_200 = pen;

    cr  = colorref_set(0, 0, 0);
    up  = U_PEN_set(U_PS_NULL, 0, cr);
    rec = wcreatepenindirect_set(&pen, wht, up);                   taf(rec,wt,"wcreatepenindect_set");
    pen_null = pen;

    puf = U_FONT_set(  -400, 0, 0, 0, 
                      U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, "Courier New");
    rec  = wcreatefontindirect_set( &font, wht, puf); taf(rec,wt,"wextcreatefontindirect_set");
    free(puf);
    font_courier_400=font;
    
    puf = U_FONT_set(  -40, 0, 0, 0, 
                      U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, "Courier New");
    rec  = wcreatefontindirect_set( &font, wht, puf); taf(rec,wt,"wextcreatefontindirect_set");
    free(puf);
    font_courier_40=font;
    
    puf = U_FONT_set(  -60, 0, 0, 0, 
                      U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, "Courier New");
    rec  = wcreatefontindirect_set( &font, wht, puf); taf(rec,wt,"wextcreatefontindirect_set");
    free(puf);
    font_courier_60=font;
    
    puf = U_FONT_set(  -30, 0, 0, 0, 
                      U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, "Courier New");
    rec  = wcreatefontindirect_set( &font, wht, puf); taf(rec,wt,"wextcreatefontindirect_set");
    free(puf);
    font_courier_30=font;
    
    puf = U_FONT_set(  -300, 0, 0, 0, 
                      U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, "Arial");
    rec  = wcreatefontindirect_set( &font, wht, puf); taf(rec,wt,"wextcreatefontindirect_set");
    free(puf);
    font_arial_300=font;

   /* **********put a rectangle under everything, so that transparency is obvious ******************* */

    rec = wselectobject_set(pen_black_1, wht);         taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_yellow, wht);        taf(rec,wt,"wselectobject_set");
    rclBox = U_RECT16_set(point16_set(14031,9921),point16_set(0,0));
    rec = U_WMRRECTANGLE_set(rclBox);                  taf(rec,wt,"U_WMRRECTANGLE_set");

    /* label the drawing */
    
    textlabel(400, "libUEMF v0.1.17",      9700, 200, font_courier_400, wt, wht);
    textlabel(400, "July 25, 2014",       9700, 500, font_courier_400, wt, wht);
    rec = malloc(128);
    (void)sprintf(rec,"WMF test: %2.2X",mode);
    textlabel(400, rec,                    9700, 800, font_courier_400, wt, wht);
    free(rec);


    /* ********************************************************************** */
    // basic drawing operations
    
    rec = wselectobject_set(brush_purple, wht);                      taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(pen_turquoise_50, wht);                  taf(rec,wt,"wselectobject_set");

    ul     = point16_set(100,1300);
    lr     = point16_set(400,1600);
    rclBox = U_RECT16_set(ul,lr);
    rec = U_WMRELLIPSE_set(rclBox);    taf(rec,wt,"U_WMRELLIPSE_set");

    ul     = point16_set(500,1300);
    lr     = point16_set(800,1600);
    rclBox = U_RECT16_set(ul,lr);
    rec = U_WMRRECTANGLE_set(rclBox);  taf(rec,wt,"U_WMRRECTANGLE_set");

    rec = wbegin_path_set();                        taf(rec,wt,"wbegin_path_set");
    rec = U_WMRMOVETO_set(point16_set(900,1300));   taf(rec,wt,"U_WMRMOVET_set");
    rec = U_WMRLINETO_set(point16_set(1200,1500));  taf(rec,wt,"U_WMRLINETO_set");
    rec = U_WMRLINETO_set(point16_set(1200,1700));  taf(rec,wt,"U_WMRLINETO_set");
    rec = U_WMRLINETO_set(point16_set(900,1500));   taf(rec,wt,"U_WMRLINETO_set");
    rec = U_WMRLINETO_set(point16_set(900,1300));   taf(rec,wt,"U_WMRLINETO_set");
    rec = wend_path_set();                          taf(rec,wt,"wend_path_set");

    ul     = point16_set(1300,1300);
    lr     = point16_set(1600,1600);
    rclBox = U_RECT16_set(ul,lr);
    rec = U_WMRARC_set(point16_set(1450,1300), point16_set(1300,1600), rclBox);     taf(rec,wt,"U_WMRARC_set");
 
    not_in_wmf(1.5,1500,1300, pen_red_10, brush_darkred, wt, wht);  /* WMF has no bezierto */
    rec = wselectobject_set(brush_purple, wht);                      taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(pen_turquoise_50, wht);                  taf(rec,wt,"wselectobject_set");

    ul     = point16_set(1900,1300);
    lr     = point16_set(2200,1600);
    rclBox = U_RECT16_set(ul,lr);
    rec = U_WMRCHORD_set(point16_set(1900,1300), point16_set(2050,1600), rclBox);   taf(rec,wt,"U_WMRCHORD_set");

    ul     = point16_set(2200,1300);
    lr     = point16_set(2500,1600);
    rclBox = U_RECT16_set(ul,lr);
    rec = U_WMRPIE_set(point16_set(2350,1600), point16_set(2200,1300), rclBox);   taf(rec,wt,"U_WMRPIE_set");

    ul     = point16_set(2600,1300);
    lr     = point16_set(2900,1600);
    rclBox = U_RECT16_set(ul,lr);
    rec = U_WMRROUNDRECT_set(25,100,rclBox);                                      taf(rec,wt,"U_WMRROUNDRECT_set");

    /* WMRPATBLT is a rectangle with no border, type in WMF that has no specific record in EMF (not needed
    there) */
    rec = U_WMRPATBLT_set(point16_set(3600,1300),point16_set(300,300),U_PATCOPY);
    taf(rec,wt,"U_WMRPATBLT_set");

    rec = wselectobject_set(pen_black_1, wht);      taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_gray, wht);     taf(rec,wt,"wselectobject_set");


    point16 = point16_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3000, 1300));
    rec = U_WMRSETPOLYFILLMODE_set(U_WINDING);      taf(rec,wt,"U_WMRSETPOLYFILLMODE_set");
    rec = wbegin_path_set();                        taf(rec,wt,"wbegin_path_set");
    rec = U_WMRPOLYGON_set(5, point16);             taf(rec,wt,"U_WMRPOLYGON_set");
    rec = wend_path_set();                          taf(rec,wt,"wend_path_set");
    free(point16);

    point16 = point16_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3300, 1300));
    rec = U_WMRSETPOLYFILLMODE_set(U_ALTERNATE);    taf(rec,wt,"U_WMRSETPOLYFILLMODE_set");
    rec = wbegin_path_set();                        taf(rec,wt,"wbegin_path_set");
    rec = U_WMRPOLYGON_set(5, point16);             taf(rec,wt,"U_WMRPOLYGON_set");
    rec = wend_path_set();                          taf(rec,wt,"wend_path_set");
    free(point16);

    /* ****************** various types of draw *************************** */


    not_in_wmf(1.5,100,1800, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polybezier */

    rec = wselectobject_set(pen_red_10, wht);          taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_gray, wht);       taf(rec,wt,"wselectobject_set");

    point16 = point16_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 600, 1800));
    rec = wbegin_path_set();                        taf(rec,wt,"wbegin_path_set");
    rec = U_WMRPOLYGON_set(6, point16);             taf(rec,wt,"U_WMRPOLYGON_set");
    rec = wend_path_set();                          taf(rec,wt,"wend_path_set");
    free(point16);
 
    point16 = point16_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1100, 1800));
    rec = wbegin_path_set();                        taf(rec,wt,"wbegin_path_set");
    rec = U_WMRPOLYLINE_set(6, point16);            taf(rec,wt,"U_WMRPOLYLINE_set");
    rec = wend_path_set();                          taf(rec,wt,"wend_path_set");
    free(point16);

    not_in_wmf(1.5,1600,1800, pen_red_10, brush_darkred, wt, wht);  /* WMF has no bezierto */

    not_in_wmf(1.5,2100,1800, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polylineto */

    not_in_wmf(1.5,2600,1800, pen_red_10, brush_darkred, wt, wht);  /* WMF has no abortpath */

    // same without begin/end path

    not_in_wmf(1.5,3100,1800, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polybezier */

    rec = wselectobject_set(pen_red_10, wht);          taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_gray, wht);       taf(rec,wt,"wselectobject_set");

    point16 = point16_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3600, 1800));
    rec = U_WMRPOLYGON_set(6, point16);             taf(rec,wt,"U_WMRPOLYGON_set");
    free(point16);
 
    point16 = point16_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4100, 1800));
    rec = U_WMRPOLYLINE_set(6, point16);            taf(rec,wt,"U_WMRPOLYLINE_set");
    free(point16);

    not_in_wmf(1.5,4600,1800, pen_red_10, brush_darkred, wt, wht);  /* WMF has no bezierto */

    not_in_wmf(1.5,5100,1800, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polylineto */


     /* ********************************************************************** */
    // test transform routines, first generate a circle of points

    rec = wselectobject_set(pen_red_10, wht);          taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_gray, wht);       taf(rec,wt,"wselectobject_set");

    for(i=0; i<12; i++){
       pl1     = point16_set(1,0);
       point16  = point16_transform(&pl1,1,xform_alt_set(100.0, 1.0, (float)(i*30), 0.0, 0.0, 0.0));
       pl12[i] = *point16;
       free(point16);
    }
    pl12[0].x = U_ROUND((float)pl12[0].x) * 1.5; // make one points stick out a bit

    // test scale (range 1->2) and axis ratio (range 1->0)
    for(i=0; i<12; i++){
       point16 = point16_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 0.0, 30.0*((float)i), 200 + i*300, 2800));
       rec = wbegin_path_set();              taf(rec,wt,"wbegin_path_set");
       rec = U_WMRPOLYGON_set(12, point16);  taf(rec,wt,"U_WMRPOLYGON_set");
       rec = wend_path_set();                taf(rec,wt,"wend_path_set");
       free(point16);
    }

    // test scale (range 1->2) and axis ratio (range 1->0), rotate major axis to vertical (point lies on positive Y axis)
    for(i=0; i<12; i++){
       point16 = point16_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 90.0, 30.0*((float)i), 200 + i*300, 3300));
       rec = wbegin_path_set();              taf(rec,wt,"wbegin_path_set");
       rec = U_WMRPOLYGON_set(12, point16);  taf(rec,wt,"U_WMRPOLYGON_set");
       rec = wend_path_set();                taf(rec,wt,"wend_path_set");
       free(point16);
    }
    
    // test polypolyline and polypolygon using this same array.

    plc[0]=3;
    plc[1]=4;
    plc[2]=5;

    not_in_wmf(1.0, 4000 - 100, 2800 - 100, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polypolyline32  */

    not_in_wmf(1.0, 4000 - 100, 3300 - 100, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polypolyline16  */

    rec = wselectobject_set(pen_red_10, wht);          taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_gray, wht);       taf(rec,wt,"wselectobject_set");
    point16 = point16_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4300, 2800));
    rec = U_WMRPOLYPOLYGON_set(3, plc, point16);    taf(rec,wt,"U_WMRPOLYPOLYGON_set");
    free(point16);

    not_in_wmf(1.0, 4300 - 100, 3300 - 100, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polypolygon 16 bit  */

 

    plc[0]=3;
    plc[1]=3;
    plc[2]=3;
    plc[3]=3;

    not_in_wmf(1.0, 4600 - 100, 2800 - 100, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polypolyline 32 bit  */
    not_in_wmf(1.0, 4600 - 100, 3300 - 100, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polypolyline 16 bit  */

    rec = wselectobject_set(pen_red_10, wht);          taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_gray, wht);       taf(rec,wt,"wselectobject_set");
    point16 = point16_transform(pl12, 12, xform_alt_set(1.0, 1.0, 0.0, 0.0, 4900, 2800));
    rec = U_WMRPOLYPOLYGON_set(4, plc, point16);
    taf(rec,wt,"U_WMRPOLYPOLYGON_set");
    free(point16);

    not_in_wmf(1.0, 4900 - 100, 3300 - 100, pen_red_10, brush_darkred, wt, wht);  /* WMF has no polypolygon 16 bit  */

    if(mode & PREVIEW_BLOCKERS){

    } // PREVIEW_BLOCKERS


    /* ********************************************************************** */
    // line types

 
    pl12[0] = point16_set(0,   0  );
    pl12[1] = point16_set(200, 200);
    pl12[2] = point16_set(0,   400);
    pl12[3] = point16_set(2,   1 ); // these next two are actually used as the pattern for U_PS_USERSTYLE
    pl12[4] = point16_set(4,   3 ); // dash =2, gap=1, dash=4, gap =3
    for(i=0; i<=U_PS_DASHDOTDOT; i++){
       cr  = colorref_set(255,0,255);        //fuchsia color
       up  = U_PEN_set(i, 1, cr);
       rec = wcreatepenindirect_set(&pen, wht, up);       taf(rec,wt,"wcreatepenindirect_set");
       rec = wselectobject_set(pen, wht);                 taf(rec,wt,"wselectobject_set");

       point16 = point16_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
       rec = U_WMRPOLYLINE_set(3, point16);               taf(rec,wt,"U_WMRPOLYLINE_set");
       free(point16);
       rec = wdeleteobject_set(&pen, wht);                taf(rec,wt,"wdeleteobject_set");
    }

    /* WMF apparently does not support custom dash patterns.  There is U_PS_USERSTYLE but no record defines a pen
       using one of those patterns */
    rec = wselectobject_set(pen_darkred_10, wht);  taf(rec,wt,"wselectobject_set");

    point16 = point16_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
    rec = U_WMRPOLYLINE_set(3, point16); taf(rec,wt,"U_WMRPOLYLINE_set");
    free(point16);
    i++;

    point16 = point16_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
    rec = U_WMRPOLYLINE_set(3, point16); taf(rec,wt,"U_WMRPOLYLINE_set");
    free(point16);

    /* run over all combinations of cap(join(miter 1,5))), but draw as solid lines with */
    pl12[0] = point16_set(0,   0  );
    pl12[1] = point16_set(600, 200);
    pl12[2] = point16_set(0,   400);
    for (cap=U_PS_ENDCAP_ROUND; cap<= U_PS_ENDCAP_FLAT; cap+= U_PS_ENDCAP_SQUARE){
       for(join=U_PS_JOIN_ROUND; join<=U_PS_JOIN_MITER; join+= U_PS_JOIN_BEVEL){
          for(miter=1;miter<=5;miter+=4){
             rec = wmiterlimit_set(miter);                      taf(rec,wt,"wmiterlimit_set");

             up  = U_PEN_set(U_PS_SOLID| U_PS_GEOMETRIC | cap | join, 20, colorref_set(64*(5-miter), 0, 0));
             rec = wcreatepenindirect_set(&pen, wht, up);       taf(rec,wt,"wcreatepenindirect_set");
             rec = wselectobject_set(pen, wht);                 taf(rec,wt,"wselectobject_set");

             point16 = point16_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 500 + (cap/U_PS_ENDCAP_SQUARE)*250 + miter*20, 5200 + (join/U_PS_JOIN_BEVEL)*450));
             rec = U_WMRPOLYLINE_set(3, point16); taf(rec,wt,"U_WMRPOLYLINE_set");
             free(point16);
             rec = wdeleteobject_set(&pen, wht);  taf(rec,wt,"deleteobject_set");
          }
       }
    }
    rec = wmiterlimit_set(8);                       taf(rec,wt,"wmiterlimit_set");
    rec = wselectobject_set(pen_black_1, wht);      taf(rec,wt,"wselectobject_set");

    /* ********************************************************************** */
    // bitmaps
    
    offset = 5000;
    label_column(offset, 5000, font_courier_40, wt, wht);
    label_row(offset + 400, 5000 - 30, font_courier_30, wt,  wht);
    offset += 400;

    // Make the first test image, it is 10 x 10 and has various colors, R,G,B in each of 3 corners
    rgba_px = (char *) malloc(10*10*4);
    FillImage(rgba_px,10,10,40);

    rec =  U_WMRSETSTRETCHBLTMODE_set(U_STRETCH_DELETESCANS);  taf(rec,wt,"U_WMRSETSTRETCHBLTMODE_set");

    /* Windows XP Preview (GDI?) does not render this properly, colors are offset  */
    colortype = U_BCBM_COLOR32;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
       //  Test the inverse operation - this does not affect the WMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR32\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(wt, offset, 5000,10,10,Bmi, cbPx, px);
    offset += 220;

    // we are going to step on this one with little rectangles using different binary raster operations
    rec = U_WMRSTRETCHDIB_set(
       point16_set(2900,5000),
       point16_set(2000,2000),
       point16_set(0,0), 
       point16_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,wt,"U_WMRSTRETCHDIB_set");

    free(Bmi);
    free(px);


    /* Windows XP Preview (GDI?) does render this properly  */
    colortype = U_BCBM_COLOR24;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
       //  Test the inverse operation - this does not affect the WMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 1))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR24\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, NULL);
    image_column(wt, offset, 5000,10,10,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);
    free(px);

    // 16 bit, 5 bits per color, no table
    /* Windows XP Preview (GDI?) does not render this properly, colors are offset  */
    colortype = U_BCBM_COLOR16;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
       //  Test the inverse operation - this does not affect the WMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4,2))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR16\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(wt, offset, 5000,10,10,Bmi, cbPx, px);
    offset += 220;
       
    // write a second copy next to it using the negative height method to indicate it should be upside down
    /* Windows XP Preview (GDI?) does not render this properly, colors are offset  */
    Bmi->bmiHeader.biHeight *= -1;
    image_column(wt, offset, 5000,10,10,Bmi, cbPx, px);
    offset += 220;

    free(Bmi);
    free(px);


    /* Windows XP Preview (GDI?) does render this properly */
    colortype = U_BCBM_COLOR8;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
       //  Test the inverse operation - this does not affect the WMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR8\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(wt, offset, 5000,10,10,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);
    
    // also test the numCt==0 form
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, 0, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(wt, offset+660, 5000,10,10,Bmi, cbPx, px);
    free(Bmi);
        
    free(ct);
    free(px);

    // done with the first test image, make the 2nd

    free(rgba_px); 
    rgba_px = (char *) malloc(4*4*4);
    FillImage(rgba_px,4,4,16);

    /* Windows XP Preview (GDI?) does render this properly */
    colortype = U_BCBM_COLOR4;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
       //  Test the inverse operation - this does not affect the WMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 4, 4,     colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
    if(rgba_diff(rgba_px, rgba_px2, 4 * 4 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR4\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(wt, offset, 5000,4,4,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);

    // also test the numCt==0 form
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, 0, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(wt, offset + 660, 5000,4,4,Bmi, cbPx, px);

    free(ct);
    free(Bmi);
    free(px);

    // make a two color image in the existing RGBA array
    memset(rgba_px, 0x55, 4*4*4);
    memset(rgba_px, 0xAA, 4*4*2);

    /* Windows XP Preview (GDI?) does render this properly */
    colortype = U_BCBM_MONOCHROME;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
       //  Test the inverse operation - this does not affect the WMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 4, 4,     colortype, U_CT_BGRA, U_ROW_ORDER_SAME);
    if(rgba_diff(rgba_px, rgba_px2, 4 * 4 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_MONOCHROME\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
        image_column(wt, offset, 5000,4,4,Bmi, cbPx, px);
    offset += 220;

    // write a second copy next to it using the negative height method to indicate it should be upside down
    /* Windows XP Preview (GDI?) does render this properly */
    Bmi->bmiHeader.biHeight *= -1;
    image_column(wt, offset,5000,10,10,Bmi, cbPx, px);
    offset += 220;
    free(Bmi);

    // also test the numCt==0 form
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, 0, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    image_column(wt, offset + 660 - 220 ,5000,4,4,Bmi, cbPx, px);

    free(ct);
    free(Bmi);
    free(px);

    free(rgba_px);


    // Make another sacrificial image, this one all black
    rec = U_WMRBITBLT_set(
        point16_set(2900,7020),
        point16_set(2000,2000),
        point16_set(0,0),
        U_DSTINVERT,
        NULL);
    taf(rec,wt,"U_WMRBITBLT_set");

    if(!(mode & (PPT_BLOCKERS | LODRAW_BLOCKERS))){

        textlabel(40, "STRETCHDIBITS", 5000, 6200, font_courier_40, wt, wht);

        /* Screen display in GDI apparently does not support either the PNG or JPG mode, some printers may. 
        When these are used the images will not be visible in Windows XP Preview and they will terminate
        processing of the WMF if ungrouped within PowerPoint 2003 at this point. */
        px   = (char *) &pngarray[0];
        cbPx = 138; 
        Bmih = bitmapinfoheader_set(10, -10, 1, U_BCBM_EXPLICIT, U_BI_PNG, cbPx, 0, 0, 0, 0); /* PNG, fields 5 and 6 must be present! */
        Bmi  = bitmapinfo_set(Bmih, NULL);
        rec = U_WMRSTRETCHDIB_set(
           point16_set(5400,6200),
           point16_set(200,200),
           point16_set(0,0), 
           point16_set(10,10),
           U_DIB_RGB_COLORS, 
           U_SRCCOPY,
           Bmi, 
           cbPx, 
           px);
        taf(rec,wt,"U_WMRSTRETCHDIB_set");
        free(Bmi);
        textlabel(30, "PNG", 5400, 6200-30, font_courier_30, wt, wht);

        px   = (char *) &jpgarray[0];
        cbPx = 676; 
        Bmih = bitmapinfoheader_set(10, -10, 1, U_BCBM_EXPLICIT, U_BI_JPEG, cbPx, 0, 0, 0, 0); /* JPG, fields 5 and 6 must be present! */
        Bmi  = bitmapinfo_set(Bmih, NULL);
        rec = U_WMRSTRETCHDIB_set(
           point16_set(5620,6200),
           point16_set(200,200),
           point16_set(0,0), 
           point16_set(10,10),
           U_DIB_RGB_COLORS, 
           U_SRCCOPY,
           Bmi, 
           cbPx, 
           px);
        taf(rec,wt,"U_WMRSTRETCHDIB_set");
        free(Bmi);
        textlabel(30, "JPG", 5620, 6200-30, font_courier_30, wt, wht);
    } //PPT_BLOCKERS || LODRAW_BLOCKERS

    // testing binary raster operations
    // make a series of rectangle draws with grey rectangles under various binary raster operations
    for(i=1;i<=16;i++){
       rec = U_WMRSETROP2_set(i);                                                taf(rec,wt,"U_WMRSETROP2_set");
       ul     = point16_set(2800 + i*100,5000);
       lr     = point16_set(2800 + i*100+90,9019);
       rec = U_WMRRECTANGLE_set(U_RECT16_set(ul,lr));                            taf(rec,wt,"U_WMRRECTANGLE_set");
    }
    // make a series of white line draws under various binary raster operations
    rec = wselectobject_set(pen_white_1, wht); taf(rec,wt,"wselectobject_set");
    for(i=1;i<=16;i++){
       rec = U_WMRSETROP2_set(i);                                                taf(rec,wt,"U_WMRSETROP2_set");
       rec = U_WMRMOVETO_set(point16_set(4520 + i*10,4950));                     taf(rec,wt,"U_WMRMOVETOEX_set");
       rec = U_WMRLINETO_set(  point16_set(4520 + i*10,9069));                   taf(rec,wt,"U_WMRLINETO_set");
    }
    // make a series of black line draws under various binary raster operations, restore previous pen value
    rec = wselectobject_set(pen_black_1, wht); taf(rec,wt,"wselectobject_set");
    for(i=1;i<=16;i++){
       rec = U_WMRSETROP2_set(i);                                                taf(rec,wt,"U_WMRSETROP2_set");
       rec = U_WMRMOVETO_set(point16_set(4720 + i*10,4950));                     taf(rec,wt,"U_WMRMOVETOEX_set");
       rec = U_WMRLINETO_set(  point16_set(4720 + i*10,9069));                   taf(rec,wt,"U_WMRLINETO_set");
    }
    //restore the previous defaults
    rec = U_WMRSETROP2_set(U_R2_COPYPEN);    taf(rec,wt,"U_WMRSETROP2_set");


    rec  = wselectobject_set(font_arial_300, wht);        taf(rec,wt,"wselectobject_set");
    rec = U_WMRSETTEXTCOLOR_set(colorref_set(255,0,0));   taf(rec,wt,"U_WMRSETTEXTCOLOR_set");
    rec = U_WMRSETBKCOLOR_set(colorref_set(0,255,0));     taf(rec,wt,"U_WMRSETBKCOLOR_set");

    string = U_strdup("Text from U_WMREXTTEXTOUT");
    slen = strlen(string);
    dx = dx16_set(-300,  U_FW_NORMAL, slen);
    rec = U_WMREXTTEXTOUT_set(point16_set(100, 50), slen, U_ETO_NONE, string, dx, U_RCL16_DEF);
    taf(rec,wt,"U_WMREXTTEXTOUT_set");
    free(dx);
    free(string);

    string = U_strdup("Text from U_WMRTEXTOUT");
    rec = U_WMRTEXTOUT_set(point16_set(100, 350), string);
    taf(rec,wt,"U_WMRTEXTOUT_set");
    free(string);

    not_in_wmf(1.0, 100, 650, pen_red_10, brush_darkred, wt, wht);  /* WMF has no SMALLTEXTOUT or EXTTEXTOUTA  */

    // In some monochrome modes these are used.  0 gets the bk color and !0 gets the text color.
    // Make these distinctive colors so it is obvious where it is happening.  Background is "Aqua", foreground is "Fuchsia"
    rec  = wselectobject_set(font_arial_300, wht);        taf(rec,wt,"wselectobject_set");
    rec = U_WMRSETTEXTCOLOR_set(colorref_set(255,0,255));    taf(rec,wt,"U_WMRSETTEXTCOLOR_set");
    rec = U_WMRSETBKCOLOR_set(colorref_set(0,255,255));      taf(rec,wt,"U_WMRSETBKCOLOR_set");
    string = U_strdup("Text from U_WMREXTTEXTOUT");
    slen = strlen(string);
    dx = dx16_set(-300,  U_FW_NORMAL, slen);
    rec = U_WMREXTTEXTOUT_set(point16_set(100, 950), slen, U_ETO_NONE, string, dx, U_RCL16_DEF);
    taf(rec,wt,"U_WMREXTTEXTOUT_set");
    free(dx);
    free(string);

    /* ***************    test all text alignments  *************** */
    /* WNF documentation (section 2.1.2.3) says that the bounding rectangle should supply edges which are used as
       reference points.  This appears to not be true.  The following example draws two sets of aligned text,
       with the bounding rectangles indicated with a grey rectangle, using two different size bounding rectangles,
       and the text is in the same position in both cases.
       
    */
    
    rec = wselectobject_set(font_courier_30, wht);   taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(pen_null,        wht);   taf(rec,wt,"wselectobject_set");
    rec = wselectobject_set(brush_gray,      wht);   taf(rec,wt,"wselectobject_set");


    // Use a smaller font for text alignment tests.  Note that if the target system does
    // not have "Courier New" it will use some default font, which will not look right

    rec = U_WMRRECTANGLE_set(U_RECT16_set(point16_set(5500-20,100-20),point16_set(5500+20,100+20)));
    taf(rec,wt,"U_WMRRECTANGLE_set");
    string = U_strdup("textalignment:default");
    slen = strlen(string);
    dx = dx16_set(-30,  U_FW_NORMAL, slen);
    rec = U_WMREXTTEXTOUT_set(point16_set(5500,100), slen, U_ETO_NONE, string, dx, U_RCL16_DEF);
    taf(rec,wt,"U_WMREXTTEXTOUT_set");
    free(dx);
    free(string);

    string = (char *) malloc(32);
    for(i=0;i<=0x18; i+=2){
       rec = U_WMRSETTEXTALIGN_set(i);            taf(rec,wt,"U_WMRSETTEXTALIGN_set");

       rec = U_WMRRECTANGLE_set(U_RECT16_set(point16_set(6000-20,100+ i*50-20),point16_set(6000+20,100+ i*50+20)));
       taf(rec,wt,"U_WMRRECTANGLE_set");

       sprintf(string,"textalignment:0x%2.2X",i);
       slen   = strlen(string);
       dx = dx16_set(-30,  U_FW_NORMAL, slen);
       rec = U_WMREXTTEXTOUT_set(point16_set(6000,100+ i*50), slen, U_ETO_NONE, string, dx, U_RCL16_DEF);
       taf(rec,wt,"U_WMREXTTEXTOUT_set");
       free(dx);
    }
 
    for(i=0;i<=0x18; i+=2){
       rec = U_WMRSETTEXTALIGN_set(i);            taf(rec,wt,"U_WMRSETTEXTALIGN_set");
 
       rec = U_WMRRECTANGLE_set(U_RECT16_set(point16_set(7000-40,100+ i*50-40),point16_set(7000+40,100+ i*50+40)));
       taf(rec,wt,"U_WMRRECTANGLE_set");

       sprintf(string,"textalignment:0x%2.2X",i);
       slen   = strlen(string);
       dx = dx16_set(-30,  U_FW_NORMAL, slen);
       rec = U_WMREXTTEXTOUT_set(point16_set(7000-40,100+ i*50), slen, U_ETO_NONE, string, dx, U_RCL16_DEF);
       taf(rec,wt,"U_WMREXTTEXTOUT_set");
       free(dx);
    }
    free(string);
    
    // restore the default text alignment
    rec = U_WMRSETTEXTALIGN_set(U_TA_DEFAULT);    taf(rec,wt,"U_WMRSETTEXTALIGN_set");
    
    /* ***************    test rotated text  *************** */
    
    spintext(8000, 300,U_TA_BASELINE,wt,wht);
    spintext(8600, 300,U_TA_TOP,     wt,wht);
    spintext(9200, 300,U_TA_BOTTOM,  wt,wht);
    rec = U_WMRSETTEXTALIGN_set(U_TA_BASELINE); // go back to baseline
    taf(rec,wt,"U_WMRSETTEXTALIGN_set");
    

    /* ***************    test hatched fill and stroke (standard patterns)  *************** */
    
    // use BLACK_PEN (edge on drawn rectangle)
    rec = wselectobject_set(pen_black_1, wht);    taf(rec,wt,"wselectobject_set");
    cr = colorref_set(63, 127, 255);
    ul = point16_set(0,0);
    lr = point16_set(300,300);
    rclBox = U_RECT16_set(ul,lr);

    /* *** fill *** */
    for(i=0;i<=U_HS_DITHEREDBKCLR;i++){
      lb  = U_WLOGBRUSH_set(U_BS_HATCHED, cr, i);
      rec = wcreatebrushindirect_set(&brush, wht,lb);   taf(rec,wt,"wcreatebrushindirect_set");
      rec = wselectobject_set(brush, wht);              taf(rec,wt,"wselectobject_set");

      point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 3500));
      rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16);    taf(rec,wt,"U_WMRRECTANGLE_set");
      free(point16);
      rec = wdeleteobject_set(&brush, wht);             taf(rec,wt,"wdeleteobject_set");
    }
    rec = wselectobject_set(brush_null, wht);      taf(rec,wt,"wselectobject_set");


    /* *** stroke *** 
    WMF does not appear to support hatched stroked
    */
    rec = wselectobject_set(pen_darkred_200, wht); taf(rec,wt,"wselectobject_set");
    for(i=0; i<=U_HS_DITHEREDBKCLR; i++){
      rec = U_WMRMOVETO_set(point16_set(6300+i*330, 3650));     taf(rec,wt,"U_WMRMOVETOEX_set"); /* not as tall as for EMF, line may have rounded caps  */
      rec = U_WMRLINETO_set(point16_set(6300+i*330, 4050));     taf(rec,wt,"U_WMRLINETO_set");
    }


    //repeat with red background, green text

    rec = U_WMRSETBKCOLOR_set(colorref_set(255, 0, 0));  taf(rec,wt,"U_WMRSETBKCOLOR_set");
    rec = U_WMRSETTEXTCOLOR_set(colorref_set(0,255,0));  taf(rec,wt,"U_WMRSETTEXTCOLOR_set");
    rec = wselectobject_set(pen_black_1, wht);             taf(rec,wt,"wselectobject_set");
    for(i=0;i<=U_HS_DITHEREDBKCLR;i++){
       lb  = U_WLOGBRUSH_set(U_BS_HATCHED, cr, i);
       rec = wcreatebrushindirect_set(&brush, wht,lb);   taf(rec,wt,"wcreatebrushindirect_set");
       rec = wselectobject_set(brush, wht);              taf(rec,wt,"wselectobject_set");

       point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 3830));
       rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16); taf(rec,wt,"U_WMRRECTANGLE_set");
       free(point16);
       rec = wdeleteobject_set(&brush, wht);             taf(rec,wt,"wdeleteobject_set");
    }
    
    // restore original settings
    rec = U_WMRSETTEXTCOLOR_set(colorref_set(255,0,255));     taf(rec,wt,"U_WMRSETTEXTCOLOR_set");
    rec = U_WMRSETBKCOLOR_set(colorref_set(0,255,255));       taf(rec,wt,"U_WMRSETBKCOLOR_set");

    
    /* ***************    test image fill and stroke  *************** */
     
    // this will draw a series of squares of increasing size, all with the same color fill pattern  
    
    // Make the first test image, it is 5 x 5 and has various colors, R,G,B in each of 3 corners
    rgba_px = (char *) malloc(5*5*4);
    FillImage(rgba_px,5,5,20);

    colortype = U_BCBM_COLOR32;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  5, 5, 20, colortype, U_CT_NO, U_ROW_ORDER_INVERT);
    Bmih = bitmapinfoheader_set(5, 5, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);

    /* use dibcreatepatternbrush_srcdib_set */

    rec = wcreatedibpatternbrush_srcdib_set(&brush, wht, U_DIB_RGB_COLORS, Bmi, cbPx, px);
    taf(rec,wt,"wcreatedibpatternbrushpt_set");
    rec = wselectobject_set(brush, wht);              taf(rec,wt,"wselectobject_set");

    ul = point16_set(0,0);
    for(i=1;i<=10;i++){
      lr = point16_set(30*i,30*i);
      rclBox = U_RECT16_set(ul,lr);
      point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4160));
      rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16); taf(rec,wt,"U_WMRRECTANGLE_set");
      free(point16);
    }
    rec = wdeleteobject_set(&brush, wht);  taf(rec,wt,"wdeleteobject_set");

    if(!(mode & LODRAW_BLOCKERS)){  /* LODRAW does not like bitmap16 */
       /* use dibcreatepatternbrush_srcbm16_set */
       Bm16 = U_BITMAP16_set(
           Bmi->bmiHeader.biBitCount,    //!<  bitmap type.?????  No documentation on what this field should hold!
           Bmi->bmiHeader.biWidth,       //!<  bitmap width in pixels.
           Bmi->bmiHeader.biHeight,      //!<  bitmap height in scan lines.
           4,                            //!<  alignment in array, this is a DIB, so 4
           Bmi->bmiHeader.biBitCount,    //!<  number of adjacent color bits on each plane (R bits + G bits + B bits ????)
           px                            //!<  bitmap pixel data. Bytes contained = (((Width * BitsPixel + 15) >> 4) << 1) * Height
       );
       rec = wcreatedibpatternbrush_srcbm16_set(&brush, wht, U_DIB_RGB_COLORS, Bm16);  
       taf(rec,wt,"wcreatedibpatternbrush_srcbm16_set");    
    
       rec = wselectobject_set(brush, wht);                   taf(rec,wt,"wselectobject_set");

       lr = point16_set(30*i,30*i);
       rclBox = U_RECT16_set(ul,lr);
       point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4160));
       rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16); taf(rec,wt,"U_WMRRECTANGLE_set");
       free(point16);
       rec = wdeleteobject_set(&brush, wht);                  taf(rec,wt,"wdeleteobject_set");


       i++;
       rec = wcreatepatternbrush_set(&brush, wht, Bm16, px);  taf(rec,wt,"wcreatepatternbrush_set");
       free(Bm16);
    
       rec = wselectobject_set(brush, wht);                   taf(rec,wt,"wselectobject_set");

       lr = point16_set(30*i,30*i);
       rclBox = U_RECT16_set(ul,lr);
       point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4160));
       rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16); taf(rec,wt,"U_WMRRECTANGLE_set");
       free(point16);
       rec = wdeleteobject_set(&brush, wht);                  taf(rec,wt,"wdeleteobject_set");

       
    } /* LODRAW_BLOCKERS */

    free(rgba_px);
    free(px);
    free(Bmi);

    /* *** stroke ***
     WMF does not support pattern stroke, draw filler to show the missing function
    */

    rec = wselectobject_set(brush_null, wht);      taf(rec,wt,"wselectobject_set");

    for(i=k=0;i<3;i++){
       if(     i==0){  } // elpBrushStyle = U_BS_DIBPATTERN;   Imports into PPT correctly.
       else if(i==1){  } // elpBrushStyle = U_BS_DIBPATTERNPT; Imports into PPT correctly.
       else if(i==2){  } // elpBrushStyle = U_BS_PATTERN;      Displays as Solid using TextColor
       for(j=0;j<2;j++,k++){
          if(j==0){    // solid line works
             // elpNumEntries = 0;
             // elpStyleEntry = NULL;
          }
          else {       // dashed line does not, nothing is drawn
             // elpNumEntries = 4;
             // elpStyleEntry = (uint32_t *)&(pl12[3]);
          }
          rec = wselectobject_set(pen_black_1, wht);     taf(rec,wt,"wselectobject_set");
          rec = U_WMRMOVETO_set(point16_set(6600+k*330, 4150));                 taf(rec,wt,"U_WMRMOVETOEX_set");
          rec = U_WMRLINETO_set(point16_set(6600+k*330, 4870));                 taf(rec,wt,"U_WMRLINETO_set");

 
          rec = wselectobject_set(pen_darkred_200, wht);   taf(rec,wt,"wselectobject_set");
          rec = U_WMRMOVETO_set(point16_set(6600+k*330, 4310));                 taf(rec,wt,"U_WMRMOVETOEX_set");  /* not as tall as for EMF, line may have rounded caps  */
          rec = U_WMRLINETO_set(point16_set(6600+k*330, 4710));                 taf(rec,wt,"U_WMRLINETO_set");

       }
    }

    /* ***************    test mono fill  *************** */
     
    rec = wselectobject_set(pen_black_1, wht);            taf(rec,wt,"wselectobject_set");

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

    /* use dibcreatepatternbrush_srcdib_set */
    rec = wcreatedibpatternbrush_srcdib_set(&brush, wht, U_DIB_RGB_COLORS, Bmi, cbPx, px);
    taf(rec,wt,"wcreatedibpatternbrushpt_set");
    rec = wselectobject_set(brush, wht);              taf(rec,wt,"wselectobject_set");

    ul = point16_set(0,0);
    for(i=1;i<=10;i++){
      lr = point16_set(30*i,30*i);
      rclBox = U_RECT16_set(ul,lr);
      point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4520));
      rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16); taf(rec,wt,"U_WMRRECTANGLE_set");
      free(point16);
    }
    rec = wdeleteobject_set(&brush, wht);  taf(rec,wt,"wdeleteobject_set");
    
    if(!(mode & LODRAW_BLOCKERS)){  /* LODRAW does not like bitmap16 */
        /* use dibcreatepatternbrush_srcbm16_set */

       Bm16 = U_BITMAP16_set(
           Bmi->bmiHeader.biBitCount,    //!<  bitmap type.?????  No documentation on what this field should hold!
           Bmi->bmiHeader.biWidth,       //!<  bitmap width in pixels.
           Bmi->bmiHeader.biHeight,      //!<  bitmap height in scan lines.
           4,                            //!<  alignment in array, this is a DIB, so 4
           Bmi->bmiHeader.biBitCount,    //!<  number of adjacent color bits on each plane (R bits + G bits + B bits ????)
           px                            //!<  bitmap pixel data. Bytes contained = (((Width * BitsPixel + 15) >> 4) << 1) * Height
       );
       rec = wcreatedibpatternbrush_srcbm16_set(&brush, wht, U_DIB_RGB_COLORS, Bm16);  
       taf(rec,wt,"wcreatedibpatternbrush_srcbm16_set");    
       free(Bm16);
       rec = wselectobject_set(brush, wht);              taf(rec,wt,"wselectobject_set");

       lr = point16_set(30*i,30*i);
       rclBox = U_RECT16_set(ul,lr);
       point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4520));
       rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16); taf(rec,wt,"U_WMRRECTANGLE_set");
       free(point16);

       rec = wdeleteobject_set(&brush, wht);  taf(rec,wt,"wdeleteobject_set");
    } /* LODRAW_BLOCKERS */

    free(rgba_px);
    free(px);
    free(Bmi);

    /* *** stroke ***
     WMF does not support pattern stroke, draw filler to show the missing function
    */

    rec = wselectobject_set(brush_null, wht);    taf(rec,wt,"wselectobject_set");

    for(i=k=0;i<3;i++){
       if(     i==0){ } // elpBrushStyle = U_BS_DIBPATTERN;   This uses the color map.  Imports into PPT that way too, which is odd, because the fill doesn't import.
       else if(i==1){ } // elpBrushStyle = U_BS_DIBPATTERNPT; This uses the color map.  Imports into PPT that way too, which is odd, because the fill doesn't import.
       else if(i==2){ } // elpBrushStyle = U_BS_PATTERN;      This uses Text/Bk colors instead of grey, like the fill tests above.
       for(j=0;j<2;j++,k++){
          if(j==0){    // solid line works
             // elpNumEntries = 0;
             // elpStyleEntry = NULL;
          }
          else {       // dashed line does not, nothing is drawn
             // elpNumEntries = 4;
             // elpStyleEntry = (uint32_t *)&(pl12[3]);
          }
          rec = wselectobject_set(pen_black_1, wht);               taf(rec,wt,"wselectobject_set");
          rec = U_WMRMOVETO_set(point16_set(9300+k*330, 4150));    taf(rec,wt,"U_WMRMOVETOEX_set");
          rec = U_WMRLINETO_set(point16_set(9300+k*330, 4870));    taf(rec,wt,"U_WMRLINETO_set");

          rec = wselectobject_set(pen_darkred_200, wht);           taf(rec,wt,"wselectobject_set");
          rec = U_WMRMOVETO_set(point16_set(9300+k*330, 4310));    taf(rec,wt,"U_WMRMOVETOEX_set");
          rec = U_WMRLINETO_set(point16_set(9300+k*330, 4710));    taf(rec,wt,"U_WMRLINETO_set");

       }
    }

  /* draw some rectangles, then try floodfill/setpixel  */

  rec = wselectobject_set(brush_purple, wht);            taf(rec,wt,"wselectobject_set");
  rec = wselectobject_set(pen_black_1,  wht);            taf(rec,wt,"wselectobject_set");
  ul = point16_set(0,0);
  lr = point16_set(100,100);
  rclBox = U_RECT16_set(ul,lr);
  for (i=0; i<5; i++){
    point16 = point16_transform((U_POINT16 *) &rclBox, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200+i*330, 7000));
    rec = U_WMRRECTANGLE_set(*(U_RECT16 *)point16); taf(rec,wt,"U_WMRRECTANGLE_set");
    free(point16);
  }

  rec = wselectobject_set(brush_gray, wht);            taf(rec,wt,"wselectobject_set");  // flood with this color

/*
  Broken in XP Preview, does not fill the rectangle.  
*/
  rec = U_WMRFLOODFILL_set(U_FLOODFILLBORDER,      colorref_set(0,0,0),         point16_set(220+0*330, 7020));//limited by black edge
  taf(rec,wt,"U_WMRFLOODFILL_set");
  rec = U_WMREXTFLOODFILL_set(U_FLOODFILLBORDER,   colorref_set(0,0,0),         point16_set(220+1*330, 7020));//limited by black edge
  taf(rec,wt,"U_WMREXTFLOODFILL_set");
/*
  Broken in XP Preview, fills entire window.  
  rec = U_WMRFLOODFILL_set(U_FLOODFILLSURFACE,     colorref_set(196, 127, 255), point16_set(220+2*330, 7020));//limited to purple center
  taf(rec,wt,"U_WMRFLOODFILL_set");
*/
  rec = U_WMREXTFLOODFILL_set(U_FLOODFILLSURFACE,  colorref_set(196, 127, 255), point16_set(220+3*330, 7020));//limited to purple center
  taf(rec,wt,"U_WMREXTFLOODFILL_set");

  rec = U_WMRSETPIXEL_set(colorref_set(0, 1, 2), point16_set(220+4*330, 7020));
  taf(rec,wt,"U_WMRSETPIXEL_set");
  
  /* create a region, fill it
     Windows XP preview shows nothing for this.
  */
  for(i=0; i<50; i++){ ScanLines[i] = point16_set(200 -i*2, 200+i*2); }
  scans  = U_SCAN_set(50,7000,7099,(uint16_t *)ScanLines);
  Region = U_REGION_set(
       U_SIZE_REGION + 50*2*2, //!< aScans in bytes + regions size in bytes (size of this header plus all U_SCAN objects?)
       1,             //!< number of scan objects in region (docs say scanlines, but then no way to add sizes)
       10000,          //!< ???? largest number of points in any scan, what is a "point" here?
       U_RECT16_set(point16_set(200,7000),point16_set(2200,8000)),  //!< bounding rectangle
       (uint16_t *)scans              //!< series of U_SCAN objects to append. This is also an array of uint16_t, but should be handled as a bunch of U_SCAN objects tightly packed into the buffer.
   );   
   rec = wcreateregion_set(&reg,  wht, Region);   taf(rec,wt,"wcreateregion_set");
   reg_1=reg;
   free(scans);
   free(Region);
   rec = U_WMRFILLREGION_set(reg_1, brush_gray);   taf(rec,wt,"U_WMRFILLREGION_set");

    /* ***************    test background variants with combined fill, stroke, text  *************** */
    
    lb  = U_WLOGBRUSH_set(U_BS_HATCHED, colorref_set(63, 127, 255), U_HS_CROSS);
    rec = wcreatebrushindirect_set(&brush, wht,lb);   taf(rec,wt,"wcreatebrushindirect_set");
    rec = wselectobject_set(brush, wht);              taf(rec,wt,"wselectobject_set");

    /* no pen test, WMF pens do not have fill */

    /* use a font that was already created at the beginning */
    rec = wselectobject_set(font_courier_60, wht);  taf(rec,wt,"wselectobject_set");

    rec = U_WMRSETTEXTALIGN_set(U_TA_CENTER | U_TA_BASEBIT);           taf(rec,wt,"U_WMRSETTEXTALIGN_set");
    
    offset=5000;
    for(i=0; i<2; i++){         /* both bkmode*/
       if(i){ rec = U_WMRSETBKMODE_set(U_TRANSPARENT); cbuf[0]='\0'; strcat(cbuf,"bk:- "); }
       else { rec = U_WMRSETBKMODE_set(U_OPAQUE);      cbuf[0]='\0'; strcat(cbuf,"bk:+ "); }
       taf(rec,wt,"U_WMRSETBKMODE_set");

       for(j=0; j<2; j++){      /* two bkcolors         R & turQuoise */
          if(j){ rec = U_WMRSETBKCOLOR_set(colorref_set(255, 0,   0)); cbuf[5]='\0'; strcat(cbuf,"bC:R ");  }
          else { rec = U_WMRSETBKCOLOR_set(colorref_set(0, 255, 255)); cbuf[5]='\0'; strcat(cbuf,"bC:Q ");  }
          taf(rec,wt,"U_WMRSETBKCOLOR_set");

          for(k=0; k<2; k++){   /* two text colors      G & Black */
             if(k){ rec = U_WMRSETTEXTCOLOR_set(colorref_set(  0, 127,   0)); cbuf[10]='\0'; strcat(cbuf,"tC:G "); }
             else { rec = U_WMRSETTEXTCOLOR_set(colorref_set(  0,   0,   0)); cbuf[10]='\0'; strcat(cbuf,"tC:K "); }
             taf(rec,wt,"U_WMRSETTEXTCOLOR_set");

             draw_textrect(8000,offset,700,300,cbuf, 60, wt);  
             offset += 400;  
          }
       }
    }

    // restore original settings
    rec = U_WMRSETBKMODE_set(U_TRANSPARENT);                 taf(rec,wt,"U_WMRSETBKMODE_set");
    rec = U_WMRSETTEXTCOLOR_set(colorref_set(0,0,0));        taf(rec,wt,"U_WMRSETTEXTCOLOR_set");
    rec = U_WMRSETBKCOLOR_set(colorref_set(0,255,255));      taf(rec,wt,"U_WMRSETBKCOLOR_set");
    rec = U_WMRSETTEXTALIGN_set(U_TA_DEFAULT);               taf(rec,wt,"U_WMRSETTEXTALIGN_set");

    /* ***************    test variants of background, hatch pattern, hatchcolor on start/end path  *************** */
    
    /* Note, AFAIK this is not a valid operation and no tested application can actually draw this.  Applications should at least not explode
       when they see it.  */

    offset = 5000;
    textlabel(40, "Path contains invalid operations.",   8800, offset, font_courier_30, wt, wht); offset+=50;
    textlabel(40, "Any graphic produced is acceptable.", 8800, offset, font_courier_30, wt, wht); offset+=50;
    textlabel(40, "Rendering program should not crash.", 8800, offset, font_courier_30, wt, wht); offset+=50;
    textlabel(40, "(No WMF equivalent for EMF test.)",   8800, offset, font_courier_30, wt, wht); offset+=100;
    not_in_wmf(1.5,9000,offset, pen_red_10, brush_darkred, wt, wht);  /* WMF has no bezierto */


    /* Test clipping regions */
    if(!(mode & NO_CLIP_TEST)){
      test_clips(13250, 1400, font_courier_30, pen_red_1, pen_black_1, brush_gray, brush_null, wt, wht);
    }

/* ************************************************* */

//
//  careful not to call anything after here twice!!! 
//

    rec = U_WMREOF_set();
    taf(rec,wt,"U_WMREOF_set");

    /* Test the endian routines (on either Big or Little Endian machines).
    This must be done befoe the call to wmf_finish, as that will swap the byte
    order of the WMF data before it writes it out on a BE machine.  */
    
#if 1 
    string = (char *) malloc(wt->used);
    if(!string){
       printf("Could not allocate enough memory to test u_wmf_endian() function\n");
    }
    else {
       memcpy(string,wt->buf,wt->used);
       status = 0;
       if(!U_wmf_endian(wt->buf,wt->used,1)){
          printf("Error in byte swapping of completed WMF, native -> reverse\n");
       }
       if(!U_wmf_endian(wt->buf,wt->used,0)){
          printf("Error in byte swapping of completed WMF, reverse -> native\n");
       }
       if(rgba_diff(string,wt->buf,wt->used, 0)){ 
          printf("Error in u_wmf_endian() function, round trip byte swapping does not match original\n");
       }
       free(string);
    }
#endif // swap testing

    status=wmf_finish(wt);
    if(status){ printf("wmf_finish failed: %d\n", status); }
    else {      printf("wmf_finish sucess\n");             }

    wmf_free(&wt);
    wmf_htable_free(&wht);

  exit(EXIT_SUCCESS);
}
