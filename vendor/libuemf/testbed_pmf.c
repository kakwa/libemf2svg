/**
 Example progam used for exercising the libUEMF functions.  
 Produces a single output file: test_libuemf_p.emf
 Single command line parameter, hexadecimal bit flag.
   1  Disable tests that block EMF import into PowerPoint (dotted lines)
   2  Enable tests that block EMF being displayed in Windows Preview (currently, GradientFill)
   4  Use a rotated, scaled, offset world transform
   8  Disable clipping tests.
   Default is 0, no option set.

 Compile with 
 
    gcc -g -O0 -o testbed_pmf -std=c99 -Wall -pedantic -I. testbed_pmf.c uemf.c uemf_endian.c uemf_utf.c upmf.c upmf.h -lm 

 or

    gcc -g -O0 -o testbed_pmf -std=c99 -DU_VALGRIND -Wall -pedantic -I. testbed_pmf.c uemf.c uemf_endian.c uemf_utf.c upmf.c upmf.h -lm 

 The latter from enables code which lets valgrind check each record for
 uninitialized data.
 
*/

/* If Version or Date are changed also edit the text labels for the output.

File:      testbed_pmf.c
Version:   0.0.7
Date:      13-MAY-2020
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2020 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include "upmf.h"

#define PPT_BLOCKERS     1
#define PREVIEW_BLOCKERS 2
#define WORLDXFORM_TEST  4
#define NO_CLIP_TEST     8
#define ALPHA_YES        1
#define ALPHA_NO         0
#define CMP_IMG_YES      1
#define CMP_IMG_NO       0

/* definitions for slots in the object table  */
#define OBJ_FONT_TCOMMENT   2
#define OBJ_FONT_OTHER      3
#define OBJ_SF_TCOMMENT     4
#define OBJ_SF_OTHER        5
#define OBJ_PEN_GROUP1     20 /* variable */
#define OBJ_PEN_GROUP2     21 /* variable */
#define OBJ_PEN_BLACK_1    22 /*   fixed  */
#define OBJ_PEN_FUCHSIA_1  23 /* variable */
#define OBJ_PEN_RED_10     24 /*   fixed  */
#define OBJ_PEN_BLACK_20   25 /* variable */
#define OBJ_PEN_RED_1      26 /*   fixed  */
#define OBJ_BRUSH_GROUP1   30 /* variable */
#define OBJ_PATH_1         40 /* variable */
#define OBJ_IMG_1          50 /* variable */
#define OBJ_IA             51 /* variable */
#define OBJ_REGION_1       60 /* variable */

#define STRALLOC        64

void printf_and_flush(const char *string){
   printf(string);
   fflush(stdout);
}

void IfNullPtr(void *po, int LineNo, char *text){
  if(!po){ 
     printf("Line:%d %s",LineNo,text); 
     fflush(stdout);
     exit(EXIT_FAILURE);
  }
}

void IfNotTrue(int Val, int LineNo, char *text){
  if(!Val){ 
     printf("Line:%d %s",LineNo,text); 
     fflush(stdout);
     exit(EXIT_FAILURE);
  }
}

/* use this to find uninitialized data*/
void DumpPo(U_PSEUDO_OBJ *po, char *text){
  unsigned int i;
  unsigned long int us,uu;
  uint8_t *ptr = (uint8_t *)po->Data;
  printf("DEBUG %s\n", text); fflush(stdout);
  us = po->Size; /* printing size_t portably is a pain, this avoids the issue */
  uu = po->Used;
  printf("DEBUG Data:%p Size:%lu Used:%lu",po->Data, us, uu);fflush(stdout);
  for(i=0; i < po->Used; i++, ptr++){
     printf("DEBUG %5.5d %2.2X\n",i,*ptr); fflush(stdout);
  } 
}


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

/* accept the load of an EMF+ record, pack it into an EMF comment record, and append it */
void paf(EMFTRACK *et,  U_PSEUDO_OBJ *sum, U_PSEUDO_OBJ *po, char *text){  // Test, append, free
    unsigned long int uu;
    char *rec;
    uu = po->Used;                                          /* printing size_t portably is a pain, this avoids the issue */
    if(!po){ printf("%s failed",text);                }
    else {   printf("%s recsize: %lu",text, uu + 16); }
    sum->Used = 0;                                          /* clean it out, retaining allocated memory         */
    sum       = U_PO_append(sum, "EMF+", 4);                /* indicates that this comment holds an EMF+ record */
    sum       = U_PO_append(sum,  po->Data, po->Used);      /* the EMF+ record itself                           */
    U_PO_free(&po);                                         /* delete the PseudoObject                          */
    rec       = U_EMRCOMMENT_set(sum->Used, sum->Data);     /* stuff it into the EMF comment                    */
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
void FillImage(char *px, int w, int h, int stride, int alpha){
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
          if(alpha){ a = (xm > yp ? yp : xm); }
          else{      a = 255;                 }
          color = U_RGBA(r,g,b,a);
          memcpy(px,&color,4);
       }
       px += pad;
    }
}

/* 
Draw a colored rectangle onto an RGBA image.
Used for testing ImageEffects operations.
*/
void DrawImageRect(char *px, int stride, int ulx, int uly, int rw, int rh, U_COLORREF color){
int   j,i;
char *lpx;
    px += stride * uly;
    for(j=0; j<rh; j++, px += stride){
       lpx = px + 4*ulx;
       for(i=0; i<rw; i++, lpx+=4){
          memcpy(lpx, &color, 4);
       }
    }
}

/* 
Fill a rectangular RGBA image with gradients then draw some sharp rectangles on top of it. 
Used for testing ImageEffects operations.
*/
void FillImage2(char *px, int w, int h, int stride, int alpha){
int lh,ly;
double dw = 2;
double dk = 255.0;
uint8_t k;
    FillImage(px, w, h, stride, alpha);
    ly = h/32;
    lh = 0;
    for(; lh + ly < h; ly += lh+3){
       k = U_ROUND(dk);
       lh = U_ROUND(dw);
       DrawImageRect(px, stride, w/4, ly, w/2, lh, U_RGBA(k,k,k,255));
       dw *= 1.25;
       dk /= 1.4;
    }  
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



void textlabel(U_PSEUDO_OBJ *sum, U_FLOAT size, const char *string, uint32_t x, uint32_t y,
    int ax, int ay, U_PSEUDO_OBJ *poColor, EMFTRACK *et){
    uint16_t      *FontName;
    uint16_t      *text16;
    int            slen;
    U_PSEUDO_OBJ  *po;
    U_PSEUDO_OBJ  *poRect;
    U_PSEUDO_OBJ  *poFont;
    U_FLOAT        rx,ry,rw,rh;
    
    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    slen = strlen("Courier New");
    poFont = U_PMF_FONT_set(U_PMF_GRAPHICSVERSIONOBJ_set(2), size, U_UT_Pixel, U_FS_None, slen, FontName);
    IfNullPtr(poFont,__LINE__,"OOPS on U_PMF_FONT_set\n");
    free(FontName);

    po        = U_PMR_OBJECT_PO_set(OBJ_FONT_TCOMMENT, poFont); /* font to use */
    IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, sum, po, "set font in textlabel");
    U_PO_free(&poFont);

    rw = 4*size*slen; /* This could probably be any value */
    rh = size;
    switch (ax){
       case U_SA_Far:    rx = x-rw;     break;
       case U_SA_Center: rx = x-rw/2.0; break;
       case U_SA_Near:
       default:          rx = x;        break;
    }
    switch (ay){
       case U_SA_Far:    ry = y+rh;     break;
       case U_SA_Center: ry = y+rh/2.0; break;
       case U_SA_Near:
       default:          ry = y;        break;
    }

    poRect = U_PMF_RECTF4_set(rx, ry, rw, rh); 
    text16 = U_Utf8ToUtf16le(string, 0, NULL);
    slen = strlen(string);
    po = U_PMR_DRAWSTRING_set(OBJ_FONT_TCOMMENT, poColor, OBJ_SF_TCOMMENT, slen, poRect, text16);
    IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWSTRING_set\n");
    paf(et, sum, po, "U_PMR_DRAWSTRING_set in textlabel");

    U_PO_free(&poRect);
    free(text16);
}

void spintext(U_PSEUDO_OBJ *sum, U_FLOAT size, U_FLOAT x, U_FLOAT y, int ax, int ay, U_PSEUDO_OBJ *poColor, EMFTRACK *et){
    int                 i;
    char               *string;
    U_FLOAT             Angle;
    U_PSEUDO_OBJ       *po;
    
    string = malloc(STRALLOC);
    for(i=0; i<360; i+=30){
       Angle = i;
       po = U_PMR_ROTATEWORLDTRANSFORM_set(U_XM_PostX, Angle);
       paf(et, sum, po, "U_PMR_ROTATEWORLDTRANSFORM_set in spintext");
       po = U_PMR_TRANSLATEWORLDTRANSFORM_set(U_XM_PostX, x, y);
       paf(et, sum, po, "U_PMR_TRANSLATEWORLDTRANSFORM_set in spintext");

       sprintf(string,"....Degrees:%d",i);
       textlabel(sum, size, string, 0,0, ax, ay, poColor, et);
       po = U_PMR_TRANSLATEWORLDTRANSFORM_set(U_XM_PostX, -x, -y);
       paf(et, sum, po, "U_PMR_TRANSLATEWORLDTRANSFORM_set in spintext");
       po = U_PMR_ROTATEWORLDTRANSFORM_set(U_XM_PostX, -Angle);
       paf(et, sum, po, "U_PMR_ROTATEWORLDTRANSFORM_set in spintext");
    }
    free(string);
}
    
void label_column(U_PSEUDO_OBJ *sum, int x1, int y1, int ax, int ay, U_PSEUDO_OBJ *poColor, EMFTRACK *et){
    textlabel(sum, 40, "STRETCHDIBITS 1", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "BITBLT        1", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "STRETCHBLT    1", x1, y1, ax, ay, poColor, et);          y1 += 240;
    textlabel(sum, 40, "STRETCHDIBITS 2", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "BITBLT        2", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "STRETCHBLT    2", x1, y1, ax, ay, poColor, et);          y1 += 240;
    textlabel(sum, 40, "STRETCHDIBITS 3", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "BITBLT        3", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "STRETCHBLT    3", x1, y1, ax, ay, poColor, et);          y1 += 240;
    textlabel(sum, 40, "STRETCHDIBITS 4", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "BITBLT        4", x1, y1, ax, ay, poColor, et);          y1 += 220;
    textlabel(sum, 40, "STRETCHBLT    4", x1, y1, ax, ay, poColor, et);       // y1 += 220;// clang static analyzer does not like this
    return;
}

void label_row(U_PSEUDO_OBJ *sum, int x1, int y1, int ax, int ay, U_PSEUDO_OBJ *poColor, EMFTRACK *et){
    textlabel(sum, 30, "+COLOR32 ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+COLOR24 ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+COLOR16 ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "-COLOR16 ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+COLOR8  ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+COLOR4  ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+MONO    ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "-MONO    ",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+COLOR8 0",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+COLOR4 0",       x1, y1, ax, ay, poColor, et);          x1 += 220;
    textlabel(sum, 30, "+MONO   0",       x1, y1, ax, ay, poColor, et);       // x1 += 220;// clang static analyzer does not like this
    return;
}

