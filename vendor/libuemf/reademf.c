/** 
 Example program demonstrating how to code a program to read an EMF file.  This example just examines the contents
 of one EMF file and prints the contents record by record, showing all fields (except for bitmaps).  

 Run like:
    reademf filename.emf
 
 Writes description of the EMF file to stdout.

 Build with:  gcc -Wall -o reademf reademf.c uemf.c uemf_print.c uemf_endian.c uemf_utf.c upmf.c upmf_print.c -lm
*/

/*
File:      reademf.c
Version:   0.0.6
Date:      17-OCT-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include "uemf.h"
#include "uemf_print.h"

/**
  \fn myEnhMetaFileProc(char *contents, unsigned int length, PEMF_WORKING_DATA lpData)
  \param contents binary contents of an EMF file
  \param length   length in bytes of contents
*/
int myEnhMetaFileProc(char *contents, size_t length)
{
    size_t   off=0;
    size_t   result;
    int      OK =1;
    int      recnum=0;
    PU_ENHMETARECORD pEmr;
    char     *blimit;
    
    blimit = contents + length;

    while(OK){
       if(off>=length){ //normally should exit from while after EMREOF sets OK to false, this is most likely a corrupt EMF
          printf("WARNING: record claims to extend beyond the end of the EMF file\n");
          return(0); 
       }

       pEmr = (PU_ENHMETARECORD)(contents + off);

       if(!recnum && (pEmr->iType != U_EMR_HEADER)){
          printf("WARNING: EMF file does not begin with an EMR_HEADER record\n");
       }
       
       result = U_emf_onerec_print(contents, blimit, recnum, off);
       if(result == (size_t) -1){
          printf("ABORTING on invalid record - corrupt file?\n");
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

    return 1;
}

int main(int argc, char *argv[]){
(void) argc;  /* quiet the unused parameter compiler warning */
size_t    length;
char     *contents=NULL;

   if(emf_readdata(argv[1],&contents,&length)){
      printf("reademf: fatal error: could not open or successfully read file:%s\n",argv[1]);
      exit(EXIT_FAILURE);
   }

   (void) myEnhMetaFileProc(contents,length);

   free(contents);

   exit(EXIT_SUCCESS);
}


