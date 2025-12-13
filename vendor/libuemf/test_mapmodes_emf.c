/**
 Example progam which creates test files with types MM_TEXT thorugh MM_ANISOTROPIC containing a 
 rectangle, a drawn "L shape, and some text in each corner.  
 These are called test_mm_text.emf thorugh test_mm_anisotropic.emf.
 
 Compile with 
 
    gcc -O0  -g -std=c99 -Wall -pedantic -o test_mapmodes_emf -Wall -I. test_mapmodes_emf.c uemf.c uemf_endian.c uemf_utf.c -lm
 

File:      test_mapmodes_emf.c
Version:   0.0.11
Date:      12-JUL-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "uemf.h"

#define PPT_BLOCKERS     1
#define PREVIEW_BLOCKERS 2

/* lround not present on older solaris, so define our own function */
int lclround(double val){
  int ret = (val < 0.0 ? val - 0.5 : val + 0.5);
  return(ret);
} 

// noa special conditions:
// 1 then s2 is expected to have zero in the "a" channel
// 2 then s2 is expected to have zero in the "a" channel AND only top 5 bits are meaningful

void taf(char *rec,EMFTRACK *et, char *text){  // Test, append, free
    if(!rec){ printf("%s failed",text);                     }
    else {    printf("%s recsize: %d",text,U_EMRSIZE(rec)); }
    (void) emf_append((PU_ENHMETARECORD)rec, et, 1);
    printf("\n");
#ifdef U_VALGRIND
    fflush(stdout);  // helps keep lines ordered within Valgrind
#endif
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


/* ydir must be either +1.0 or -1.0 */
    
void textlabel(uint32_t size, const char *string, uint32_t x, uint32_t y, uint32_t *font, EMFTRACK *et, EMFHANDLES *eht, int ydir){
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
    rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,ydir,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTW_set");
    free(rec2);
    free(FontName);
    free(FontStyle);
}

/* "to y" (conversion) */
int toy(double yf, int val, int lim){
  int ret;
  if(yf>0){ ret = lclround(yf*((double)val));          }
  else {    ret = lclround(yf*((double)(val-lim)));    }
  return(ret);
}

/* "to x" (conversion) */
int tox(double xf, int val){
  int ret;
  ret = lclround(xf*((double)val));
  return(ret);
}

int lcl_strcasecmp(char *s1, char *s2){
int c1;
int c2;
  for(; ;s1++,s2++){
    c1=toupper(*s1);
    c2=toupper(*s2);
    if(c1 < c2)return -1;
    if(c1 > c2)return  1;
    if(c1 == 0)return  0;  /*c2 also is 0 in this case */
  }
}

void setinumeric(int *val,int *numarg,int argc,char **argv,char * label){
      (*numarg)++;
      if( ( *numarg >= argc ) || (argv[*numarg] == NULL)){
        (void) fprintf( stderr, "%s: missing argument\n",label);
        exit(EXIT_FAILURE);
      }
      if(sscanf(argv[*numarg],"%d",val) != 1){
        (void) fprintf(stderr,"Bad integer argument/parameter [%s %s] \n",label,argv[*numarg]);
        exit(EXIT_FAILURE);
      }
}

void emit_help(void){
   printf("test_mapmodes_emf\n");
   printf("  Purpose:  generates test files in all EMF map modes.\n");
   printf("  Usage:    test_mapmodes_emf -oX N1 -oY N2\n");
   printf("     -h, --h, -?, --?, -help, --help\n");
   printf("                Prints this message\n");
   printf("     -oX,-oY    EMR HEADER Bounds fields offsets in device units. Default 0.\n");
   printf("     -VX,-VY    Viewport offsets in device units.  Default 0.\n");
   printf("                (Window offsets are calculated from oX,VX and oY,VY)\n");
   printf("  Examples:\n");
   printf("      test_mapmodes_emf\n");
   printf("         Generate a set of test files with no offset to the header boundary\n");
   printf("         and both Viewport and Window origins at (0,0).\n");
   printf("      test_mapmodes_emf -oX 2000 -oY 1000\n");
   printf("         Generate a set of test files with the header boundary offset by (2000,1000)\n");
   printf("         and both Viewport and Window origins at (0,0).\n");
   printf("      test_mapmodes_emf -oX 2000 -vX 1000\n");
   printf("         Generate a set of test files with the header boundary offset by (2000,0),\n");
   printf("         the Viewport origin at (1000,0), and Window origins at (set to match,0).\n");
   printf("  Creates:\n");
   printf("       Each test file consists of an A4 sized landscape drawing.  The drawing contains:\n");
   printf("           1.  A light yellow filled, black edged rectangle exactly the size of the drawing.\n");
   printf("           2.  A red filled, black edged \"L\" shaped region at the center left edge.\n");
   printf("           3.  A red filled, black edged \"T\" shaped region at the center top edge.\n");
   printf("           4.  The strings UL, UR, LR, LL in black 12pt Courier New at their respective corners.\n");
   printf("           \n");
   printf("       File                       logical units logical->device scale   Y positive\n");
   printf("       test_mm_text.emf           pixels        1.0                     down \n");
   printf("       test_mm_lometric.emf       0.1    mm     25.4/120.0              up\n");
   printf("       test_mm_himetric.emf       0.01   mm     25.4/12.0               up\n");
   printf("       test_mm_loenglish.emf      0.01   in     1/12.0                  up\n");
   printf("       test_mm_hienglish.emf      0.001  in     1/1.20                  up\n");
   printf("       test_mm_twips.emf          1/1440 in     1.20,                   up\n");
   printf("       test_mm_isotropic.emf      arbitrary     3.0/3.0                 up\n");
   printf("       test_mm_anisotropic.emf    arbitrary     3.0/-4.0                down\n");
   printf("\n");
}


int main(int argc, char *argv[]){
    int                 numarg = 0;
    EMFTRACK            *et;
    EMFHANDLES          *eht;
    char                *rec;
    uint16_t            *Description;
    char                 name[128];
    double               xf,yf;
    U_RECTL              rclBounds,rclFrame,rclBox;
    U_SIZEL              szlDev,szlMm;
    uint32_t             brush=0;
    uint32_t             font=0;;
    U_LOGBRUSH           lb;
    U_COLORREF           cr;
    U_BITMAPINFOHEADER   Bmih;
    PU_BITMAPINFO        Bmi;
    uint32_t             cbDesc;
    uint32_t             cbPx;
    uint32_t             colortype;
    PU_RGBQUAD           ct;         //color table
    int                  numCt;      //number of entries in the color table
    int                  i;
    int                  outX,outY;   /* offset in device space of bounds   */
    int                  VX,VY;       /* offset in device space of viewport */
    int                  WX,WY;       /* offset in logical space of Window - calculated from the two preceding values */
    char                *px;
    char                *rgba_px;
    
    VX=VY=WX=WY=outX=outY=0;
    
    while( ++numarg < argc){
      if( (0==lcl_strcasecmp(argv[numarg], "-h"))     ||
          (0==lcl_strcasecmp(argv[numarg], "--h"))    ||
          (0==lcl_strcasecmp(argv[numarg], "-?"))     ||
          (0==lcl_strcasecmp(argv[numarg], "--?"))    ||
          (0==lcl_strcasecmp(argv[numarg], "-help"))  ||
          (0==lcl_strcasecmp(argv[numarg], "--help")) ){
          emit_help();
          exit(EXIT_FAILURE);
      }
      else if(lcl_strcasecmp(argv[numarg], "-oX")==0){
        setinumeric(&outX,&numarg,argc,argv,"-oX");
        continue;
      }
      else if(lcl_strcasecmp(argv[numarg], "-oY")==0){
        setinumeric(&outY,&numarg,argc,argv,"-oY");
        continue;
      }
      else if(lcl_strcasecmp(argv[numarg], "-VX")==0){
        setinumeric(&VX,&numarg,argc,argv,"-VX");
        continue;
      }
      else if(lcl_strcasecmp(argv[numarg], "-VY")==0){
        setinumeric(&VY,&numarg,argc,argv,"-VY");
        continue;
      }
      else {
        (void) fprintf( stderr, "Unknown command line parameter: %s\n",argv[numarg]);
        exit(EXIT_FAILURE);
      }
    }

 
    /* ********************************************************************** */
    // set up and begin the EMF
 
   for(i=U_MM_MIN; i<=U_MM_MAX; i++){
      switch(i){
         case U_MM_TEXT:
            xf = yf = 1.0;
            strcpy(name,"test_mm_text.emf");
            break;
         case U_MM_LOMETRIC:
            xf = 25.4/120.0;
            yf = -xf;
            strcpy(name,"test_mm_lometric.emf");
            break;
         case U_MM_HIMETRIC:
            xf = 25.4/12.0;
            yf = -xf;
            strcpy(name,"test_mm_himetric.emf");
            break;
         case U_MM_LOENGLISH:
            xf = 1/12.0;
            yf = -xf;
            strcpy(name,"test_mm_loenglish.emf");
            break;
         case U_MM_HIENGLISH:
            xf = 1/1.20;
            yf = -xf;
            strcpy(name,"test_mm_hienglish.emf");
            break;
         case U_MM_TWIPS:
            xf = 1.20;
            yf = -xf;
            strcpy(name,"test_mm_twips.emf");
            break;
         case U_MM_ISOTROPIC:
            xf = 3.0;
            yf = 3.0;
            strcpy(name,"test_mm_isotropic.emf");
            break;
         case U_MM_ANISOTROPIC:
            xf =  3.0;
            yf = -4.0;
            strcpy(name,"test_mm_anisotropic.emf");
            break;
      }

      /*  The general viewport/window transformation is (shown for y only, where y' is in device 
          coordinates and y is in logical coordinates):
             (1)   y' = s*(y - w) + v
          the special case where there are no offsets is:
             (2)   y' = s*y
          if there are offsets then
             (3)   y' + outY = s*y - s*WY + VY
          Subtract 2 from 3 to get
             (4)   outY = -s*WY + VY
          which gives:
             (5)   WY = (outY - VY)/-s = (VY - outY)/s
             
          xf,yf, as defined above scale from device to logical coordinates, so s to go the other way
          is just their reciprocal.
      */
      WX = lclround( ((double)(VX - outX)) / (1.0/(double)xf) ); 
      WY = lclround( ((double)(VY - outY)) / (1.0/(double)yf) );

      (void) emf_start(name,10000, 2500, &et);  // space allocation initial and increment 
      (void) htable_create(128, 128, &eht);
      (void) device_size( 216, 279, 1200./25.4, &szlDev, &szlMm);          // Example: device is Letter vertical, 1200 dpi = 47.244 DPmm
      (void) drawing_size(297, 210.0, 1200./25.4, &rclBounds, &rclFrame);  // Example: drawing is A4 horizontal,  1200 dpi = 47.244 DPmm
      if(yf<0){
         rclBounds.top    = -rclBounds.bottom;
         rclBounds.bottom = 0;
         rclFrame.top     = -rclFrame.bottom;
         rclFrame.bottom  = 0;
      }
      
      /* adjust bounds with offsets */
      rclBounds.left    += outX;
      rclBounds.top     += outY;
      rclBounds.right   += outX;
      rclBounds.bottom  += outY;
      /* adjust frame with somewhat arbitrary offset, apparently the bounds and frames may be offset
      and it is only the size of the frame that matters */
      rclFrame.left     += outX * 0.05;
      rclFrame.top      += outY * 0.05;
      rclFrame.right    += outX * 0.05;
      rclFrame.bottom   += outY * 0.05;

      Description = U_Utf8ToUtf16le("Test EMF\1produced by libUEMF test_mapmodes_emf program\1",0, NULL); 
      cbDesc = 2 + wchar16len(Description);  // also count the final terminator
      (void) U_Utf16leEdit(Description, U_Utf16le(1), 0);
      rec = U_EMRHEADER_set( rclBounds,  rclFrame,  NULL, cbDesc, Description, szlDev, szlMm, 0);
      taf(rec,et,"U_EMRHEADER_set");
      free(Description);

      //
      //  Fill in the EMF, first some housekeeping.
      //

      rec = U_EMRSETMAPMODE_set(i);
      taf(rec,et,"U_EMRSETMAPMODE_set");
      
      rec  = U_EMRSETWINDOWORGEX_set(pointl_set(WX,WY));
      taf(rec,et,"U_EMRSETWINDOWORGEX_set");
      rec  = U_EMRSETVIEWPORTORGEX_set(pointl_set(VX,VY));
      taf(rec,et,"U_EMRSETVIEWPORTORGEX_set");
      rec  = U_EMRSETWINDOWEXTEX_set(sizel_set(tox(xf,14031.0),tox(yf,9921.0))); //use of tox instead of toy is intended here
      taf(rec,et,"U_EMRSETWINDOWEXTEX_set");
      rec  = U_EMRSETVIEWPORTEXTEX_set(sizel_set(14031,9921));
      taf(rec,et,"U_EMRSETVIEWPORTEXTEX_set");
      

      rec = U_EMRSETBKMODE_set(U_TRANSPARENT); //! iMode uses BackgroundMode Enumeration
      taf(rec,et,"U_EMRSETBKMODE_set");

      rec = U_EMRSETMITERLIMIT_set(8);
      taf(rec,et,"U_EMRSETMITERLIMIT_set");

      rec = U_EMRSETPOLYFILLMODE_set(U_WINDING); //! iMode uses PolygonFillMode Enumeration
      taf(rec,et,"U_EMRSETPOLYFILLMODE_set");

      rec = selectobject_set(U_BLACK_PEN, eht);          taf(rec,et,"selectobject_set");

      /* ********** put a rectangle under everything ******************* */

      cr = colorref_set(255,255,173);  //yellowish color, only use here
      lb = logbrush_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
      rec = createbrushindirect_set(&brush, eht,lb);     taf(rec,et,"createbrushindirect_set");
      rec = selectobject_set(brush, eht);                taf(rec,et,"selectobject_set");


      rclBox = rectl_set(pointl_set(0,             toy(yf,   0,9921)),
                         pointl_set(tox(xf,14031), toy(yf,9921,9921)));
      rec = U_EMRRECTANGLE_set(rclBox);                  taf(rec,et,"U_EMRRECTANGLE_set");

      rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);        taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
      rec  = deleteobject_set(&brush, eht);              taf(rec,et,"deleteobject_set");


      /* ********** put text markers in each corner ******************* */
   
      textlabel(tox(xf,200), "UL", tox(xf,   50), toy(yf,  50, 9921), &font, et, eht, (yf < 0 ? -1.0 : 1.0 ));
      textlabel(tox(xf,200), "UR", tox(xf,13750), toy(yf,  50, 9921), &font, et, eht, (yf < 0 ? -1.0 : 1.0 ));
      textlabel(tox(xf,200), "LL", tox(xf,   50), toy(yf,9700, 9921), &font, et, eht, (yf < 0 ? -1.0 : 1.0 ));
      textlabel(tox(xf,200), "LR", tox(xf,13750), toy(yf,9700, 9921), &font, et, eht, (yf < 0 ? -1.0 : 1.0 ));


      /* ********* *Draw a large (red/black outline) L shaped object at left center********** */

      rec  = deleteobject_set(&brush, eht);                            taf(rec,et,"deleteobject_set");
      cr = colorref_set(255, 0, 0);
      lb = logbrush_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
      rec = createbrushindirect_set(&brush, eht,lb);                   taf(rec,et,"createbrushindirect_set");
      rec = selectobject_set(brush, eht);                              taf(rec,et,"selectobject_set");
      rec = U_EMRBEGINPATH_set();                                      taf(rec,et,"U_EMRBEGINPATH_set");
      rec = U_EMRMOVETOEX_set(pointl_set(tox(xf, 500),toy(yf,3750, 9921))); taf(rec,et,"U_EMRMOVETOEX_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf, 500),toy(yf,6250, 9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,2000),toy(yf,6250, 9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,2000),toy(yf,5750, 9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,1000),toy(yf,5750, 9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,1000),toy(yf,3750, 9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRCLOSEFIGURE_set();                                    taf(rec,et,"U_EMRCLOSEFIGURE_set");
      rec = U_EMRENDPATH_set();                                        taf(rec,et,"U_EMRENDPATH_set");
      rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                      taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");

      /* ********* *Draw a large (red/black outline) T shaped object at top center ********** */

      rec = U_EMRBEGINPATH_set();                                      taf(rec,et,"U_EMRBEGINPATH_set");
      rec = U_EMRMOVETOEX_set(pointl_set(tox(xf,6000), toy(yf, 500,9921))); taf(rec,et,"U_EMRMOVETOEX_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,8000), toy(yf, 500,9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,8000), toy(yf,1000,9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,7250), toy(yf,1000,9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,7250), toy(yf,3000,9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,6750), toy(yf,3000,9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,6750), toy(yf,1000,9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRLINETO_set(  pointl_set(tox(xf,6000), toy(yf,1000,9921))); taf(rec,et,"U_EMRLINETO_set");
      rec = U_EMRCLOSEFIGURE_set();                                    taf(rec,et,"U_EMRCLOSEFIGURE_set");
      rec = U_EMRENDPATH_set();                                        taf(rec,et,"U_EMRENDPATH_set");
      rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                      taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");


      // Make a test image, it is 10 x 10 and has various colors, R,G,B in each of 3 corners
      rgba_px = (char *) malloc(10*10*4);
      FillImage(rgba_px,10,10,10*4);
      rec =  U_EMRSETSTRETCHBLTMODE_set(U_STRETCH_DELETESCANS);
      taf(rec,et,"U_EMRSETSTRETCHBLTMODE_set");

      colortype = U_BCBM_COLOR32;
      (void) RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, 0, 1);
      Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
      Bmi = bitmapinfo_set(Bmih, ct);
      free(rgba_px);
      rec = U_EMRSTRETCHDIBITS_set(
         U_RCL_DEF,
         pointl_set(tox(xf,11250), toy(yf,7421,9921)), /* 2500 in from LR corner*/
         pointl_set(tox(xf,2000),  tox(yf,2000)),      /* note: the height may be negative, tox (not toy) for 2nd because only scale needed, not position */
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



      //
      //  careful not to call anything after here twice!!! 
      //

      rec = U_EMREOF_set(0,NULL,et);
      taf(rec,et,"U_EMREOF_set");

      (void) emf_finish(et, eht);
      emf_free(&et);
      htable_free(&eht);
   }
   exit(EXIT_SUCCESS);
}