void image_column(EMFTRACK *et, int x1, int y1, int w, int h, int IsCompressed, U_PMF_BITMAP *Bs, int numCt, U_PMF_ARGB *ct, uint32_t cbPx, char *px, U_PSEUDO_OBJ *poac){
   int   step=0;
   U_PSEUDO_OBJ *po;
   U_PSEUDO_OBJ *poPal;
   U_PSEUDO_OBJ *poBm;
   U_PSEUDO_OBJ *poBmd;
   U_PSEUDO_OBJ *poImg;
   U_PSEUDO_OBJ *poIa;
   U_PSEUDO_OBJ *poRectf;
   U_PSEUDO_OBJ *poRectfs;
   U_PSEUDO_OBJ *poPoints;
   U_PMF_POINTF pl3[3];
   U_PMF_POINTF *points;
   uint32_t Version = U_PMF_GRAPHICSVERSIONOBJ_set(2);
   int flags;
    
    /* Set up the Image, Image Attributes */
    if(Bs->PxFormat == U_PF_16bppGrayScale){ flags = U_PLTS_GrayScale; }
    else {                                   flags = U_PLTS_None;      }
    poPal = U_PMF_PALETTE_set(flags, numCt, ct);
       /* poPal will be NULL if there is no palette */
    if(IsCompressed){
       poBmd = U_PMF_COMPRESSEDIMAGE_set(cbPx, px);
         IfNullPtr(poBmd,__LINE__,"OOPS on U_PMF_BITMAPDATA_set\n");
       poRectfs = U_PMF_RECTF4_set(0, 0, 10, 10);
         IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
     }
    else {
       poBmd = U_PMF_BITMAPDATA_set(poPal, cbPx, px);
         IfNullPtr(poBmd,__LINE__,"OOPS on U_PMF_BITMAPDATA_set\n");
       poRectfs = U_PMF_RECTF4_set(0, 0, Bs->Width, Bs->Height);
         IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
    }
    poBm = U_PMF_BITMAP_set(Bs, poBmd);
       IfNullPtr(poBm,__LINE__,"OOPS on U_PMF_BITMAP_set\n");
    poImg = U_PMF_IMAGE_set(Version, poBm);
       IfNullPtr(poImg,__LINE__,"OOPS on U_PMF_IMAGE_set\n");
    po = U_PMR_OBJECT_PO_set(OBJ_IMG_1, poImg);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_IMG_1");
        U_PO_free(&poImg);
        U_PO_free(&poBm);
        U_PO_free(&poBmd);
        U_PO_free(&poPal);

    // Make an image attribute object
    poIa = U_PMF_IMAGEATTRIBUTES_set(Version, U_WM_Tile, 0, 0);
       IfNullPtr(poIa,__LINE__,"OOPS on U_PMF_BITMAP_set\n");
    po = U_PMR_OBJECT_PO_set(OBJ_IA, poIa);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_IA");
        U_PO_free(&poIa);
 
    // all methods use this

    // Draw the image: method 1
    poRectf =  U_PMF_RECTF4_set(x1, y1, w, h);
       IfNullPtr(poRectf,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
    po = U_PMR_DRAWIMAGE_set(OBJ_IMG_1, OBJ_IA, U_UT_World, poRectfs, poRectf);
    paf(et, poac, po, "U_PMR_DRAWIMAGE_set");
        U_PO_free(&poRectf);
    step += 220;

    // Draw the image: method 2
    pl3[0] = (U_PMF_POINTF) { 0,0 };
    pl3[1] = (U_PMF_POINTF) { w,0 };
    pl3[2] = (U_PMF_POINTF) { 0,h };
    points = pointfs_transform(pl3, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, x1, y1 + step));
    poPoints = U_PMF_POINTF_set(3, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_DRAWIMAGEPOINTS_set(OBJ_IMG_1, U_FILTER_IGNORE, OBJ_IA, U_UT_World, poRectfs, poPoints);
    paf(et, poac, po, "U_PMR_DRAWIMAGE_set");
        U_PO_free(&poPoints);
        free(points);
    step += 220;

    // Draw the image: method 3
    points = pointfs_transform(pl3, 3, xform_alt_set(1.0, 1.0, 90.0, 0.0, x1, y1 + step + h));
    poPoints = U_PMF_POINTF_set(3, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_DRAWIMAGEPOINTS_set(OBJ_IMG_1, U_FILTER_IGNORE, OBJ_IA, U_UT_World, poRectfs, poPoints);
    paf(et, poac, po, "U_PMR_DRAWIMAGE_set");
        U_PO_free(&poPoints);
        free(points);

    // finally done with this
    U_PO_free(&poRectfs);
}

/* this tests image effects 

   WARNING!  Windows XP Preview does not show filter effects, whether or not U_PPF_E is set.   They are visible if the EMF+
   file is inserted as an image into PowerPoint.

*/
void image_column2(EMFTRACK *et, int x1, int y1, int w, int h, int IsCompressed, U_PMF_BITMAP *Bs, int numCt, U_PMF_ARGB *ct, uint32_t cbPx, char *px, U_PSEUDO_OBJ *poac){
   int   step=0;
   U_PSEUDO_OBJ *po;
   U_PSEUDO_OBJ *poPal;
   U_PSEUDO_OBJ *poBm;
   U_PSEUDO_OBJ *poBmd;
   U_PSEUDO_OBJ *poImg;
   U_PSEUDO_OBJ *poIa;
   U_PSEUDO_OBJ *poIe;
   U_PSEUDO_OBJ *poRectfs;
   U_PSEUDO_OBJ *poPoints;
   U_PMF_POINTF pl3[3];
   U_PMF_POINTF *points;
   uint32_t Version = U_PMF_GRAPHICSVERSIONOBJ_set(2);
   int flags;
    
    /* Set up the Image, Image Attributes */
    if(Bs->PxFormat == U_PF_16bppGrayScale){ flags = U_PLTS_GrayScale; }
    else {                                   flags = U_PLTS_None;      }
    poPal = U_PMF_PALETTE_set(flags, numCt, ct);
       /* poPal will be NULL if there is no palette */
    if(IsCompressed){
       poBmd = U_PMF_COMPRESSEDIMAGE_set(cbPx, px);
         IfNullPtr(poBmd,__LINE__,"OOPS on U_PMF_BITMAPDATA_set\n");
       poRectfs = U_PMF_RECTF4_set(0, 0, 10, 10);
         IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
     }
    else {
       poBmd = U_PMF_BITMAPDATA_set(poPal, cbPx, px);
         IfNullPtr(poBmd,__LINE__,"OOPS on U_PMF_BITMAPDATA_set\n");
       poRectfs = U_PMF_RECTF4_set(0, 0, Bs->Width, Bs->Height);
         IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
    }
    poBm = U_PMF_BITMAP_set(Bs, poBmd);
       IfNullPtr(poBm,__LINE__,"OOPS on U_PMF_BITMAP_set\n");
    poImg = U_PMF_IMAGE_set(Version, poBm);
       IfNullPtr(poImg,__LINE__,"OOPS on U_PMF_IMAGE_set\n");
    po = U_PMR_OBJECT_PO_set(OBJ_IMG_1, poImg);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_IMG_1");
        U_PO_free(&poImg);
        U_PO_free(&poBm);
        U_PO_free(&poBmd);
        U_PO_free(&poPal);

    // Make an image attribute object
    poIa = U_PMF_IMAGEATTRIBUTES_set(Version, U_WM_Tile, 0, 0);
       IfNullPtr(poIa,__LINE__,"OOPS on U_PMF_BITMAP_set\n");
    po = U_PMR_OBJECT_PO_set(OBJ_IA, poIa);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_IA");
        U_PO_free(&poIa);
 
    pl3[0] = (U_PMF_POINTF) { 0,0 };
    pl3[1] = (U_PMF_POINTF) { w,0 };
    pl3[2] = (U_PMF_POINTF) { 0,h };

    // Draw the image, no image effects 
    points = pointfs_transform(pl3, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, x1, y1 + step));
    poPoints = U_PMF_POINTF_set(3, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_DRAWIMAGEPOINTS_set(OBJ_IMG_1, U_FILTER_IGNORE, OBJ_IA, U_UT_World, poRectfs, poPoints);
    paf(et, poac, po, "U_PMR_DRAWIMAGE_set");
        U_PO_free(&poPoints);
        free(points);
    step += 220;

   // Draw the image, blur effects 
   
    poIe = U_PMF_IE_BLUR_set(15.0,0);
       IfNullPtr(po,__LINE__,"OOPS on U_PMF_IE_BLUR_set\n");
    po = U_PMR_SERIALIZABLEOBJECT_set(poIe);
    paf(et, poac, po, "U_PMR_SERIALIZABLEOBJECT_set");
        U_PO_free(&poIe);
   
    points = pointfs_transform(pl3, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, x1, y1 + step));
    poPoints = U_PMF_POINTF_set(3, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_DRAWIMAGEPOINTS_set(OBJ_IMG_1, U_FILTER_APPLY, OBJ_IA, U_UT_World, poRectfs, poPoints);
    paf(et, poac, po, "U_PMR_DRAWIMAGE_set");
        U_PO_free(&poPoints);
        free(points);
    step += 220;

   // Draw the image, brightness/contrast effects 
   
    poIe = U_PMF_IE_BRIGHTNESSCONTRAST_set(255, 100);
       IfNullPtr(po,__LINE__,"OOPS on U_PMF_IE_BRIGHTNESSCONTRAST_set\n");
    po = U_PMR_SERIALIZABLEOBJECT_set(poIe);
    paf(et, poac, po, "U_PMR_SERIALIZABLEOBJECT_set");
        U_PO_free(&poIe);
   
    points = pointfs_transform(pl3, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, x1, y1 + step));
    poPoints = U_PMF_POINTF_set(3, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_DRAWIMAGEPOINTS_set(OBJ_IMG_1, U_FILTER_APPLY, OBJ_IA, U_UT_World, poRectfs, poPoints);
    paf(et, poac, po, "U_PMR_DRAWIMAGE_set");
        U_PO_free(&poPoints);
        free(points);
    //step += 220; // clang static analyzer does not like this


    // finally done with this
    U_PO_free(&poRectfs);
}

void grad_star(EMFTRACK *et, int x, int y, int options, U_PSEUDO_OBJ *poac){
   U_PMF_POINTF Pointfs[] = {
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
   U_PMF_POINTF Centerf = {383.5,358.0};
   U_PMF_ARGB SurColors[] = {
      {0xFF,0x00,0x00,0xFF},{0x00,0xFF,0x00,0xFF},
      {0x00,0x00,0xFF,0xFF},{0x00,0xFF,0x00,0xFF},
      {0xFF,0x00,0x00,0xFF},{0x00,0xFF,0x00,0xFF},
      {0x00,0x00,0xFF,0xFF},{0x00,0xFF,0x00,0xFF},
      {0xFF,0x00,0xFF,0xFF},{0x00,0xFF,0x00,0xFF}
   };
   U_PMF_POINTF           *points;
   U_PMF_POINTF           *center;
   U_PMF_TRANSFORMMATRIX   Tm;
   U_PSEUDO_OBJ           *poTm;
   U_PSEUDO_OBJ           *poGradient;
   U_PSEUDO_OBJ           *poPath;
   U_PSEUDO_OBJ           *poBPath;
   U_PSEUDO_OBJ           *poPGBOD;
   U_PSEUDO_OBJ           *poPGBD;
   U_PSEUDO_OBJ           *poBrush;
   U_PSEUDO_OBJ           *poBrushID;
   U_PSEUDO_OBJ           *poColor;
   U_PSEUDO_OBJ           *po;
   U_DPSEUDO_OBJ          *dpath=NULL;
   uint32_t                Flags = U_BD_Transform;

   poColor   = U_PMF_ARGB_set(255,0,0,0);
   textlabel(poac, 40, "Text_____Beneath______Star____Test1", x -30 , y + 250, U_SA_Near, U_SA_Near, poColor, et);

   poBrushID = U_PMF_4NUM_set(OBJ_BRUSH_GROUP1);
      IfNullPtr(poBrushID,__LINE__,"OOPS on U_PMF_4NUM_set\n");

   points = pointfs_transform( Pointfs, 10, xform_alt_set(1.0, 1.0, 0.0, 0.0, x, y));
   center = pointfs_transform(&Centerf,  1, xform_alt_set(1.0, 1.0, 0.0, 0.0, x, y));

   dpath =  U_PATH_create(0, NULL, 0, 0); /* create an empty path*/
   IfNotTrue(U_PATH_polylineto(dpath, 10, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
   IfNotTrue(U_PATH_closepath((dpath)),                                   __LINE__, "OOPS on U_PATH_closepath\n");
   poPath = U_PMF_PATH_set3(U_PMF_GRAPHICSVERSIONOBJ_set(2), dpath);
      IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
   po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
   paf(et, poac, po, "U_PMR_OBJECT_PO_set");

   if(!options){
     Flags |= U_BD_Path;
     poBPath = U_PMF_BOUNDARYPATHDATA_set(poPath);
        IfNullPtr(poBPath,__LINE__,"OOPS U_PMF_BOUNDARYPATHDATA_set\n");
   }
   else {
     /* U_BD_Path is not set for points */
     poBPath = U_PMF_BOUNDARYPOINTDATA_set(10,points);
        IfNullPtr(poBPath,__LINE__,"OOPS U_PMF_BOUNDARYPOINTDATA_set\n");
   }
   poGradient = U_PMF_ARGBN_set(10, SurColors);
      IfNullPtr(poGradient,__LINE__,"OOPS on U_PMF_ARGBN_set\n");
   Tm = (U_PMF_TRANSFORMMATRIX){1 , 0, 0, 1, 0, 0};
   poTm = U_PMF_TRANSFORMMATRIX_set(&Tm);
      IfNullPtr(poTm,__LINE__,"OOPS on U_PMF_TRANSFORMMATRIX_set\n");
   poPGBOD = U_PMF_PATHGRADIENTBRUSHOPTIONALDATA_set(Flags, poTm, NULL, NULL);
      IfNullPtr(poPGBOD,__LINE__,"OOPS on U_PMF_PATHGRADIENTBRUSHOPTIONALDATA_set\n");
   poPGBD = U_PMF_PATHGRADIENTBRUSHDATA_set(Flags, U_WM_Tile, (U_PMF_ARGB) {0x00,0x00,0x00,0xFF}, 
      *center, poGradient, poBPath, poPGBOD);
      IfNullPtr(poPGBD,__LINE__,"OOPS on U_PMF_PATHGRADIENTBRUSHDATA_set\n");
   poBrush   = U_PMF_BRUSH_set(U_PMF_GRAPHICSVERSIONOBJ_set(2), poPGBD);
      IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");
   po = U_PMR_OBJECT_PO_set(OBJ_BRUSH_GROUP1, poBrush);
      IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
   paf(et, poac, po, "U_PMR_OBJECT_PO_set");

   po = U_PMR_FILLPATH_set(OBJ_PATH_1, poBrushID);
      IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLPATH_set\n");
   paf(et, poac, po, "U_PMR_FILLPATH_set");
 
   textlabel(poac, 40, "Text_____Above________Star____Test2", x -30 , y + 450, U_SA_Near, U_SA_Near, poColor, et);

   U_PO_free(&poTm);
   U_PO_free(&poPGBOD);
   U_PO_free(&poBrush);
   U_PO_free(&poPGBD);
   U_PO_free(&poBPath);
   U_PO_free(&poGradient);
   U_PO_free(&poPath);
   free(center);
   free(points);
   U_PO_free(&poBrushID);
   U_PO_free(&poColor);
   U_DPO_free(&dpath);
}

void test_clips(EMFTRACK *et, int x, int y, U_PSEUDO_OBJ *poac){
   U_PSEUDO_OBJ          *po;
   U_PSEUDO_OBJ          *poBrushID;
   U_PSEUDO_OBJ          *poColor;
   U_PSEUDO_OBJ          *poPath;
   U_PSEUDO_OBJ          *poRectfs;
   U_PSEUDO_OBJ          *poRectfs2;
   U_PSEUDO_OBJ          *poRegion;
   U_PSEUDO_OBJ          *poRNPath;
   U_PSEUDO_OBJ          *poRNode;
   U_PSEUDO_OBJ          *poLNode;
   U_PSEUDO_OBJ          *poCNodes;
   U_PSEUDO_OBJ          *poNode;
   U_PSEUDO_OBJ          *poTm;
   U_PMF_RECTF            Rectfs[2];
   int                    i;
   U_DPSEUDO_OBJ         *dpath=NULL;
   U_PMF_TRANSFORMMATRIX  Tm;
   char                   string[STRALLOC];

   poColor   = U_PMF_ARGB_set(255,0,0,0);
   IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");

   /* no clipping */
   grad_star(et, x, y, 0, poac);
   grad_star(et, x, y+700, 1, poac);
   textlabel(poac, 40, "NoClip", x+200 , y - 60, U_SA_Near, U_SA_Near, poColor, et);

   /* rectangle clipping */
   x += 800;
   Rectfs[0] = (U_PMF_RECTF){x+200, y, 200, 1500};
   poRectfs  = U_PMF_RECTFN_set(1,Rectfs);
      IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
   po = U_PMR_DRAWRECTS_set(OBJ_PEN_RED_1, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWRECTS_set\n");
   paf(et, poac, po, "U_PMR_DRAWRECTS_set");

   po = U_PMR_SETCLIPRECT_set(U_CM_Replace, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETCLIPRECT_set\n");
   paf(et, poac, po, "U_PMR_SETCLIPRECT_set");
   U_PO_free(&poRectfs);

   grad_star(et, x, y, 0, poac);
   grad_star(et, x, y+700, 1, poac);

   po = U_PMR_RESETCLIP_set();
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_RESETCLIP\n");
   paf(et, poac, po, "U_PMR_RESETCLIP");

   textlabel(poac, 40, "Clip", x+200 , y - 60, U_SA_Near, U_SA_Near, poColor, et);

   for(i=U_CM_Intersect ; i<= U_CM_Complement; i++){
      /* two rectangle clipping, various combine operations */
      x += 800;
      Rectfs[0] = (U_PMF_RECTF){x+200, y,     200, 1500};
      Rectfs[1] = (U_PMF_RECTF){x+300, y+100, 200,  600};
      dpath =  U_PATH_create(0, NULL, 0, 0); /* create an empty path*/
      U_PATH_moveto(dpath, (U_PMF_POINTF){x+250,y+ 350}, U_PTP_None);
      U_PATH_lineto(dpath, (U_PMF_POINTF){x+550,y+ 250}, U_PTP_None);
      U_PATH_lineto(dpath, (U_PMF_POINTF){x+550,y+1200}, U_PTP_None);
      U_PATH_lineto(dpath, (U_PMF_POINTF){x+250,y+1300}, U_PTP_None);
      U_PATH_closepath(dpath);

      /* draw first and second rects and path */
      poRectfs  = U_PMF_RECTFN_set(1, &Rectfs[0]);
         IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
      po = U_PMR_DRAWRECTS_set(OBJ_PEN_RED_1, poRectfs);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWRECTS_set\n");
      paf(et, poac, po, "U_PMR_DRAWRECTS_set");

      poRectfs2  = U_PMF_RECTFN_set(1, &Rectfs[1]);
         IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
      po = U_PMR_DRAWRECTS_set(OBJ_PEN_RED_1, poRectfs2);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWRECTS_set\n");
      paf(et, poac, po, "U_PMR_DRAWRECTS_set");

      poPath = U_PMF_PATH_set3(U_PMF_GRAPHICSVERSIONOBJ_set(2), dpath);
         IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
      po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
         IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
      paf(et, poac, po, "U_PMR_OBJECT_PO_set");

      po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_RED_1);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
      paf(et, poac, po, "U_PMR_DRAWPATH_set");

      /* set clip with first and second rects and path */ 
      po = U_PMR_SETCLIPRECT_set(U_CM_Replace, poRectfs);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETCLIPRECT_set\n");
      paf(et, poac, po, "U_PMR_SETCLIPRECT_set");

      po = U_PMR_SETCLIPRECT_set(i, poRectfs2);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETCLIPRECT_set\n");
      paf(et, poac, po, "U_PMR_SETCLIPRECT_set");

      po = U_PMR_SETCLIPPATH_set(OBJ_PATH_1, i);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETCLIPPATH_set\n");
      paf(et, poac, po, "U_PMR_SETCLIPPATH_set");

      /* stars */
      grad_star(et, x, y, 0, poac);
      grad_star(et, x, y+700, 1, poac);

      po = U_PMR_RESETCLIP_set();
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_RESETCLIP\n");
      paf(et, poac, po, "U_PMR_RESETCLIP");

      switch(i){
         case U_CM_Replace:    strcpy(string,"Clip Replace   "); break;
         case U_CM_Intersect:  strcpy(string,"Clip Intersect "); break;
         case U_CM_Union:      strcpy(string,"Clip Union     "); break;
         case U_CM_XOR:        strcpy(string,"Clip XOR       "); break;
         case U_CM_Exclude:    strcpy(string,"Clip Exclude   "); break;
         case U_CM_Complement: strcpy(string,"Clip Complement"); break;
      }
      textlabel(poac, 40, string, x+200 , y - 60, U_SA_Near, U_SA_Near, poColor, et);


      U_PO_free(&poRectfs);
      U_PO_free(&poRectfs2);
      U_PO_free(&poPath);
      U_DPO_free(&dpath);
   }
   
   /* test one with a rectangle and two regions.  This is done twice, the left most one uses the
      raw clipping region, and the rightmost one uses offset clip */
   x += 800;
   Rectfs[0] = (U_PMF_RECTF){x+200, y,     200, 1500};
   Rectfs[1] = (U_PMF_RECTF){x+250, y+600, 300,  850};
   dpath =  U_PATH_create(0, NULL, 0, 0); /* create an empty path*/
   U_PATH_moveto(dpath, (U_PMF_POINTF){x+250,y+  50}, U_PTP_None);
   U_PATH_lineto(dpath, (U_PMF_POINTF){x+550,y+ 150}, U_PTP_None);
   U_PATH_lineto(dpath, (U_PMF_POINTF){x+550,y+ 400}, U_PTP_None);
   U_PATH_lineto(dpath, (U_PMF_POINTF){x+250,y+ 500}, U_PTP_None);
   U_PATH_closepath(dpath);
   U_PATH_moveto(dpath, (U_PMF_POINTF){x+250,y+ 950}, U_PTP_None);
   U_PATH_lineto(dpath, (U_PMF_POINTF){x+550,y+ 850}, U_PTP_None);
   U_PATH_lineto(dpath, (U_PMF_POINTF){x+550,y+1400}, U_PTP_None);
   U_PATH_lineto(dpath, (U_PMF_POINTF){x+250,y+1300}, U_PTP_None);
   U_PATH_closepath(dpath);
   poPath = U_PMF_PATH_set2(U_PMF_GRAPHICSVERSIONOBJ_set(2), dpath);
      IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
   po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
      IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
   paf(et, poac, po, "U_PMR_OBJECT_PO_set");

   /* set up the rect and region PseudoObjeccts */
   poRectfs  = U_PMF_RECTF_set(&Rectfs[0]);
      IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
   poRectfs2  = U_PMF_RECTF_set(&Rectfs[1]);
      IfNullPtr(poRectfs2,__LINE__,"OOPS on U_PMF_RECTFN_set\n");

   poRNPath = U_PMF_REGIONNODEPATH_set(poPath);
      IfNullPtr(poRNPath,__LINE__,"OOPS U_PMF_REGIONNODEPATH_set\n");
   poRNode = U_PMF_REGIONNODE_set(U_RNDT_Path, poRNPath);  /* make a U_PMF_REGIONNODE_OID from a path, 1st child node */
      IfNullPtr(poRNode,__LINE__,"OOPS U_PMF_REGIONNODE_set\n");
   poLNode = U_PMF_REGIONNODE_set(U_RNDT_Rect, poRectfs2); /* make a U_PMF_REGIONNODE_OID from a rect, 2nd child node */
      IfNullPtr(poLNode,__LINE__,"OOPS U_PMF_REGIONNODE_set\n");
   poCNodes = U_PMF_REGIONNODECHILDNODES_set(poLNode, poRNode);  /* make a U_PMF_REGIONNODECHILDNODES_OID from two U_PMF_REGIONNODE_OID */
      IfNullPtr(poCNodes,__LINE__,"OOPS U_PMF_REGIONNODECHILDNODES_set\n");
   poNode = U_PMF_REGIONNODE_set(U_RNDT_Or, poCNodes); /* make a U_PMF_REGIONNODE_OID from a U_PMF_REGIONNODECHILDNODES_OID */ 
      IfNullPtr(poNode,__LINE__,"OOPS U_PMF_REGIONNODE_set\n");
   poRegion = U_PMF_REGION_set(U_PMF_GRAPHICSVERSIONOBJ_set(2), 2, poNode); /* number of CHILD nodes (only count U_PMF_REGIONNODE nodes), here two, as shown above */
      IfNullPtr(poRegion,__LINE__,"OOPS U_PMF_REGION_set\n");
   po = U_PMR_OBJECT_PO_set(OBJ_REGION_1, poRegion);
      IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
   paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_REGION_1");

   /* draw rect and region (region has to be fill) */
   /* first draw then where they are for the left set  */
   /* second draw them through a worldtransform matrix for the right set */

   poBrushID = U_PMF_ARGB_set(255, 128, 128, 128);
       IfNullPtr(poBrushID,__LINE__,"OOPS on U_PMF_ARGB_set\n");
   for(i=0;i<2;i++){
       po = U_PMR_DRAWRECTS_set(OBJ_PEN_RED_1, poRectfs);
           IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWRECTS_set\n");
       paf(et, poac, po, "U_PMR_DRAWRECTS_set");

       po = U_PMR_FILLREGION_set(OBJ_REGION_1, poBrushID);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLREGION_set\n");
       paf(et, poac, po, "U_PMR_FILLREGION_set");
   
       if(i==0){
          strcpy(string,"Clip Region");
          textlabel(poac, 40, string, x+200 , y - 60, U_SA_Near, U_SA_Near, poColor, et);

          Tm = (U_PMF_TRANSFORMMATRIX){1,0,0,1,800,0};
          poTm = U_PMF_TRANSFORMMATRIX_set(&Tm);
             IfNullPtr(poTm,__LINE__,"OOPS on U_PMF_TRANSFORMMATRIX_set\n");
          po = U_PMR_MULTIPLYWORLDTRANSFORM_set(U_XM_PostX, poTm);
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETWORLDTRANSFORM_set\n");
             U_PO_free(&poTm);
          paf(et, poac, po, "U_PMR_MULTIPLYWORLDTRANSFORM_set");
       }
       else {
          strcpy(string,"Clip Offset {100,50}"); /* actually it is 900,50, but subtract the 800 instance offset*/
          textlabel(poac, 40, string, x+200 , y - 60, U_SA_Near, U_SA_Near, poColor, et);

          po = U_PMR_RESETWORLDTRANSFORM_set();
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_RESETWORLDTRANSFORM_set\n");
          paf(et, poac, po, "U_PMR_RESETWORLDTRANSFORM_set");
       }
   }

   /* set clip with one rect and the region */ 
   po = U_PMR_SETCLIPRECT_set(U_CM_Replace, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETCLIPRECT_set\n");
   paf(et, poac, po, "U_PMR_SETCLIPRECT_set");

   po = U_PMR_SETCLIPREGION_set(OBJ_REGION_1, U_CM_Union);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETCLIPREGION_set\n");
   paf(et, poac, po, "U_PMR_SETCLIPREGION_set");

   /* stars */
   grad_star(et, x, y, 0, poac);
   grad_star(et, x, y+700, 1, poac);

   /* DO NOT reset clip.  Use OffsetClip moved over by 100 */
   po = U_PMR_OFFSETCLIP_set(900,50);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OFFSETCLIP_set\n");
   paf(et, poac, po, "U_PMR_OFFSETCLIP_set");
   
   /* stars */
   grad_star(et, x+800, y, 0, poac);
   grad_star(et, x+800, y+700, 1, poac);
   
   /* NOW reset the clipping */
   po = U_PMR_RESETCLIP_set();
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_RESETCLIP\n");
   paf(et, poac, po, "U_PMR_RESETCLIP");

   U_PO_free(&poBrushID);
   U_PO_free(&poRNPath);
   U_PO_free(&poRNode);
   U_PO_free(&poLNode);
   U_PO_free(&poCNodes);
   U_PO_free(&poNode);
   U_PO_free(&poRegion);
   U_PO_free(&poRectfs);
   U_PO_free(&poRectfs2);
   U_PO_free(&poPath);
   U_PO_free(&poColor);
   U_DPO_free(&dpath);
}


void gradient_column(EMFTRACK *et, int x1, int y1, int w, int h, U_PSEUDO_OBJ *poac){
    int                           elements = 4;
    U_PMF_RECTF                   Rectfs[3];
    uint32_t                      Version = U_PMF_GRAPHICSVERSIONOBJ_set(2);
    U_PMF_LINEARGRADIENTBRUSHDATA Lgbd;
    U_PMF_TRANSFORMMATRIX         Tm;
    U_PSEUDO_OBJ                 *poBrushID;
    U_PSEUDO_OBJ                 *poTm0;
    U_PSEUDO_OBJ                 *poTm45=NULL;
    U_PSEUDO_OBJ                 *poLgbod;
    U_PSEUDO_OBJ                 *poBd;
    U_PSEUDO_OBJ                 *poBrush;
    U_PSEUDO_OBJ                 *po;
    U_PSEUDO_OBJ                 *poRectfs;
    U_PSEUDO_OBJ                 *poBf;
    U_PSEUDO_OBJ                 *poBc;
    int                           i,j;
    U_PSEUDO_OBJ                 *tpoTm,*tpoBc,*tpoBfH,*tpoBfV;

    Rectfs[0] = (U_PMF_RECTF){x1, y1, w, h}; // notice, here the same as rect in the gradient
    elements = 3;
    Lgbd = (U_PMF_LINEARGRADIENTBRUSHDATA){
       0,                                  // BrushData flags, this is the only field that varies, below
       U_WM_Tile,                          // WrapMode enumeration
       Rectfs[0],                          // Gradient area, gradients will stay "tiled" to this location
       (U_PMF_ARGB){255,0,0,255},          // U_PMF_ARGB  BLUE       Gradient start color
       (U_PMF_ARGB){0,0,255,255},          // U_PMF_ARGB  RED        Gradient end color
       0,                                  // manuals says to ignore, call will replicate the start/end colors
       0                                   // 
    };
    poBrushID = U_PMF_4NUM_set(OBJ_BRUSH_GROUP1);
       IfNullPtr(poBrushID,__LINE__,"OOPS on U_PMF_4NUM_set\n");
    Tm = (U_PMF_TRANSFORMMATRIX){1 , 0, 0, 1, 0, 0};
    poTm0 = U_PMF_TRANSFORMMATRIX_set(&Tm);
       IfNullPtr(poTm0,__LINE__,"OOPS on U_PMF_TRANSFORMMATRIX_set\n");
    poBf = U_PMF_BLENDFACTORS_linear_set(elements,  0.7, 0.3); //works
          IfNullPtr(poBf,__LINE__,"OOPS U_PMF_BLENDFACTORS_linear_set\n");
    /* blend from purple to green */
    poBc = U_PMF_BLENDCOLORS_linear_set(elements, (U_PMF_ARGB){255,0,255,255}, (U_PMF_ARGB){0,255,0,255});
          IfNullPtr(poBc,__LINE__,"OOPS U_PMF_BLENDCOLORS_linear_set\n");

    for(j=0;j<3;j++){  /* Tm: none, norot, 45 degrees */
       for(i=0;i<5;i++){    /* none, BlendColors, BlendFactors:H, V, H+V */
          switch(j){
             case 0:  tpoTm = NULL;   break;
             case 1:  tpoTm = poTm0;  break;
             case 2:
             default: 
                Tm = tm_for_gradrect(45.0, w, h, Rectfs[0].X, Rectfs[0].Y,1.0);
//                Tm = (U_PMF_TRANSFORMMATRIX){1.0, -1.0, 1.0, 1.0, Rectfs[0].X, Rectfs[0].Y + h}; // scale it so that gradient will reach from corner to corner of square
                poTm45 = U_PMF_TRANSFORMMATRIX_set(&Tm);
                   IfNullPtr(poTm0,__LINE__,"OOPS on U_PMF_TRANSFORMMATRIX_set\n");
                tpoTm = poTm45; 
                break;
          }
          switch(i){
             case 0:   tpoBc  = NULL;   tpoBfH = NULL;   tpoBfV = NULL;   break;
             case 1:   tpoBc  = poBc;   tpoBfH = NULL;   tpoBfV = NULL;   break;
             case 2:   tpoBc  = NULL;   tpoBfH = poBf;   tpoBfV = NULL;   break;
             case 3:   tpoBc  = NULL;   tpoBfH = NULL;   tpoBfV = poBf;   break;
             case 4:   
             default:  tpoBc  = NULL;   tpoBfH = poBf;   tpoBfV = poBf;   break;
          }
//         Lgbd.Flags = U_BD_IsGammaCorrected; /* other bits are set in next call */
         Lgbd.Flags = U_BD_None; /* other bits are set in next call */
          poLgbod = U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_set(&Lgbd.Flags, tpoTm, tpoBc, tpoBfH, tpoBfV);
                IfNullPtr(poLgbod,__LINE__,"OOPS U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_set\n");
          poBd  = U_PMF_LINEARGRADIENTBRUSHDATA_set(&Lgbd, poLgbod);
                IfNullPtr(poBd,__LINE__,"OOPS U_PMF_LINEARGRADIENTBRUSHDATA_set\n");
          poBrush   = U_PMF_BRUSH_set(Version, poBd);
             IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");
          po        = U_PMR_OBJECT_PO_set(OBJ_BRUSH_GROUP1, poBrush);
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
          paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_BRUSH_GROUP1");
             U_PO_free(&poLgbod);
             U_PO_free(&poBrush);
             U_PO_free(&poBd);

          poRectfs  = U_PMF_RECTFN_set(1, Rectfs);
             IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
          po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
          paf(et, poac, po, "U_PMR_FILLRECTS_set");
             U_PO_free(&poRectfs);

          Rectfs[0].X += w;
          if(poTm45)U_PO_free(&poTm45);
       }
       Rectfs[0].X -= 5*w;
       Rectfs[0].Y += h;
    }
    //stack of rectangles with rotating gradient
    double rat=3.0;
    Rectfs[0] = (U_PMF_RECTF){x1 + 5.5*w, y1, w, h/rat}; // notice, here the same as rect in the gradient
    for(i=0;i<360;i+=30){
       Tm = tm_for_gradrect(i, w, h/rat, Rectfs[0].X, Rectfs[0].Y,2.0);
       tpoTm = U_PMF_TRANSFORMMATRIX_set(&Tm);
         IfNullPtr(tpoTm,__LINE__,"OOPS on U_PMF_TRANSFORMMATRIX_set\n");

       Lgbd.Flags = U_BD_None; /* other bits are set in next call */
       poLgbod = U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_set(&Lgbd.Flags, tpoTm, NULL, NULL, NULL);
             IfNullPtr(poLgbod,__LINE__,"OOPS U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_set\n");
       poBd  = U_PMF_LINEARGRADIENTBRUSHDATA_set(&Lgbd, poLgbod);
             IfNullPtr(poBd,__LINE__,"OOPS U_PMF_LINEARGRADIENTBRUSHDATA_set\n");
       poBrush   = U_PMF_BRUSH_set(Version, poBd);
          IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");
       po        = U_PMR_OBJECT_PO_set(OBJ_BRUSH_GROUP1, poBrush);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
       paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_BRUSH_GROUP1");
          U_PO_free(&poLgbod);
          U_PO_free(&poBrush);
          U_PO_free(&poBd);
          U_PO_free(&tpoTm);

       poRectfs  = U_PMF_RECTFN_set(1, Rectfs);
          IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
       po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
       paf(et, poac, po, "U_PMR_FILLRECTS_set");
          U_PO_free(&poRectfs);

       Rectfs[0].Y += h/rat;

    }
    /* clean out common POs from above  */
    U_PO_free(&poBrushID);
    U_PO_free(&poBf);
    U_PO_free(&poBc);
    U_PO_free(&poTm0);
}

void not_in_pmf(double scale, int x, int y, uint32_t PenID, U_PSEUDO_OBJ *BrushID, 
      EMFTRACK *et, U_DPSEUDO_OBJ *dpath, U_PSEUDO_OBJ *poac){
   U_PSEUDO_OBJ *po;
   U_PSEUDO_OBJ *poPath;
   U_PMF_POINTF *points;
   U_PMF_POINTF missing[4] = { { 0, 0 }, {   0, 200 }, { 200, 200 }, { 200,   0 } };
   uint32_t Version = U_PMF_GRAPHICSVERSIONOBJ_set(2);
   points = pointfs_transform(missing, 4, xform_alt_set(scale, 1.0, 0.0, 0.0, x, y));
   U_PATH_moveto(dpath, points[0], U_PTP_None);
   U_PATH_lineto(dpath, points[1], U_PTP_None);
   U_PATH_lineto(dpath, points[2], U_PTP_None);
   U_PATH_lineto(dpath, points[3], U_PTP_None);
   U_PATH_closepath(dpath);
   poPath = U_PMF_PATH_set2(Version, dpath);
   po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
   paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
   po = U_PMR_drawfill(OBJ_PATH_1, PenID, BrushID);
   paf(et, poac, po, "U_PMR_drawfill");
      U_PO_free(&poPath);
      free(points);
}


int main(int argc, char *argv[]){
    EMFTRACK              *et;
    EMFHANDLES            *eht;
    U_SIZEL                szlDev,szlMm;
    U_RECTL                rclBounds,rclFrame;
    int                    status;
    int                    mode = 0;   // conditional for some tests
    unsigned int           umode;
    char                  *rec;
    char                  *string;
    int                    slen;
    uint16_t              *Description;
    uint16_t              *FontName;
    uint32_t               cbDesc;
    U_PSEUDO_OBJ          *po;
    U_PSEUDO_OBJ          *po1;
    U_PSEUDO_OBJ          *poGv;
    U_PSEUDO_OBJ          *poBm;
    U_PSEUDO_OBJ          *poBmd;
    U_PSEUDO_OBJ          *poImg;
    U_PSEUDO_OBJ          *poTbod;
    U_PSEUDO_OBJ          *poBd;
    U_PSEUDO_OBJ          *poBrush;
    U_PSEUDO_OBJ          *poBrushID;
    U_PSEUDO_OBJ          *poColor;
    U_PSEUDO_OBJ          *poFg;
    U_PSEUDO_OBJ          *poBg;
    U_PSEUDO_OBJ          *poPenOD;
    U_PSEUDO_OBJ          *poPenD;
    U_PSEUDO_OBJ          *poPen;
    U_PSEUDO_OBJ          *poRectfs;
    U_PSEUDO_OBJ          *poRectf;
    U_PSEUDO_OBJ          *poRects;
    U_PSEUDO_OBJ          *poFont;
    U_PSEUDO_OBJ          *poPath;
    U_PSEUDO_OBJ          *poPoints;
    U_PSEUDO_OBJ          *poPTypes;
    U_PSEUDO_OBJ          *poDashes;
 
    U_PSEUDO_OBJ          *poSF;
    U_PSEUDO_OBJ          *poTm;
    U_PSEUDO_OBJ          *poac;  /* accumulator buffer */
//    U_PMF_ARGB             ColorArgb;
//    U_PMF_ARGB            *ColorNArgb;
    uint32_t               Version = U_PMF_GRAPHICSVERSIONOBJ_set(2); /* this is needed over, and over...*/
    U_PMF_RECTF            Rectfs[20];
    U_PMF_RECTF           *pRectfs;
    U_PMF_RECT             Rects[20];
    U_PMF_POINTF           Pointfs[20];
    uint8_t                Ppts[20];
    U_PMF_TRANSFORMMATRIX  Tm;
    U_PMF_STRINGFORMAT     Sfs;
    int                    elements;
    U_DPSEUDO_OBJ         *dpath;
    U_PMF_POINTF           star5[5]     = { { 100, 0 }, { 200, 300 }, { 0, 100 }, { 200, 100 } , { 0, 300 }};
    U_PMF_POINTF          *points;
    U_PMF_POINTF           plarray[7]   = { { 200, 0 }, { 400, 200 }, { 400, 400 }, { 200, 600 } , { 0, 400 }, { 0, 200 }, { 200, 0 }};
    U_PMF_POINTF           pl1;
    U_PMF_POINTF           pl12[12];
//    uint32_t               plc[12];
//    U_PMF_POINT            point16;
    int                    i,j,k,m;
    int                    cap, join, miter;
    char                  *px;
    char                  *rgba_px;
    int                    offset;
    int                    iax,iay;    //alignment values, like U_SA_Near
    uint32_t               cbPx;
    uint32_t               colortype;
    PU_RGBQUAD             ct;         //color table
    int                    numCt;      //number of entries in the color table
    U_PMF_BITMAP           Bs;
    char                  *rgba_px2;
    
    char                   fitest[128];
    char                   mys[4];
    U_FontInfoParams         *fip;
    U_FontInfoParams          fi[] = { /* removed bold and italic because they were not rendering that way in the test */
{"Arial",1854,-434,67,2048,2060,-665},
{"Arial Black",2254,-634,0,2048,2219,-628},
{"Comic Sans MS",2257,-597,0,2048,2257,-639},
{"Courier New",1705,-615,0,2048,2091,-1392},
{"DejaVu Sans",1901,-483,0,2048,2389,-850},
{"DejaVu Sans Condensed",1901,-483,0,2048,2389,-850},
{"DejaVu Sans ExtraLight",1901,-483,0,2048,2262,-550},
{"DejaVu Sans Mono",1901,-483,0,2048,2133,-767},
{"DejaVu Serif",1901,-483,0,2048,2272,-710},
{"DejaVu Serif Condensed",1901,-483,0,2048,2272,-710},
{"Estrangelo Edessa",1434,-614,0,2048,1440,-643},
{"Franklin Gothic Medium",1877,-445,0,2048,1949,-628},
{"Gautami",1892,-1664,348,2048,1892,-1664},
{"Gentium Basic",1790,-580,0,2048,2169,-725},
{"Gentium Book Basic",1790,-580,0,2048,2182,-728},
{"Georgia",1878,-449,0,2048,1868,-444},
{"Impact",2066,-432,0,2048,2392,-677},
{"Kartika",1408,-640,0,2048,1434,-645},
{"Lucida Sans Unicode",2246,-901,0,2048,2344,-901},
{"Latha",2048,-1352,0,2048,2220,-876},
{"Liberation Mono",1705,-615,0,2048,2009,-615},
{"Liberation Sans Narrow",1916,-434,0,2048,1864,-621},
{"Liberation Sans",1854,-434,67,2048,2007,-621},
{"Liberation Serif",1825,-443,87,2048,2010,-621},
{"Lucida Console",1616,-432,0,2048,1616,-432},
{"Mangal",2542,-898,0,2048,2743,-1016},
{"Microsoft Sans Serif",1888,-430,0,2048,1888,-430},
{"Mv Boli",2333,-967,0,2048,1792,-610},
// Do NOT use OpenSymbol - it has no glyphs for a-z, A-Z, resulting in font substitution in the viewer
// EMF+ is not stable to font substituion, in general, because ascender, descender, etc. may differ from the
// original font.
//{"OpenSymbol",1878,-641,0,2048,1878,-641},
{"Open Sans",2189,-600,0,2048,2146,-555},
{"Palatino Linotype",1499,-582,682,2048,2150,-597},
{"PT Serif",1039,-286,0,1000,1003,-272},
{"Raavi",2048,-1352,256,2048,2048,-1352},
{"Shruti",2084,-1352,320,2048,2091,-1123},
{"Sylfaen",1510,-576,611,2048,2062,-604},
{"Symbol",2059,-450,0,2048,2059,-450},
{"Tahoma",2049,-423,0,2048,2118,-424},
{"Times New Roman",1825,-443,87,2048,2062,-628},
{"Trebuchet MS",1923,-455,0,2048,1931,-537},
{"Tunga",2048,-1352,224,2048,2048,-1356},
{"Bitstream Vera Sans",1901,-483,0,2048,1901,-483},
{"Bitstream Vera Sans Mono",1901,-483,0,2048,1901,-483},
{"Bitstream Vera Serif",1901,-483,0,2048,1901,-483},
{"Verdana",2059,-430,0,2048,2049,-423},
{"Vrinda",1464,-584,43,2048,1483,-574},
{"Webdings",1638,-410,0,2048,1638,-410},
{"WingDings",1841,-432,0,2048,1841,-432},
         {"",0,0,0,0,0,0}
    };

#define PNG_ARRAY_SIZE 558
    uint8_t pngarray[PNG_ARRAY_SIZE]= { /* 10 x 10 array */
       0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,
       0x00,0x0d,0x49,0x48,0x44,0x52,0x00,0x00,0x00,0x0a,
       0x00,0x00,0x00,0x0a,0x08,0x03,0x00,0x00,0x00,0xba,
       0xec,0x3f,0x8f,0x00,0x00,0x00,0x01,0x73,0x52,0x47,
       0x42,0x00,0xae,0xce,0x1c,0xe9,0x00,0x00,0x01,0x2c,
       0x50,0x4c,0x54,0x45,0x00,0x00,0x00,0x1d,0x1c,0x1c,
       0x1d,0x00,0x00,0x00,0x00,0x1c,0x1d,0x1d,0x38,0x00,
       0x00,0x38,0x1d,0x1d,0x55,0x00,0x00,0x55,0x1d,0x1d,
       0x71,0x00,0x00,0x71,0x1d,0x1d,0x8d,0x00,0x00,0x8d,
       0x1d,0x1d,0xaa,0x00,0x00,0xaa,0x1d,0x1d,0xc6,0x00,
       0x00,0xc6,0x1d,0x1d,0xe2,0x00,0x1d,0xe2,0x00,0x00,
       0xe2,0x00,0x00,0xff,0x1d,0x39,0xc6,0x00,0x39,0xc6,
       0x1d,0x55,0xaa,0x00,0x55,0xaa,0x1d,0x72,0x8d,0x00,
       0x72,0x8d,0x1d,0x8e,0x71,0x00,0x8e,0x71,0x1d,0xaa,
       0x55,0x00,0xaa,0x55,0x1d,0xc7,0x38,0x00,0xc7,0x38,
       0x1d,0xe2,0x00,0x00,0xff,0x00,0x1d,0xe2,0x1c,0x00,
       0xe3,0x1c,0x39,0x00,0x00,0x39,0x1c,0x1c,0x39,0x38,
       0x38,0x39,0x39,0x55,0x39,0x39,0x71,0x39,0x39,0x8d,
       0x39,0x39,0xaa,0x39,0x39,0xc6,0x39,0x55,0xaa,0x39,
       0x72,0x8d,0x39,0x8e,0x71,0x39,0xaa,0x55,0x39,0xc6,
       0x00,0x39,0xc6,0x1c,0x39,0xc6,0x38,0x55,0x00,0x00,
       0x55,0x1c,0x1c,0x55,0x38,0x38,0x55,0x55,0x55,0x55,
       0x55,0x71,0x55,0x55,0x8d,0x55,0x55,0xaa,0x55,0x72,
       0x8d,0x55,0x8e,0x71,0x55,0xaa,0x00,0x55,0xaa,0x1c,
       0x55,0xaa,0x38,0x55,0xaa,0x55,0x72,0x00,0x00,0x72,
       0x1c,0x1c,0x72,0x38,0x38,0x72,0x55,0x55,0x72,0x71,
       0x71,0x72,0x72,0x8d,0x72,0x8d,0x00,0x72,0x8d,0x1c,
       0x72,0x8d,0x38,0x72,0x8d,0x55,0x72,0x8d,0x71,0x8e,
       0x00,0x00,0x8e,0x1c,0x1c,0x8e,0x38,0x38,0x8e,0x55,
       0x55,0x8e,0x71,0x00,0x8e,0x71,0x1c,0x8e,0x71,0x38,
       0x8e,0x71,0x55,0x8e,0x71,0x71,0xaa,0x00,0x00,0xaa,
       0x1c,0x1c,0xaa,0x38,0x38,0xaa,0x55,0x00,0xaa,0x55,
       0x1c,0xaa,0x55,0x38,0xaa,0x55,0x55,0xc7,0x00,0x00,
       0xc7,0x1c,0x1c,0xc7,0x38,0x00,0xc7,0x38,0x1c,0xc7,
       0x38,0x38,0xff,0x00,0x00,0xe3,0x1c,0x00,0xe3,0x00,
       0x00,0xe3,0x1c,0x1c,0xdd,0x04,0xe1,0xb3,0x00,0x00,
       0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x00,0xec,
       0x00,0x00,0x00,0xec,0x01,0x79,0x28,0x71,0xbd,0x00,
       0x00,0x00,0x19,0x74,0x45,0x58,0x74,0x53,0x6f,0x66,
       0x74,0x77,0x61,0x72,0x65,0x00,0x4d,0x69,0x63,0x72,
       0x6f,0x73,0x6f,0x66,0x74,0x20,0x4f,0x66,0x66,0x69,
       0x63,0x65,0x7f,0xed,0x35,0x71,0x00,0x00,0x00,0x76,
       0x49,0x44,0x41,0x54,0x18,0x57,0x63,0x48,0x48,0x8c,
       0x0d,0xf7,0x77,0xb3,0x31,0x50,0x50,0x64,0x48,0x4a,
       0x8e,0x8b,0x08,0x70,0xb7,0x35,0x54,0x52,0x66,0x88,
       0x8e,0x89,0x8f,0x0c,0xf4,0xb0,0x33,0x92,0x93,0x67,
       0x08,0x09,0x0d,0x8b,0x0a,0xf2,0xb4,0xd7,0x97,0x91,
       0x65,0xf0,0xf6,0xf1,0xf5,0x0b,0xf6,0xb2,0xd6,0x93,
       0x92,0x66,0x70,0x70,0x74,0x72,0x76,0x71,0xb5,0xd2,
       0x95,0x90,0x64,0x30,0x36,0x31,0x35,0x33,0xb7,0xb0,
       0xd4,0x11,0x13,0x67,0x50,0x51,0x55,0x53,0xd7,0xd0,
       0xd4,0xd2,0x16,0x11,0x65,0x60,0x62,0x64,0x61,0xe3,
       0xe0,0xe2,0xe1,0x13,0x10,0x64,0x60,0x60,0x66,0x65,
       0xe7,0xe4,0xe6,0xe5,0x17,0x12,0x06,0x00,0x38,0x73,
       0x13,0x57,0x30,0x29,0x6b,0x1f,0x00,0x00,0x00,0x00,
       0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
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
      printf("Syntax: testbed_pmf flags\n");
      printf("   Flags is a hexadecimal number composed of the following optional bits (use 0 for a standard run):\n");
      printf("   1  Disable tests that block EMF import into PowerPoint (dotted lines)\n");
      printf("   2  Enable tests that block EMF being displayed in Windows Preview (currently, GradientFill)\n");
      printf("   4  Rotate and scale the test image within the page.\n");
      printf("   8  Disable clipping tests.\n");
      exit(EXIT_FAILURE);
    }
 
    /* initialize an accumulator, big enough for anything except possibly images.
       Do not use this explicitly, just pass it to paf()! */
    poac = U_PO_create(NULL, 65536, 65536, 0); 

    /* ********************************************************************** */
    // set up and begin the EMF+ file, core is EMF
 
    status=emf_start("test_libuemf_p.emf",1000000, 250000, &et);  // space allocation initial and increment 
    if(status)printf_and_flush("error in emf_start\n");
    status=emf_htable_create(128, 128, &eht);
    if(status)printf_and_flush("error in emf_htable\n");
    (void) device_size(216, 279, 47.244094, &szlDev, &szlMm); // Example: device is Letter vertical, 1200 dpi = 47.244 DPmm
    (void) drawing_size(297, 210, 47.244094, &rclBounds, &rclFrame);  // Example: drawing is A4 horizontal,  1200 dpi = 47.244 DPmm
    Description = U_Utf8ToUtf16le("Test EMF+\1produced by libUEMF testbed_pmf program\1",0, NULL); 
    cbDesc = 2 + wchar16len(Description);  // also count the final terminator
    (void) U_Utf16leEdit(Description, U_Utf16le(1), 0);
    rec = U_EMRHEADER_set( rclBounds,  rclFrame,  NULL, cbDesc, Description, szlDev, szlMm, 0);
    taf(rec,et,"U_EMRHEADER_set");
    free(Description);

    
    poGv = U_PMF_GRAPHICSVERSION_set(2);
    po = U_PMR_HEADER_set(1, 0, poGv, 1200, 1200);
       U_PO_free(&poGv);
    paf(et, poac, po, "U_PMR_HEADER_set");

    /* Header in place, add an wide assortment of PMR records */

    string = malloc(128);
    memset(string,0,128);
    sprintf(string,"Everything after this is EMF+, except for the very last record.");
    po = U_PMR_COMMENT_set(128, string);
    paf(et, poac, po, "U_PMR_COMMENT_set");
    free(string);

    /* Put a bunch of EMF+ records in one EMF comment */
    po = U_PMR_SETPIXELOFFSETMODE_set(U_POM_Half);


//    po1 = U_PMR_SETANTIALIASMODE_set(U_SM_HighQuality, 1);
    po1 = U_PMR_SETANTIALIASMODE_set(U_SM_Default, 1);
    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
       U_PO_free(&po1);

    po1 = U_PMR_SETTEXTRENDERINGHINT_set(U_TRH_ClearTypeGridFit);
    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
       U_PO_free(&po1);

    po1 = U_PMR_SETCOMPOSITINGQUALITY_set(U_CQ_Default);
    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
    U_PO_free(&po1);

    po1 = U_PMR_SETPAGETRANSFORM_set(U_UT_Pixel, 1.0); /* only value known tested so far */
    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
       U_PO_free(&po1);

    po1 = U_PMR_SETINTERPOLATIONMODE_set(U_IM_HighQualityBicubic);
    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
       U_PO_free(&po1);

    po1 = U_PMR_SETTEXTCONTRAST_set(1000);
    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
       U_PO_free(&po1);

    po1 = U_PMR_GETDC_set();
       IfNullPtr(po1,__LINE__,"OOPS on U_PMR_GETDC_set\n");
    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
       U_PO_free(&po1);

    paf(et, poac, po, "multiple PMR, starting with U_PMR_SETPIXELOFFSETMODE_set");


    U_PSEUDO_OBJ *poBrushDarkRedID = U_PMF_ARGB_set(0xFF, 0x8B, 0x00, 0x00);

    /* ******** In this section the various objects like brushes, pens, and so forth, are set *********   */
    /* OBJ_* defines are at the top of this file */

    /* Object OBJ_PEN_BLACK_1, solid black pen 1 pixel wide */
    poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                   NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Miter,
                   10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                   NULL, U_PA_Center, NULL, NULL, 
                   NULL);
       IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");

    poPenD    = U_PMF_PENDATA_set(U_UT_World, 1.0, poPenOD);
       IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");

    poColor   = U_PMF_ARGB_set(255,0,0,0);
       IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
    poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
    IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
    poBrush   = U_PMF_BRUSH_set(Version, poBd);
       IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");

    poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
       IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
    po        = U_PMR_OBJECT_PO_set(OBJ_PEN_BLACK_1, poPen);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_BLACK_1");
       U_PO_free(&poPen);
       U_PO_free(&poPenD);
       U_PO_free(&poPenOD);
       U_PO_free(&poColor);
       U_PO_free(&poBd);
       U_PO_free(&poBrush);
 
    /* Object OBJ_PEN_RED_10, solid red pen 10 World pixels wide */
    poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                   NULL, U_LCT_Square, U_LCT_Square, U_LJT_Miter,
                   10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                   NULL, U_PA_Center, NULL, NULL, 
                   NULL);
       IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");

    poPenD    = U_PMF_PENDATA_set(U_UT_World, 10.0, poPenOD);
       IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");

    poColor   = U_PMF_ARGB_set(255,255,0,0);
       IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
    poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
    IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
    poBrush   = U_PMF_BRUSH_set(Version, poBd);
       IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");

    poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
       IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
    po        = U_PMR_OBJECT_PO_set(OBJ_PEN_RED_10, poPen);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_RED_10");
       U_PO_free(&poPen);
       U_PO_free(&poPenD);
       U_PO_free(&poPenOD);
       U_PO_free(&poColor);
       U_PO_free(&poBd);
       U_PO_free(&poBrush);

    /* Object OBJ_PEN_RED_1, solid red pen 1 World pixels wide */
    poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                   NULL, U_LCT_Square, U_LCT_Square, U_LJT_Miter,
                   1.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                   NULL, U_PA_Center, NULL, NULL, 
                   NULL);
       IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");

    poPenD    = U_PMF_PENDATA_set(U_UT_World, 1.0, poPenOD);
       IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");

    poColor   = U_PMF_ARGB_set(255,255,0,0);
       IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
    poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
    IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
    poBrush   = U_PMF_BRUSH_set(Version, poBd);
       IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");

    poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
       IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
    po        = U_PMR_OBJECT_PO_set(OBJ_PEN_RED_1, poPen);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_RED_1");
       U_PO_free(&poPen);
       U_PO_free(&poPenD);
       U_PO_free(&poPenOD);
       U_PO_free(&poColor);
       U_PO_free(&poBd);
       U_PO_free(&poBrush);

    /* Object OBJ_FONT_TCOMMENT is reserved for text comment Font */

    /* Object OBJ_FONT_OTHER is reserved for all other Fonts */

    /* Object OBJ_SF_TCOMMENT is reserved for text comment stringformat */
    Sfs.Version           = Version;
    Sfs.Flags             = U_SF_NoFitBlackBox + U_SF_NoClip; // ignore the bounding box;
    Sfs.Language          = U_LID_en_US;
    Sfs.StringAlignment   = U_SA_Near; //  Horizontal
    Sfs.LineAlign         = U_SA_Near; //  Vertical
    Sfs.DigitSubstitution = U_SDS_None;
    Sfs.DigitLanguage     = U_LID_en_US;
    Sfs.FirstTabOffset    = 0.0;
    Sfs.HotkeyPrefix      = U_HKP_None;
    Sfs.LeadingMargin     = 0.0;
    Sfs.TrailingMargin    = 0.0;
    Sfs.Tracking          = 1.0;
    Sfs.Trimming          = U_ST_None; // probably does nothing because of NoClip and NoWrap
    Sfs.TabStopCount      = 0;
    Sfs.RangeCount        = 0;
    poSF = U_PMF_STRINGFORMAT_set(&Sfs, NULL);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");

    po = U_PMR_OBJECT_PO_set(OBJ_SF_TCOMMENT, poSF);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");

    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_SF_TCOMMENT");
       U_PO_free(&poSF);


    /* Object OBJ_SF_OTHER is reserved for text alignment stringformat, gets rewritten as String/Line Align change */
    poSF = U_PMF_STRINGFORMAT_set(&Sfs, NULL);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");

    po = U_PMR_OBJECT_PO_set(OBJ_SF_OTHER, poSF);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");

    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_SF_OTHER");
       U_PO_free(&poSF);

    /* Object OBJ_SF_OTHER is reserved for all other stringformats */
    
    /* Object OBJ_PEN_GROUP1 */
    poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                   NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Miter,
                   10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                   NULL, U_PA_Center, NULL, NULL, 
                   NULL);
       IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");
    poPenD    = U_PMF_PENDATA_set(U_UT_World, 50, poPenOD);
       IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");

    poColor   = U_PMF_ARGB_set(255, 127, 255, 196);
       IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
    poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
       IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
    poBrush   = U_PMF_BRUSH_set(Version, poBd);
       IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");

    poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
       IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
    po        = U_PMR_OBJECT_PO_set(OBJ_PEN_GROUP1, poPen);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_GROUP1");
       U_PO_free(&poPen);
       U_PO_free(&poPenD);
       U_PO_free(&poPenOD);
       U_PO_free(&poColor);
       U_PO_free(&poBd);
       U_PO_free(&poBrush);

    /* Object OBJ_GROUP_PEN2 (almost the same as above, different join) */
    poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                   NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Round,
                   10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                   NULL, U_PA_Center, NULL, NULL, 
                   NULL);
       IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");
    poPenD    = U_PMF_PENDATA_set(U_UT_World, 50, poPenOD);
       IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");

    poColor   = U_PMF_ARGB_set(255, 127, 255, 196);
       IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
    poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
       IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
    poBrush   = U_PMF_BRUSH_set(Version, poBd);
       IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");

    poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
       IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
    po        = U_PMR_OBJECT_PO_set(OBJ_PEN_GROUP2, poPen);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_GROUP1");
       U_PO_free(&poPen);
       U_PO_free(&poPenD);
       U_PO_free(&poPenOD);
       U_PO_free(&poColor);
       U_PO_free(&poBd);
       U_PO_free(&poBrush);



    /* Object OBJ_BRUSH_GROUP1 */
    poColor   = U_PMF_ARGB_set(255, 196, 127, 255);
       IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
    poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
       IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
    poBrush   = U_PMF_BRUSH_set(Version, poBd);
       IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");
    po        = U_PMR_OBJECT_PO_set(OBJ_BRUSH_GROUP1, poBrush);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_BRUSH_GROUP1");
       U_PO_free(&poColor);
       U_PO_free(&poBd);
       U_PO_free(&poBrush);




    /* **********put a rectangle under everything, so that transparency is obvious ******************* */

    Tm = (U_PMF_TRANSFORMMATRIX){1,-0.0,-0.0,1,-0.0,-0.0};
    poTm = U_PMF_TRANSFORMMATRIX_set(&Tm);
       IfNullPtr(poTm,__LINE__,"OOPS on U_PMF_TRANSFORMMATRIX_set\n");
    po  = U_PMR_SETWORLDTRANSFORM_set(poTm);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_SETWORLDTRANSFORM_set\n");
       U_PO_free(&poTm);


    Rectfs[0] = (U_PMF_RECTF){0, 0, 14031, 9921};
    poRectfs  = U_PMF_RECTFN_set(1, Rectfs);
       IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");

    poColor   = U_PMF_ARGB_set(255,255,255,173);     //yellowish color, only use here
       IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");

    po1 = U_PMR_FILLRECTS_set(poColor, poRectfs);
       IfNullPtr(po1,__LINE__,"OOPS on U_PMR_FILLRECTS_set\n");
       U_PO_free(&poRectfs);
       U_PO_free(&poColor);

    (void) U_PO_po_append(po, po1, U_PMF_KEEP_ELEMENTS);
    U_PO_free(&po1);

    paf(et, poac, po, "multiple, starting with U_PMR_SETWORLDTRANSFORM_set");

    /* label the drawing */
    
    poColor   = U_PMF_ARGB_set(255,0,0,0);

    textlabel(poac, 400, "libUEMF v0.2.1",     9700, 200, U_SA_Near, U_SA_Near, poColor, et);
    textlabel(poac, 400, "April 23, 2015",     9700, 500, U_SA_Near, U_SA_Near, poColor, et);
    rec = malloc(128);
    (void)sprintf(rec,"EMF+ test: %2.2X",mode);
    textlabel(poac, 400, rec,                   9700, 800, U_SA_Near, U_SA_Near, poColor, et);
    free(rec);

    /* Test a very large collection of fonts, see if the alignment methods work.  Some of these may not be present on some
       target systems, in which case, they will not align */
    j = 6500-325;
    for(m=0;m<3;m++){ // U_SA_Near|Center|Far
       fip = &fi[0];
       i = 150000-1000,
       j+=125;
       sprintf(mys,"My%c",(m==U_SA_Near? 'N' :(m==U_SA_Center ? 'C' : 'F' ))); /* "My" provides a good baseline and a descender,*/
       while(*(fip->name)){
          if(i>=12000){
             i =120;
             j+=125;
             (void) U_PMR_drawline(OBJ_PEN_RED_1,OBJ_PATH_1, (U_PMF_POINTF){100, j}, (U_PMF_POINTF){13500, j},  0, poac, et);
          }
          sprintf(fitest,"%s %-s                                      ",mys, fip->name);
          (void) U_PMR_drawstring(fitest, m, OBJ_FONT_TCOMMENT, poColor, OBJ_SF_TCOMMENT, Sfs, 
             fip->name, 100.0, fip, U_FS_None,  i, j, poac, et); /* no more than 40 characters of name */
          i+=1500;
          fip++;
       }
       fip = &fi[0];
       i = 120;
       j+= 125;
       (void) U_PMR_drawline(OBJ_PEN_RED_1,OBJ_PATH_1, (U_PMF_POINTF){100, j}, (U_PMF_POINTF){13500, j},  0, poac, et);
       for(k=1;k<10;k++){
           sprintf(fitest,"%s %i %-s                                      ",mys,100/k,fip->name); /* my guarantees a good baseline and a descender,*/
           (void) U_PMR_drawstring(fitest, m, OBJ_FONT_TCOMMENT, poColor, OBJ_SF_TCOMMENT, Sfs, 
             fip->name, 100/k, fip, U_FS_None,  i, j, poac, et); /* no more than 40 characters of name */
           i+=1500;
       }
    }
    U_PO_free(&poColor);

    /* ********************************************************************** */
    // basic drawing operations
    /* Hang onto this until the very end, all Fills use it */
    poBrushID = U_PMF_4NUM_set(OBJ_BRUSH_GROUP1);
       IfNullPtr(poBrushID,__LINE__,"OOPS on U_PMF_4NUM_set\n");

    /* Fill + Draw circle */
    poRectf = U_PMF_RECTF4_set(100,1300, 300, 300);
       IfNullPtr(poRectf,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
    po = U_PMR_FILLELLIPSE_set(poBrushID, poRectf);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWELLIPSE_set\n");
    paf(et, poac, po, "U_PMR_FILLELLIPSE_set");
    po = U_PMR_DRAWELLIPSE_set(OBJ_PEN_GROUP1, poRectf);
       IfNullPtr(po,__LINE__,"OOPS on OBJ_PEN_GROUP1\n");
    paf(et, poac, po, "U_PMR_DRAWELLIPSE_set");
       U_PO_free(&poRectf);

    /* Fill + Draw Rect */
    Rectfs[0] = (U_PMF_RECTF){500,1300, 300, 300};
    poRectfs  = U_PMF_RECTFN_set(1, Rectfs);
       IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
    po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLRECTS_set\n");
    paf(et, poac, po, "U_PMR_FILLELLIPSE_set");
    po = U_PMR_DRAWRECTS_set(OBJ_PEN_GROUP1, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on OBJ_PEN_GROUP1\n");
    paf(et, poac, po, "U_PMR_DRAWRECTS_set");
       U_PO_free(&poRectfs);

    /* 32 bit float points, absolute */
    /* Fill + Draw Polygon */
    elements = 4; /* MUST agree with thenumber set below */
    Pointfs[0] = (U_PMF_POINTF) { 900,1300};
    Pointfs[1] = (U_PMF_POINTF) {1200,1500};
    Pointfs[2] = (U_PMF_POINTF) {1200,1700};
    Pointfs[3] = (U_PMF_POINTF) { 900,1500};
    Ppts[0] = U_PPT_Start;
    Ppts[1] = U_PPT_Line;
    Ppts[2] = U_PPT_Line;
    Ppts[3] = U_PPT_Line | U_PTP_CloseSubpath;
    poPTypes = U_PMF_PATHPOINTTYPE_set(elements, Ppts);
       IfNullPtr(poPTypes,__LINE__,"OOPS on U_PMF_PATHPOINTTYPE_set\n");
    poPoints = U_PMF_POINTF_set(elements, Pointfs);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    poPath = U_PMF_PATH_set(Version, poPoints, poPTypes);
       IfNullPtr(poPath,__LINE__,"OOPS on U_PMF_PATH_set\n");
//    DumpPo(poPath, "check for U_PMF_PATH");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
       U_PO_free(&poPoints);
       U_PO_free(&poPTypes);


    po = U_PMR_FILLPATH_set(OBJ_PATH_1, poBrushID);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLPATH_set\n");
    paf(et, poac, po, "U_PMR_FILLPATH_set");
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");

#if 0
   /* 16 bit int points.  This variant also works. */
    U_PMF_POINT            Point16s[20];
    elements = 4; /* MUST agree with thenumber set below */
    Point16s[0] = (U_PMF_POINT) { 900,1300};
    Point16s[1] = (U_PMF_POINT) {1200,1500};
    Point16s[2] = (U_PMF_POINT) {1200,1700};
    Point16s[3] = (U_PMF_POINT) { 900,1500};
    Ppts[0] = U_PPT_Start;
    Ppts[1] = U_PPT_Line;
    Ppts[2] = U_PPT_Line;
    Ppts[3] = U_PPT_Line | U_PTP_CloseSubpath;
    poPTypes = U_PMF_PATHPOINTTYPE_set(elements, Ppts);
       IfNullPtr(poPTypes,__LINE__,"OOPS on U_PMF_PATHPOINTTYPE_set\n");
    poPoints = U_PMF_POINT_set(elements, Point16s);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    poPath = U_PMF_PATH_set(Version, poPoints, poPTypes);
       IfNullPtr(poPath,__LINE__,"OOPS on U_PMF_PATH_set\n");
//    DumpPo(poPath, "check for U_PMF_PATH");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
       U_PO_free(&poPoints);
       U_PO_free(&poPTypes);
#endif

#if 0
    /* 32 bit float points, relative, draw a line.  No success so far in numerous attempts
       to get Relative points to work in paths.  This is the simplest one I could think of, 
       it just makes 4 single bytes, and the boundaries all line up on multiples of 4, yet
       it does not draw */
    elements = 2; /* MUST agree with thenumber set below */
    Pointfs[0] = (U_PMF_POINTF) {  50.,  50.};
    poPoints = U_PMF_POINTR_set(elements, Pointfs);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTR_set\n");
    po = U_PMR_DRAWLINES_set(OBJ_PEN_GROUP1, 0, poPoints);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMR_DRAWLINES_set\n");
    paf(et, poac, po, "U_PMR_DRAWLINES_set");
       U_PO_free(&poPoints);
#endif

    po = U_PMR_FILLPATH_set(OBJ_PATH_1, poBrushID);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLPATH_set\n");
    paf(et, poac, po, "U_PMR_FILLPATH_set");
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");


    /* Draw Arc */
    poRectf = U_PMF_RECTF4_set(1300, 1300, 300, 300);
       IfNullPtr(poRectf,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
    po = U_PMR_DRAWARC_set(OBJ_PEN_GROUP1, 90.0, 135.0, poRectf);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWARC_set\n");
    paf(et, poac, po, "U_PMR_DRAWARC_set");
       U_PO_free(&poRectf);

    /* Draw line, arc, line */
    Rectfs[0] = (U_PMF_RECTF){1600, 1300, 300, 300};
    dpath =  U_PATH_create(0, NULL, 0, 0); /* create an empty path*/
       IfNullPtr(dpath,__LINE__,"OOPS on U_PATH_create\n");
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){1800,1300},      U_PTP_None           ),__LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_arcto( dpath, 225.0, -135.0, 0.0, &Rectfs[0], U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_arcto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){1750,1450},      U_PTP_None           ),__LINE__, "OOPS on U_PATH_moveto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
       U_DPO_free(&dpath);
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");

    /* dpath created in the next line will be used over and over again below, being cleared between draws,
       and only being free'd at the very end. */
    dpath =  U_PATH_create(0, NULL, 0, 0); /* create an empty path*/
       IfNullPtr(dpath,__LINE__,"OOPS on U_PATH_create\n");

    /* Fill + Draw chord */
    U_DPO_clear(dpath);
    Rectfs[0] = (U_PMF_RECTF){1900, 1300, 300, 300};
    IfNotTrue(U_PATH_arcto( dpath, 225.0, -135.0, 0.0, &Rectfs[0], U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_arcto\n");
    IfNotTrue(U_PATH_closepath((dpath)),                                                  __LINE__, "OOPS on U_PATH_closepath\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
    po = U_PMR_FILLPATH_set(OBJ_PATH_1, poBrushID);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLPATH_set\n");
    paf(et, poac, po, "U_PMR_FILLPATH_set");
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");


    /* Fill + Draw Pie */
    poRectf = U_PMF_RECTF4_set(2200,1300, 300, 300);
       IfNullPtr(poRectf,__LINE__,"OOPS on U_PMF_RECTF4_set\n");
    po = U_PMR_FILLPIE_set(90.0, -225.0, poBrushID, poRectf);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLPIE_set\n");
    paf(et, poac, po, "U_PMR_FILLELLIPSE_set");
    po = U_PMR_DRAWPIE_set(OBJ_PEN_GROUP1, 90.0, -225.0, poRectf);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPIE_set\n");
    paf(et, poac, po, "U_PMR_DRAWPIE_set");
       U_PO_free(&poRectf);
       
     /* Emulate roundrect by changing the join on the pen */
     
    Rectfs[0] = (U_PMF_RECTF){2600,1300, 300, 300};
    poRectfs  = U_PMF_RECTFN_set(1, Rectfs);
       IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
    po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWELLIPSE_set\n");
    paf(et, poac, po, "U_PMR_FILLELLIPSE_set");
    po = U_PMR_DRAWRECTS_set(OBJ_PEN_GROUP2, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWRECTS_set\n");
    paf(et, poac, po, "U_PMR_DRAWELLIPSE_set");
       U_PO_free(&poRectfs);



    /* finally release this, it was used by all of the above */
    U_PO_free(&poBrushID);

    /* this is used for the next couple */
    poBrushID = U_PMF_ARGB_set(255, 128, 128, 128);
       IfNullPtr(poBrushID,__LINE__,"OOPS on U_PMF_ARGB_set\n");

    /* Draw a star with WINDING */
    points = pointfs_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3000, 1300));
    poPoints = U_PMF_POINTF_set(5, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_FILLCLOSEDCURVE_set(U_WINDING, 0.0, poBrushID, poPoints);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLCLOSEDCURVE_set\n");
    paf(et, poac, po, "U_PMR_FILLCLOSEDCURVE_set");
    po = U_PMR_DRAWCLOSEDCURVE_set(OBJ_PEN_BLACK_1, 0.0, poPoints);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWCLOSEDCURVE_set\n");
    paf(et, poac, po, "U_PMR_DRAWCLOSEDCURVE_set");
       U_PO_free(&poPoints);
       free(points);

    /* Draw a star with Alternate */
    points = pointfs_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3300, 1300));
    poPoints = U_PMF_POINTF_set(5, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_FILLCLOSEDCURVE_set(U_ALTERNATE, 0.0, poBrushID, poPoints);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLCLOSEDCURVE_set\n");
    paf(et, poac, po, "U_PMR_FILLCLOSEDCURVE_set");
    po = U_PMR_DRAWCLOSEDCURVE_set(OBJ_PEN_BLACK_1, 0.0, poPoints);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWCLOSEDCURVE_set\n");
    paf(et, poac, po, "U_PMR_DRAWCLOSEDCURVE_set");
       U_PO_free(&poPoints);
       free(points);
       

    /* Test the moveto, lineto, arcto, polylineto and polybezierto functions.  These are not part of EMF+ per se,
    but are utility functions that lie on top of it. Do not fill.  The paths are arbitrary and some of the
    fill results would look strange (filled open curves look odd).  */

    U_DPO_clear(dpath);
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){4000,1300}, U_PTP_None),__LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4300,1500}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4300,1700}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4000,1500}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");

    U_DPO_clear(dpath);
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){4400,1300}, U_PTP_None),__LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4700,1500}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4700,1700}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4400,1500}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    IfNotTrue(U_PATH_closepath((dpath)),                                  __LINE__, "OOPS on U_PATH_closepath\n");
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){4600,1300}, U_PTP_None),__LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4700,1300}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    IfNotTrue(U_PATH_lineto(dpath, (U_PMF_POINTF){4700,1425}, U_PTP_None),__LINE__, "OOPS on U_PATH_lineto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");

    /* same shape as the last one, but constructed in a different way */
    U_DPO_clear(dpath);
    Pointfs[0] = (U_PMF_POINTF) {4800,1300};
    Pointfs[1] = (U_PMF_POINTF) {5100,1500};
    Pointfs[2] = (U_PMF_POINTF) {5100,1700};
    Pointfs[3] = (U_PMF_POINTF) {4800,1500};
    Pointfs[4] = (U_PMF_POINTF) {5000,1300};
    Pointfs[5] = (U_PMF_POINTF) {5100,1300};
    Pointfs[6] = (U_PMF_POINTF) {5100,1425};
       IfNullPtr(dpath,__LINE__,"OOPS on U_PATH_create\n");
    IfNotTrue(U_PATH_polylineto(dpath, 2, &Pointfs[0], U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
    IfNotTrue(U_PATH_polylineto(dpath, 2, &Pointfs[2], U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_polylineto\n");
    IfNotTrue(U_PATH_closepath((dpath)),                                          __LINE__, "OOPS on U_PATH_closepath\n");
    IfNotTrue(U_PATH_polylineto(dpath, 3, &Pointfs[4], U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");

    U_DPO_clear(dpath);
    Pointfs[0] = (U_PMF_POINTF) {5200,1300};
    Pointfs[1] = (U_PMF_POINTF) {5500,1500};
    Pointfs[2] = (U_PMF_POINTF) {5500,1700};
    Pointfs[3] = (U_PMF_POINTF) {5200,1500};
    Pointfs[4] = (U_PMF_POINTF) {5400,1300};
    Pointfs[5] = (U_PMF_POINTF) {5500,1300};
    Pointfs[6] = (U_PMF_POINTF) {5500,1425};
    IfNotTrue(U_PATH_polybezierto(dpath, 4, &Pointfs[0], U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polybezierto\n");
    IfNotTrue(U_PATH_polybezierto(dpath, 3, &Pointfs[4], U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_polybezierto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");


    U_DPO_clear(dpath);
    Rectfs[0] = (U_PMF_RECTF){5900,1300, 300, 600};
    IfNotTrue(U_PATH_arcto(dpath,   0.0, -90.0, 45.0, &Rectfs[0], U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_arcto\n");
    IfNotTrue(U_PATH_arcto(dpath, 180.0, -90.0, 45.0, &Rectfs[0], U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_arcto\n");
    IfNotTrue(U_PATH_arcto(dpath,  30.0,  30.0, 45.0, &Rectfs[0], U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_arcto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       U_PO_free(&poPath);
    po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_GROUP1);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
    paf(et, poac, po, "U_PMR_DRAWPATH_set");

 
    /* Test the multiple rects functions with multiple rects */
    Rectfs[0] = (U_PMF_RECTF){5900, 2000, 300, 300};
    Rectfs[1] = (U_PMF_RECTF){6000, 2100, 300, 300};
    Rectfs[2] = (U_PMF_RECTF){6100, 2200, 300, 300};
    poRectfs  = U_PMF_RECTFN_set(3, Rectfs);
       IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
    po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLRECTS_set\n");
    paf(et, poac, po, "U_PMR_FILLRECTS_set");
    po = U_PMR_DRAWRECTS_set(OBJ_PEN_GROUP1, poRectfs);
    paf(et, poac, po, "U_PMR_DRAWRECTS_set");
       U_PO_free(&poRectfs);
       
    /* Test the multiple rects functions with multiple rects (int)*/
    Rects[0] = (U_PMF_RECT){5900, 2600, 300, 300};
    Rects[1] = (U_PMF_RECT){6000, 2700, 300, 300};
    Rects[2] = (U_PMF_RECT){6100, 2800, 300, 300};
    poRects  = U_PMF_RECTN_set(3, Rects);
       IfNullPtr(poRects,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
    po = U_PMR_FILLRECTS_set(poBrushID, poRects);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLRECTS_set\n");
    paf(et, poac, po, "U_PMR_FILLRECTS_set");
    po = U_PMR_DRAWRECTS_set(OBJ_PEN_GROUP1, poRects);
    paf(et, poac, po, "U_PMR_DRAWRECTS_set");
       U_PO_free(&poRects);
       
    /* a series of red bounded, grey filled shapes, using U_PMF_POINTF points */
    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 100, 1800));
    IfNotTrue(U_PATH_polybezierto(dpath, 7, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polybezierto\n");
    IfNotTrue(U_PATH_closepath((dpath)),                                   __LINE__, "OOPS on U_PATH_closepath\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 600, 1800));
    IfNotTrue(U_PATH_polylineto(dpath, 6, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
    IfNotTrue(U_PATH_closepath((dpath)),                                   __LINE__, "OOPS on U_PATH_closepath\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1100, 1800));
    IfNotTrue(U_PATH_polylineto(dpath, 6, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1600, 1800));
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){1600,1800}, U_PTP_None), __LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_polybezierto(dpath, 6, points, U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_polybezierto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 2100, 1800));
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){2100,1800}, U_PTP_None), __LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_polylineto(dpath, 6, points, U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_polylineto\n");
    poPath = U_PMF_PATH_set2(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 
    /* a series of red bounded, grey filled shapes, using U_PMF_POINT (int16) points, conversion from float to int16 in _set3 function */

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 100, 2300));
    IfNotTrue(U_PATH_polybezierto(dpath, 7, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polybezierto\n");
    IfNotTrue(U_PATH_closepath((dpath)),                                   __LINE__, "OOPS on U_PATH_closepath\n");
    poPath = U_PMF_PATH_set3(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 600, 2300));
    IfNotTrue(U_PATH_polylineto(dpath, 6, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
    IfNotTrue(U_PATH_closepath((dpath)),                                   __LINE__, "OOPS on U_PATH_closepath\n");
    poPath = U_PMF_PATH_set3(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1100, 2300));
    IfNotTrue(U_PATH_polylineto(dpath, 6, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
    poPath = U_PMF_PATH_set3(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1600, 2300));
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){1600,2300}, U_PTP_None), __LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_polybezierto(dpath, 6, points, U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_polybezierto\n");
    poPath = U_PMF_PATH_set3(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 

    U_DPO_clear(dpath);
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 2100, 2300));
    IfNotTrue(U_PATH_moveto(dpath, (U_PMF_POINTF){2100,2300}, U_PTP_None), __LINE__, "OOPS on U_PATH_moveto\n");
    IfNotTrue(U_PATH_polylineto(dpath, 6, points, U_PTP_None, U_SEG_OLD),__LINE__, "OOPS on U_PATH_polylineto\n");
    poPath = U_PMF_PATH_set3(Version, dpath);
       IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
    po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
    po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
    paf(et, poac, po, "U_PMR_drawfill");
       U_PO_free(&poPath);
       free(points);
 
     /* ********************************************************************** */
    // misc draw functions

    // beziers
    points = pointfs_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3800, 1800));
    poPoints = U_PMF_POINTF_set(7, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_DRAWBEZIERS_set(OBJ_PEN_RED_10, poPoints);
    paf(et, poac, po, "U_PMR_DRAWBEZIERS_set");
       U_PO_free(&poPoints);
       free(points);
 
    // polygonfill
    points = pointfs_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3800, 2300));
    poPoints = U_PMF_POINTF_set(6, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
    po = U_PMR_FILLPOLYGON_set(poBrushID,poPoints);
    paf(et, poac, po, "U_PMR_FILLPOLYGON_set");
       U_PO_free(&poPoints);
       free(points);
 
    // curves with varying tension
    for(j=2;j<5;j++){
    for(i=0;i<5;i++){
       points = pointfs_transform(plarray, 5, xform_alt_set(0.25, 1.0, 0.0, 0.0, 4000 + i*200, 1800 + (j-2)*200));
       poPoints = U_PMF_POINTF_set(5, points);
          IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTF_set\n");
       po = U_PMR_DRAWCURVE_set(OBJ_PEN_RED_10, i, 0, j, poPoints);
       paf(et, poac, po, "U_PMR_DRAWCURVE_set");
          U_PO_free(&poPoints);
          free(points);
    }
    }
    

     /* ********************************************************************** */
    // test transform routines, first generate a circle of points

    for(i=0; i<12; i++){
       pl1     = (U_PMF_POINTF){1,0};
       points  = pointfs_transform(&pl1,1,xform_alt_set(100.0, 1.0, (float)(i*30), 0.0, 0.0, 0.0));
       pl12[i] = *points;
       free(points);
    }
    pl12[0].X = U_ROUND((float)pl12[0].X) * 1.5; // make one points stick out a bit

    // test scale (range 1->2) and axis ratio (range 1->0)
    for(i=0; i<12; i++){
       U_DPO_clear(dpath);
       points = pointfs_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 0.0, 30.0*((float)i), 200 + i*300, 2800));
       IfNotTrue(U_PATH_polygon(dpath, 12, points, U_PTP_None),__LINE__, "OOPS on U_PATH_polygon\n");
       poPath = U_PMF_PATH_set2(Version, dpath);
          IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
       po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
       paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
       paf(et, poac, po, "U_PMR_drawfill");
          U_PO_free(&poPath);
          free(points);
    }

    // test scale (range 1->2) and axis ratio (range 1->0), rotate major axis to vertical (point lies on positive Y axis)
    // use int16 instead of float coordinates
    for(i=0; i<12; i++){
       U_DPO_clear(dpath);
       points = pointfs_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 90.0, 30.0*((float)i), 200 + i*300, 3300));
       IfNotTrue(U_PATH_polygon(dpath, 12, points, U_PTP_None),__LINE__, "OOPS on U_PATH_polygon\n");
       poPath = U_PMF_PATH_set3(Version, dpath);
          IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set2\n");
       po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
       paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       po = U_PMR_drawfill(OBJ_PATH_1,OBJ_PEN_RED_10, poBrushID);
       paf(et, poac, po, "U_PMR_drawfill");
          U_PO_free(&poPath);
          free(points);
    }
    
    // test polypolyline and polypolygon using this same array.

    not_in_pmf(1.0, 4000 - 100, 2800 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolyline  */
    not_in_pmf(1.0, 4000 - 100, 3300 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolyline16  */

    not_in_pmf(1.0, 4300 - 100, 2800 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolygon  */
    not_in_pmf(1.0, 4300 - 100, 3300 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolygon16  */

 
    not_in_pmf(1.0, 4600 - 100, 2800 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolyline  */
    not_in_pmf(1.0, 4600 - 100, 3300 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolyline16   */

    not_in_pmf(1.0, 4900 - 100, 2800 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolygon  */
    not_in_pmf(1.0, 4900 - 100, 3300 - 100, OBJ_PEN_RED_10, poBrushDarkRedID, et, dpath, poac);  /* PMF has no polypolygon16  */

    /* Done with OBJ_BRUSH_GRAY now */   
    U_PO_free(&poBrushID);

    /*  gradientfill
    */


    /* ********************************************************************** */
    // line types

 
    pl12[0] = (U_PMF_POINTF){  0,   0};
    pl12[1] = (U_PMF_POINTF){200, 200};
    pl12[2] = (U_PMF_POINTF){  0, 400};
    pl12[3] = (U_PMF_POINTF){  2,   1}; // these next two are actually used as the pattern for U_PS_USERSTYLE
    pl12[4] = (U_PMF_POINTF){  4,   3}; // dash =2, gap=1, dash=4, gap =3
    for(i=0; i<U_DD_Types+7; i++){ 
       /* make the dashed pen - this is quite involved... */
       if(i>=1 && i<U_DD_Types){ /* dashed lines using one of our predefined types */
          poDashes = U_PMF_DASHEDLINEDATA_set2(50.0, i);
          IfNullPtr(poDashes,__LINE__,"OOPS on U_PMF_DASHEDLINEDATA_set2\n");
          poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle | U_PD_DLData,
                  NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Miter,
                  10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                  poDashes, U_PA_Center, NULL, NULL, NULL);
          U_PO_free(&poDashes);
       }
       else if(i==0){ /* solid line */
          poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                  NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Miter,
                  10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                  NULL, U_PA_Center, NULL, NULL, NULL);
       }
       else { /* dot/dash set by bit patterns, lowest bit must always be set*/
          poDashes = U_PMF_DASHEDLINEDATA_set3(50.0, (i<<16) | 0x7F00FFFF); /* lowest bit set, highest bit clear */
          IfNullPtr(poDashes,__LINE__,"OOPS on U_PMF_DASHEDLINEDATA_set3\n");
          poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle | U_PD_DLData,
                  NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Miter,
                  10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                  poDashes, U_PA_Center, NULL, NULL, NULL);
          U_PO_free(&poDashes);
        }
          IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");
       poPenD    = U_PMF_PENDATA_set(U_UT_World, 1.0, poPenOD);
          IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");

       poColor   = U_PMF_ARGB_set(255,255,0,255);
          IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
       poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
       IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
       poBrush   = U_PMF_BRUSH_set(Version, poBd);
          IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");

       poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
          IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
       po        = U_PMR_OBJECT_PO_set(OBJ_PEN_FUCHSIA_1, poPen);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
       paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_FUCHSIA_1");
          U_PO_free(&poPen);
          U_PO_free(&poPenD);
          U_PO_free(&poPenOD);
          U_PO_free(&poColor);
          U_PO_free(&poBd);
          U_PO_free(&poBrush);

       U_DPO_clear(dpath);
       points = pointfs_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 100 + i*50, 3700));
       IfNotTrue(U_PATH_polylineto(dpath, 3, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
       poPath = U_PMF_PATH_set2(Version, dpath);
          IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
       po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
       paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
       po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_FUCHSIA_1);
          IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
       paf(et, poac, po, "U_PMR_drawfill");
          U_PO_free(&poPath);
          free(points);
    }

    /* run over all combinations of cap(join(miter 1,5))), but draw as solid lines with World width 20 */
    pl12[0] = (U_PMF_POINTF){0,   0  };
    pl12[1] = (U_PMF_POINTF){600, 200};
    pl12[2] = (U_PMF_POINTF){0,   400};
    for (i=0; i<9; i++){
       switch(i){
          case 0: cap = U_LCT_Flat;           break;
          case 1: cap = U_LCT_Square;         break;
          case 2: cap = U_LCT_Round;          break;
          case 3: cap = U_LCT_Triangle;       break;
          case 4: cap = U_LCT_NoAnchor;       break;
          case 5: cap = U_LCT_SquareAnchor;   break;
          case 6: cap = U_LCT_RoundAnchor;    break;
          case 7: cap = U_LCT_DiamondAnchor;  break;
          case 8: cap = U_LCT_ArrowAnchor;    break;
       }
       for(join=U_LJT_Miter; join<=U_LJT_MiterClipped; join++){
          for(miter=1;miter<=5;miter+=4){
             /* have to completely remake the pen */
             poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                     NULL, cap, cap, join,
                     miter, U_LS_Solid, U_DLCT_Flat, 0.0,
                     NULL, U_PA_Center, NULL, NULL, NULL);
                IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");
             poPenD    = U_PMF_PENDATA_set(U_UT_World, 20.0, poPenOD);
                IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");

             poColor   = U_PMF_ARGB_set(255,0,0,0);
                IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
             poBd      = U_PMF_SOLIDBRUSHDATA_set(poColor);
             IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_SOLIDBRUSHDATA_set\n");
             poBrush   = U_PMF_BRUSH_set(Version, poBd);
                IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");

             poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
                IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
             po        = U_PMR_OBJECT_PO_set(OBJ_PEN_BLACK_20, poPen);
                IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
             paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_BLACK_20");
                U_PO_free(&poPen);
                U_PO_free(&poPenD);
                U_PO_free(&poPenOD);
                U_PO_free(&poColor);
                U_PO_free(&poBd);
                U_PO_free(&poBrush);


             U_DPO_clear(dpath);
             points = pointfs_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 100 + i*250 + miter*20, 4400 + join*450));
             IfNotTrue(U_PATH_polylineto(dpath, 3, points, U_PTP_None, U_SEG_NEW),__LINE__, "OOPS on U_PATH_polylineto\n");
             poPath = U_PMF_PATH_set2(Version, dpath);
                IfNullPtr(poPath,__LINE__,"OOPS U_PMF_PATH_set3\n");
             po = U_PMR_OBJECT_PO_set(OBJ_PATH_1, poPath);
                IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
             paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PATH_1");
             po = U_PMR_DRAWPATH_set(OBJ_PATH_1, OBJ_PEN_BLACK_20);
                IfNullPtr(po,__LINE__,"OOPS on U_PMR_DRAWPATH_set\n");
             paf(et, poac, po, "U_PMR_drawfill");
                U_PO_free(&poPath);
                free(points);
          }
       }
    }

    
    /* ********************************************************************** */
    // bitmaps
    
    offset = 5000;
//    label_column(poac, offset, 5000, &font, et, eht);
//    label_row(poac, offset + 400, 5000 - 30, &font, et,  eht);
    offset += 400;
   
    /* Make the first test image, it is 10 x 10 and has various colors, R,G,B in each of 3 corners.
       It will be used to generate bitmaps of various kinds, only be free'd after that
    */
    rgba_px = (char *) malloc(10*10*4);
    FillImage(rgba_px,10,10,40,ALPHA_YES);

    // Test various bitmap types
    colortype = U_BCBM_COLOR32; /* as U_PF_32bppRGB (Alpha channel is ignored) */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){10,10,40,U_PF_32bppRGB,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;

    colortype = U_BCBM_COLOR32; /* as U_PF_32bppARGB (Alpha channel is referenced)  */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){10,10,40,U_PF_32bppARGB,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;

    colortype = U_BCBM_COLOR24; /* as U_PF_24bppRGB */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){10,10,32,U_PF_24bppRGB,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;

    colortype = U_BCBM_COLOR16; /* as U_PF_16bppRGB555 */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){10,10,20,U_PF_16bppRGB555,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;

    colortype = U_BCBM_COLOR16; /* as U_PF_16bppRGB555, this time invert row order */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_INVERT), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){10,10,20,U_PF_16bppRGB555,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;

    colortype = U_BCBM_COLOR16; /* Interpret as U_PF_16bppGrayScale */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){10,10,20,U_PF_16bppGrayScale,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;

    /* The next one does not work - it gets into the file but does not render in XP Preview */
    colortype = U_BCBM_COLOR8; /* asU_PF_8bppIndexed */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, U_CT_ARGB, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){10,10,12,U_PF_8bppIndexed,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;  
    
    /* EMF has a numCt = 0 form, EMF+ does not seem to */

   // done with the first test image, make the 2nd
    free(rgba_px); 
    rgba_px = (char *) malloc(4*4*4);
    FillImage(rgba_px,4,4,16,ALPHA_NO);

    colortype = U_BCBM_COLOR4; /* as U_PF_4bppIndexed, this time invert row order */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_ARGB, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){4,4,4,U_PF_4bppIndexed,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;  
    
    /* EMF has a numCt = 0 form, EMF+ does not seem to */
    

    // make a two color image in the existing RGBA array, setting A to 255
    rgba_px2 = rgba_px;
    for(i=0;i<2;i++){
       for(j=0;j<4;j++){
          for(k=0;k<3;k++){ memset(rgba_px2++, 0x55, 1); }
          memset(rgba_px2++, 0xFF, 1); 
       }
    }
    for(i=0;i<2;i++){
       for(j=0;j<4;j++){
          for(k=0;k<3;k++){ memset(rgba_px2++, 0xAA, 1); }
          memset(rgba_px2++, 0xFF, 1); 
       }
    }

    colortype = U_BCBM_MONOCHROME;
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_ARGB, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){4,4,4,U_PF_1bppIndexed,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 220;  
    
    colortype = U_BCBM_MONOCHROME;  /* invert this one */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, U_CT_ARGB, U_ROW_ORDER_INVERT), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){4,4,4,U_PF_1bppIndexed,U_BDT_Pixel};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    offset += 620;  

    free(rgba_px);

    Bs = (U_PMF_BITMAP){0,0,0,U_PF_Undefined,U_BDT_Compressed};
    image_column(et, offset, 5000, 200, 200, CMP_IMG_YES, &Bs, 0, NULL, PNG_ARRAY_SIZE, (char *)pngarray, poac);
    offset += 220;  
    image_column(et, offset, 5000, 200, 200, CMP_IMG_YES, &Bs, 0, NULL, 676, (char *)jpgarray, poac);
    // offset += 220; // clang static analyzer does not like this  

    // test image effects.  These seem to have no effect when viewed in XP Preview.
    rgba_px = (char *) malloc(100*100*4);
    FillImage2(rgba_px,100,100,400,ALPHA_YES);
    colortype = U_BCBM_COLOR32; /* as U_PF_32bppRGB (Alpha channel is ignored) */
    IfNotTrue(!RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  100,100,400, colortype, U_CT_NO, U_ROW_ORDER_SAME), __LINE__, "OOPS on RGBA_to_DIB\n");
    Bs = (U_PMF_BITMAP){100,100,400,U_PF_32bppRGB,U_BDT_Pixel};
    image_column2(et, 10000, 5000, 200, 200, CMP_IMG_NO, &Bs, numCt, (U_PMF_ARGB *)ct, cbPx, px, poac);
    free(ct);
    free(px);
    free(rgba_px);

    
    // EMF+ does not have binary raster operations
    
    /* ***************    test all text alignments  *************** */
    poColor   = U_PMF_ARGB_set(255,255,0,255);
    IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");

    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    slen = strlen("Courier New");
    poFont = U_PMF_FONT_set(U_PMF_GRAPHICSVERSIONOBJ_set(2), 40, U_UT_Pixel, U_FS_None, slen, FontName);
    IfNullPtr(poFont,__LINE__,"OOPS on U_PMF_FONT_set\n");
    free(FontName);

    po        = U_PMR_OBJECT_PO_set(OBJ_FONT_OTHER, poFont); /* font to use */
    IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "set font in Text Alignments");
    U_PO_free(&poFont);

    string = (char *) malloc(STRALLOC);
    poBrushID = U_PMF_ARGB_set(255, 128, 128, 128);
       IfNullPtr(poBrushID,__LINE__,"OOPS on U_PMF_ARGB_set\n");

    /* draw all the little squares */
    i = 0;
    for(iay=0;iay<3; iay++){ /* all combinations of near, center, far */
       for(iax=0;iax<3; iax++, i++){ /* all combinations of near, center, far */
          Rectfs[0] = (U_PMF_RECTF){6000-20,100+ i*50-20, 40, 40};
          poRectfs  = U_PMF_RECTFN_set(1, Rectfs);
             IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
          po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_FILLRECTS_set\n");
          paf(et, poac, po, "U_PMR_FILLRECTS_set");
             U_PO_free(&poRectfs);
       }
    }

    /* draw all the text */
    i = 0;
    for(iay=0;iay<3; iay++){ /* all combinations of near, center, far */
       for(iax=0;iax<3; iax++, i++){ /* all combinations of near, center, far */
          Sfs.StringAlignment   = iax; //  Horizontal
          Sfs.LineAlign         = iay; //  Vertical
          poSF = U_PMF_STRINGFORMAT_set(&Sfs, NULL);
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");

          po = U_PMR_OBJECT_PO_set(OBJ_SF_TCOMMENT, poSF);
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
             U_PO_free(&poSF);
          paf(et, poac, po, "set OBJ_SF_TCOMMENT in Text Alignments");
             
          sprintf(string,"textalignment:%d_%d",iax,iay);
          /* Note that the text is drawn 20 pixels higher than the squares (half the font height) */
          textlabel(poac, 40, string,      6000,100+ i*50 -20, iax, iay, poColor, et);
       }
    }
    U_PO_free(&poBrushID);
    free(string);
    
    /* ***************    test rotated text  *************** */
    /* use current font and text color */
    
    Sfs.StringAlignment   = U_SA_Near; //  Horizontal
    Sfs.LineAlign         = U_SA_Near; //  Vertical
    poSF = U_PMF_STRINGFORMAT_set(&Sfs, NULL);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    po = U_PMR_OBJECT_PO_set(OBJ_SF_TCOMMENT, poSF);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
       U_PO_free(&poSF);
    paf(et, poac, po, "set OBJ_SF_TCOMMENT in Text Alignments");

    spintext(poac, 40, 8000, 300, U_SA_Near, U_SA_Near, poColor, et);
    U_PO_free(&poColor);
    
    /* ***************    test bold etc.  text  *************** */

    fip = &fi[0];
    poColor   = U_PMF_ARGB_set(255,0,0,0);
    IfNullPtr(poColor,__LINE__,"OOPS on U_PMF_ARGB_set\n");
    string = (char *) malloc(64);
    Sfs.StringAlignment   = U_SA_Near; //  Horizontal
    Sfs.LineAlign         = U_SA_Near; //  Vertical
    for (i=0;i<16;i++){ // all combinations of FS flags
          
       sprintf(string,"Text: %s%s%s%s%s",
          (i == U_FS_None      ? "Plain"     : ""),
          (i &  U_FS_Bold      ? "Bold "     : ""),
          (i &  U_FS_Italic    ? "Italic "   : ""),
          (i &  U_FS_Underline ? "Underline ": ""),
          (i &  U_FS_Strikeout ? "Strikeout" : "")
       );
       (void) U_PMR_drawstring(string, U_SA_Near, OBJ_FONT_OTHER, poColor, OBJ_SF_TCOMMENT, Sfs, 
             fip->name, 40.0, fip, i,  5000,100+ i*50, poac, et);
    }
    free(string);
    U_PO_free(&poColor);


    /* ***************    test hatched fill and stroke (standard patterns)  *************** */
    

    /* *** fill *** */
    Rectfs[0]  = (U_PMF_RECTF){0, 0, 100, 300};
    Pointfs[0] = (U_PMF_POINTF) {50,  0};
    Pointfs[1] = (U_PMF_POINTF) {50,300};
    poBrushID = U_PMF_4NUM_set(OBJ_BRUSH_GROUP1);
    poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                   NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Miter,
                   10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                   NULL, U_PA_Center, NULL, NULL, 
                   NULL);
       IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");
    poPenD    = U_PMF_PENDATA_set(U_UT_World, 75, poPenOD);
       IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");
       U_PO_free(&poPenOD);
    for(k=0;k<2;k++){
       if(k==0){
          // blue background, transparent background
          poFg   = U_PMF_ARGB_set(255, 0, 0, 255);
             IfNullPtr(poFg,__LINE__,"OOPS on U_PMF_ARGB_set\n");
          poBg   = U_PMF_ARGB_set(0, 0, 0, 0);
             IfNullPtr(poBg,__LINE__,"OOPS on U_PMF_ARGB_set\n");
       }
       else {
          //repeat with red background, green text
          poFg   = U_PMF_ARGB_set(255, 0, 255, 0);
             IfNullPtr(poFg,__LINE__,"OOPS on U_PMF_ARGB_set\n");
          poBg   = U_PMF_ARGB_set(255, 255, 0, 0);
             IfNullPtr(poBg,__LINE__,"OOPS on U_PMF_ARGB_set\n");
       }
       for(i=0;i<=U_HSP_SolidDiamond;i++){
          poBd = U_PMF_HATCHBRUSHDATA_set(i, poFg, poBg);
             IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_HATCHBRUSHDATA_set\n");
          poBrush   = U_PMF_BRUSH_set(Version, poBd);
             IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");
          po        = U_PMR_OBJECT_PO_set(OBJ_BRUSH_GROUP1, poBrush);
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
          paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_BRUSH_GROUP1");
             U_PO_free(&poBd);

          poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
             IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
          po        = U_PMR_OBJECT_PO_set(OBJ_PEN_GROUP1, poPen);
             IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
          paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_GROUP1");
             U_PO_free(&poBrush);
             U_PO_free(&poPen);

          pRectfs = rectfs_transform(Rectfs, 1, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*105, 3500 + k*310));
          poRectfs  = U_PMF_RECTFN_set(1, pRectfs);
             IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
          po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
          paf(et, poac, po, "U_PMR_FILLRECTS_set");
          po = U_PMR_DRAWRECTS_set(OBJ_PEN_BLACK_1, poRectfs);
          paf(et, poac, po, "U_PMR_DRAWRECTS_set");
             U_PO_free(&poRectfs);
             free(pRectfs);

          points = pointfs_transform(Pointfs, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+(i+54)*105, 3500 + k*310));
          poPoints = U_PMF_POINTF_set(2, points);
             IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTR_set\n");
          po = U_PMR_DRAWLINES_set(OBJ_PEN_GROUP1, 0, poPoints);
          paf(et, poac, po, "U_PMR_DRAWLINES_set");
             U_PO_free(&poPoints);
             free(points);

       }
       U_PO_free(&poFg);
       U_PO_free(&poBg);
    }
    U_PO_free(&poPenD);
    U_PO_free(&poBrushID);
    
    /* ***************    test image fill and stroke  *************** */
     
    // this will draw a series of squares of increasing size, all with the same color fill pattern  
    
    // Make the first test image, it is 5 x 5 and has various colors, R,G,B in each of 3 corners
    rgba_px = (char *) malloc(10*10*4);
    FillImage(rgba_px,10,10,40,0); // disable Alpha

    Bs = (U_PMF_BITMAP){10,10,40,U_PF_32bppARGB,U_BDT_Pixel};

    colortype = U_BCBM_COLOR32;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px, 10, 10, 40, colortype, U_CT_NO, U_ROW_ORDER_SAME);
    if(status)printf_and_flush("error at RGBA_to_DIB with colortype U_BCBM_COLOR32\n"); 
    poBmd = U_PMF_BITMAPDATA_set(NULL, cbPx, px); // No palette
       IfNullPtr(poBmd,__LINE__,"OOPS on U_PMF_BITMAPDATA_set\n");
    poBm = U_PMF_BITMAP_set(&Bs, poBmd);
       IfNullPtr(poBm,__LINE__,"OOPS on U_PMF_BITMAP_set\n");
    poImg = U_PMF_IMAGE_set(Version, poBm);
       IfNullPtr(poImg,__LINE__,"OOPS on U_PMF_IMAGE_set\n");

    poTbod = U_PMF_TEXTUREBRUSHOPTIONALDATA_set(NULL, poImg); // No Transform Matrix
       IfNullPtr(poTbod,__LINE__,"OOPS on U_PMF_TEXTUREBRUSHOPTIONALDATA_set\n");
       U_PO_free(&poImg);
       U_PO_free(&poBm);
       U_PO_free(&poBmd);
       free(px);
       free(rgba_px);
    poBd = U_PMF_TEXTUREBRUSHDATA_set(U_BD_None, U_WM_Tile, poTbod);
       IfNullPtr(poBd,__LINE__,"OOPS on U_PMF_TEXTUREBRUSHDATA_set\n");
    poBrush   = U_PMF_BRUSH_set(Version, poBd);
       IfNullPtr(poBrush,__LINE__,"OOPS on U_PMF_BRUSH_set\n");
    po = U_PMR_OBJECT_PO_set(OBJ_BRUSH_GROUP1, poBrush);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_BRUSH_GROUP1");
       U_PO_free(&poTbod);
       U_PO_free(&poBd);
    poBrushID = U_PMF_4NUM_set(OBJ_BRUSH_GROUP1);
       IfNullPtr(poBrushID,__LINE__,"OOPS on U_PMF_4NUM_set\n");

    /* filled pen, will be used to draw just one line */
    poPenOD   = U_PMF_PENOPTIONALDATA_set(U_PD_StartCap |U_PD_EndCap |U_PD_Join |U_PD_MiterLimit |U_PD_LineStyle,
                   NULL, U_LCT_Flat, U_LCT_Flat, U_LJT_Miter,
                   10.0, U_LS_Solid, U_DLCT_Flat, 0.0,
                   NULL, U_PA_Center, NULL, NULL, 
                   NULL);
       IfNullPtr(poPenOD,__LINE__,"OOPS on U_PMF_PENOPTIONALDATA_set\n");
    poPenD    = U_PMF_PENDATA_set(U_UT_World, 50, poPenOD);
       IfNullPtr(poPenD,__LINE__,"OOPS on U_PMF_PENDATA_set\n");


    poPen     = U_PMF_PEN_set(Version, poPenD, poBrush);
       IfNullPtr(poPen,__LINE__,"OOPS on U_PMF_PEN_set\n");
    po        = U_PMR_OBJECT_PO_set(OBJ_PEN_GROUP1, poPen);
       IfNullPtr(po,__LINE__,"OOPS on U_PMR_OBJECT_PO_set\n");
    paf(et, poac, po, "U_PMR_OBJECT_PO_set OBJ_PEN_GROUP1");
       U_PO_free(&poPen);
       U_PO_free(&poPenD);
       U_PO_free(&poPenOD);
       U_PO_free(&poBrush);


    /* draw the image filled squares */
    for(i=1;i<=10;i++){
       Rectfs[0] = (U_PMF_RECTF){2000+i*330, 4160, 30*i,30*i};
       poRectfs  = U_PMF_RECTFN_set(1, Rectfs);
          IfNullPtr(poRectfs,__LINE__,"OOPS on U_PMF_RECTFN_set\n");
       po = U_PMR_FILLRECTS_set(poBrushID, poRectfs);
       paf(et, poac, po, "U_PMR_DRAWRECTS_set");
          U_PO_free(&poRectfs);
    }
    
    /* draw the image filled line (there is just 1) */
    points = pointfs_transform(Pointfs, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2000+i*330, 4160));
    poPoints = U_PMF_POINTF_set(2, points);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMF_POINTR_set\n");
    po = U_PMR_DRAWLINES_set(OBJ_PEN_GROUP1, 0, poPoints);
       IfNullPtr(poPoints,__LINE__,"OOPS on U_PMR_DRAWLINES_set\n");
    paf(et, poac, po, "U_PMR_DRAWLINES_set");
       U_PO_free(&poPoints);
       free(points);
    
    U_PO_free(&poBrushID);
    /*************** EMF+ does not have mono fill  *************** */

    /*  gradientfill */
    offset = 3000;
    gradient_column(et, offset, 5000, 300, 300, poac);

    /* Test clipping regions */
    if(!(mode & NO_CLIP_TEST)){
       test_clips(et, 6700,1300, poac);
    }

