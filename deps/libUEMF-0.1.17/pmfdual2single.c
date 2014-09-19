/**  
 Utility program which may be used to delete specific records from an EMF file.  This is probably
 not very userful for most people, but is handy when debugging problem EMF files sent in from other
 people, which otherwise could not be easily modified.  The first record cannot be removed.  The last
 record, if it is an EOF, should not be removed.
 
 Run like:
    pmfdual2single -0 'rec1,rec2...,recN' src.emf dst.emf     neutralize all comments (EMF+ comments become normal comments)
    pmfdual2single -1 'rec1,rec2...,recN' src.emf dst.emf     remove all extraneous EMF records, clear dual-mode bit
    pmfdual2single -2 'rec1,rec2...,recN' src.emf dst.emf     remove all extraneous EMF records, retain dual-mode bit
 
 Build with:  gcc -std=c99 -pedantic -Wall -o pmfdual2single pmfdual2single.c uemf.c uemf_endian.c uemf_utf.c upmf.c -lm
*/

/*
File:      pmfdual2single.c
Version:   0.0.1
Date:      19-JUL-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "upmf.h" // includes "uemf.h"

/*
  Copy only those records that are needed for EMF+.
*/
char *cut_recs(EMFTRACK *et, char *contents, size_t *length, int ProcMode)
{
    size_t   off=0;
    int      OK =1;
    int      recnum=0;
    int      pmf_header_done=0;
    char    *pmf_contents;
    U_PMF_CMN_HDR Header;
    
    PU_ENHMETARECORD      pEmr;
    PU_EMRCOMMENT_EMFPLUS pPmrComment;
    

    while(OK){
       if(off>=*length){ //normally should exit from while after EMREOF sets OK to false, this is most likely a corrupt EMF
          printf("pmfdual2single: Fatal Error: record claims to extend beyond the end of the EMF file\n");
          exit(EXIT_FAILURE); 
       }

       pEmr = (PU_ENHMETARECORD)(contents + off);
       if(pEmr->iType == U_EMR_EOF){ OK=0; }

       if(!recnum && (pEmr->iType != U_EMR_HEADER)){
          printf("pmfdual2single: Fatal Error: EMF file does not begin with an EMR_HEADER record\n");
          exit(EXIT_FAILURE); 
       }
       
       /* pass through only these types: 
           EMF header, Comment, and EOF
           In theory the "dual-mode" bit should be cleared.  In practice nothing bad happens if there is only EMF+ content.
       */
       if( ProcMode){
          if( pEmr->iType == U_EMR_HEADER  || pEmr->iType == U_EMR_EOF){
              emf_append(pEmr,et,0);
          }
          else if(pEmr->iType == U_EMR_COMMENT ){
              pPmrComment = (PU_EMRCOMMENT_EMFPLUS) pEmr;
              /* find the EMF+ header record and clear the dual flag */
              if(pPmrComment->cbData >= 12){
                 if(!pmf_header_done){
                    if(pPmrComment->cIdent == U_EMR_COMMENT_EMFPLUSRECORD){ /* first EMF+ record better be the EMF+ header */
                       pmf_contents = (char *) &(pPmrComment->Data);
                       memcpy(&Header, pmf_contents, sizeof(U_PMF_CMN_HDR));
                       if((Header.Type & U_PMR_TYPE_MASK) != U_PMR_HEADER){
                          printf("pmfdual2single: fatal error: first EMF+ record is not an EMF+ header\n");
                          exit(EXIT_FAILURE);
                       }
                       pmf_header_done = 1;
                       /* clear all of the flag bits */
                       memset(pmf_contents + 2, 0, 2);
                    }
                 }
                 if(pPmrComment->cIdent == U_EMR_COMMENT_EMFPLUSRECORD){  /* only keep EMF+ comment records */
                    emf_append(pEmr,et,0);
                 }
              }

          }
       }
       else { /* !ProcMode, clear all EMF comment records */
         if(pEmr->iType == U_EMR_COMMENT){
            pPmrComment = (PU_EMRCOMMENT_EMFPLUS) pEmr;
            if(pPmrComment->cIdent == U_EMR_COMMENT_EMFPLUSRECORD){
               pPmrComment->cIdent = 1;  /* no long an emf+ comment, but nothing else changes */ 
            }
         }
         emf_append(pEmr,et,0);
       }
       off += pEmr->nSize;
       recnum++;  /* Cut positions are numbered from 0 */
    }  //end of while
    free(contents);

    return(NULL);
}

int main(int argc, char *argv[]){
    EMFTRACK     *et;
    EMFHANDLES   *eht;
    size_t        length;
    int           status;
    char         *contents=NULL;
    char         *infile;
    char         *outfile;
    int           ProcMode;
    int           oops=0;
    PU_EMRHEADER  EmrHeader;
    int           nHandles;
   
   if(argc != 4){ oops = 1; }
   else {
       if(0==strcmp(argv[1],"-0")){      ProcMode = 0; }
       else if(0==strcmp(argv[1],"-1")){ ProcMode = 1; }
       else if(0==strcmp(argv[1],"-2")){ ProcMode = 2; }
       else { oops = 1; }
   }
   if(oops){
      printf("pmfdual2single:  convert Dual-mode emf+ file to single mode, or remove EMF+ records\n\n");
      printf("   Usage:    pmfdual2single -N dual_mode.emf single_mode.emf\n");
      printf("      N    Action\n");
      printf("      0    Neutralize all EMF comments (all EMF+ records disappear)\n");
      printf("      1    Remove all extraneous EMF records, clear  EMF+ header dual-mode bit\n");
      printf("      2    Remove all extraneous EMF records, retain EMF+ header dual-mode bit\n");
      exit(EXIT_FAILURE);
   }
   infile  = argv[2];
   outfile = argv[3];

   if(emf_readdata(infile,&contents,&length)){
      printf("pmfdual2single: fatal error: could not open or successfully read file:%s\n",infile);
      exit(EXIT_FAILURE);
   }
   
   /* grab the number of handles from the header.  This information is needed later and the cut routine
   is not sophisticated enough to figure out the value on the fly. */
   EmrHeader = (PU_EMRHEADER) contents;
   nHandles = EmrHeader->nHandles;
   
   status=emf_start(outfile,length, 4096, &et); // space allocation initial, increment will never be used
   if(status){
      printf("pmfdual2single: fatal error: could not open output file:%s\n",outfile);
      exit(EXIT_FAILURE);
   }
   status=htable_create(128, 128, &eht);

   contents = cut_recs(et, contents,&length, ProcMode); /* copy records and free contents */
   
   eht->peak = nHandles - 1; /* poke the number of handles into the expected place so that emf_finish will do the right thing */
   status=emf_finish(et, eht);
   if(status){
      printf("pmfdual2single: fatal error: emf_finish failed with status: %d\n", status);
      exit(EXIT_FAILURE);
   }

   emf_free(&et);
   htable_free(&eht);

   exit(EXIT_SUCCESS);
}


