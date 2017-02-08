/**  
 Example program demonstrating how to code a program to read an WMF file.  This example just examines the contents
 of one WMF file and prints the contents record by record, showing all fields (except for bitmaps).  

 Run like:
    readwmf filename.wmf
 
 Writes description of the WMF file to stdout.

 Build with:  gcc -Wall -o readwmf readwmf.c uemf.c uwmf.c uemf_print.c uemf_endian.c uemf_utf.c uwmf_print.c uwmf_endian.c -lm
*/

/*
File:      readwmf.c
Version:   0.0.5
Date:      17-OCT-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2012 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() */
#include "uwmf.h"
#include "uwmf_print.h"

/**
  \fn myMetaFileProc(char *contents, unsigned int length, PWMF_WORKING_DATA lpData)
  \returns 0 on normal exit, -1 on error
  \param contents binary contents of an WMF file
  \param length   length in bytes of contents
*/
int myMetaFileProc(char *contents, size_t length)
{
    size_t   off=0;
    size_t   result;
    int      OK =1;
    int      recnum=0;
    char    *blimit = contents + length;

    off = wmfheader_print(contents, blimit);
    while(OK){

       result = U_wmf_onerec_print(contents, blimit, recnum, off);
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

    return(result);
}

int main(int argc, char *argv[]){
(void) argc;  /* quiet the unused parameter compiler warning */
size_t    length;
char     *contents=NULL;

   if(wmf_readdata(argv[1],&contents,&length)){
      printf("readwmf: fatal error: could not open or successfully read file:%s\n",argv[1]);
      exit(EXIT_FAILURE);
   }

   (void) myMetaFileProc(contents,length);

   free(contents);

   exit(EXIT_SUCCESS);
}