/* WORKING HERE, INSERT NEW CODE ABOVE */


/* ************************************************* */

//
//  careful not to call anything after here twice!!! 
//
    po = U_PMR_ENDOFFILE_set();
    IfNullPtr(po,__LINE__,"OOPS on U_PMR_ENDOFFILE_set\n");
    paf(et, poac, po, "U_PMR_ENDOFFILE_set");

    rec = U_EMREOF_set(0,NULL,et);
    taf(rec,et,"U_EMREOF_set");

    /* Test the endian routines (on either Big or Little Endian machines).
    This must be done before the call to emf_finish, as that will swap the byte
    order of the EMF data before it writes it out on a BE machine.  The PMF data
    should always be in the same byte order, and it should not change, as it is
    hidden away inside the comment payloads, which are not swapped.  */
    
#if 1
    string = (char *) malloc(et->used);
    if(!string){
       printf("Could not allocate enough memory to test u_emf_endian() function\n");
    }
    else {
       memcpy(string,et->buf,et->used);
       if(!U_emf_endian(et->buf,et->used,1)){
          printf("Error in byte swapping of completed EMF, native -> reverse\n");
       }
       if(!U_emf_endian(et->buf,et->used,0)){
          printf("Error in byte swapping of completed EMF, reverse -> native\n");
       }
       int oops_byte=rgba_diff(string, et->buf, et->used, 0);
       if(oops_byte){ 
          printf("Error in u_emf_endian() function, round trip byte swapping does not match original\n");
          printf("Error in u_emf_endian() function at byte %d\n",oops_byte);
       }
       free(string);
    }
#endif // swap testing

    status=emf_finish(et, eht);
    if(status){ printf("emf_finish failed: %d\n", status); }
    else {      printf("emf_finish sucess\n");             }

    U_DPO_free(&dpath);
    U_PO_free(&poac);
    U_PO_free(&poBrushDarkRedID);
    
    emf_free(&et);
    emf_htable_free(&eht);

  exit(EXIT_SUCCESS);
}
