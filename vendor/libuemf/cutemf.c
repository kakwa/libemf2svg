/**  
 Utility program which may be used to delete specific records from an EMF file.  This is probably
 not very userful for most people, but is handy when debugging problem EMF files sent in from other
 people, which otherwise could not be easily modified.  The first record cannot be removed.  The last
 record, if it is an EOF, should not be removed.
 
 Run like:
    cutemf 'rec1,rec2...,recN' src.emf dst.emf
 
 Build with:  gcc -Wall -o cutemf cutemf.c uemf.c uemf_endian.c uemf_utf.c upmf.c -lm

0.0.13  11-OCT-2019.  Encountered EMF files with >10k records.  
   Changed so that it can operate on up to 10M records and handled more than that better.
*/

/*
File:      cutemf.c
Version:   0.0.13
Date:      11-OCT-2019
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2019 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "upmf.h" // includes "uemf.h"
#define MAXREC 10000000

/*
  cut_recs  Copy only those records NOT in the cuts list.  Returns NULL or exits.
*/
char *cut_recs(EMFTRACK *et, EMFHANDLES *eht, char *contents, size_t *length, int *cuts, int cutN)
{
    size_t   off=0;
    int      OK =1;
    int      recnum=0;
    int      icuts=0;
    PU_ENHMETARECORD pEmr;
    

    while(OK){
       if(off>=*length){ //normally should exit from while after EMREOF sets OK to false, this is most likely a corrupt EMF
          printf("cutemf: Fatal Error: record claims to extend beyond the end of the EMF file\n");
          exit(EXIT_FAILURE); 
       }

       pEmr = (PU_ENHMETARECORD)(contents + off);
       if(pEmr->iType == U_EMR_EOF){ OK=0; }
       
       if(recnum==0){
           U_EMRHEADER *record;
           if(pEmr->iType != U_EMR_HEADER){
              printf("cutemf: Fatal Error: EMF file does not begin with an EMR_HEADER record\n");
              exit(EXIT_FAILURE); 
           }
           record = (U_EMRHEADER *)pEmr;
           eht->peak = record->nHandles -1;  /* emf_finish needs this later */
       }
       
       if(icuts>=cutN || (recnum != cuts[icuts])){
          emf_append(pEmr,et,0);
       }
       else {
          if(!OK){
             printf("cutemf: Fatal Error: The final EMR_EOF record may not be removed\n");
             exit(EXIT_FAILURE); 
          }
          while(icuts<cutN && (recnum == cuts[icuts])){ icuts++; } /* increment, removing any duplicates */
       }

       off += pEmr->nSize;
       recnum++;  /* Cut positions are numbered from 0 */
    }  //end of while
    free(contents);

    return(NULL);
}

static int
cmpint(const void *p1, const void *p2)
{
    int result;
    if(     *(int *)p1 < *(int *)p2){ result = -1; }
    else if(*(int *)p1 > *(int *)p2){ result =  1; }
    else {                            result =  0; }
    return(result);
}

void get_ints(const char *estring, int *array, int *cutN){
   char *token;
   char *string = malloc(1 + strlen(estring));
   int   count=0;
   strcpy(string,estring);
   token = strtok(string," \t:,");
   while(token){
      int slen = strlen(token);
      int dash = strcspn(token,"-");
      int i;
      if(dash < slen){
         token[dash]='\0';
         int start = atoi(token);
         int stop  = atoi(token+dash+1);
         if(start>stop){
            printf("cutemf: fatal error: range A-B has start:%d and stop:%d\n",start,stop);
            exit(EXIT_FAILURE);
         }
         if(stop >= MAXREC){
            printf("cutemf: fatal error: only up to 10M records may be processed.  Modify code.\n");
            exit(EXIT_FAILURE);
         }
         for(i=start;i<=stop;i++){
            array[count++]=i;
         }
      }
      else {
         if(count >= MAXREC){
            printf("cutemf: fatal error: only up to 10M records may be processed.  Modify code.\n");
            exit(EXIT_FAILURE);
         }
         array[count++]=atoi(token);
      }
      token = strtok(NULL," \t:,");
   }
   qsort(array, count, sizeof(int), cmpint);
   *cutN=count;
   free(string);
}

int main(int argc, char *argv[]){
    EMFTRACK            *et;
    EMFHANDLES          *eht;
    size_t               length;
    int                  status;
    char                *contents=NULL;
    int                  *cutem;
    int                  cutN=0;

   if(argc != 4){
      printf("cutemf:  remove specific records from an EMF file.\n\n");
      printf("   Usage:    cutemf 'rec1,rec2,recA-recB,...recN' src.emf dst.emf\n");
      printf("   Example:  cutemf '2,6,9,13-20' in.emf out.emf\n\n");
      printf("   Record numbering starts at 0.\n");
      printf("   Record numbers are separated by spaces, commas, colons, or tabs.\n");
      printf("   Record ranges may be specified with A-B (no spaces).\n");
      printf("   Record 0 may not be removed.\n");
      printf("   When the last record is an EMR_EOF, it may not be removed.\n");
      printf("   A maximum of 10000 records may be removed at a time.\n");
      exit(EXIT_FAILURE);
   }
   if(emf_readdata(argv[2],&contents,&length)){
      printf("cutemf: fatal error: could not open or successfully read file:%s\n",argv[2]);
      exit(EXIT_FAILURE);
   }
   
   cutem = calloc(sizeof(int),MAXREC);
   if(!cutem){
      printf("cutemf: fatal error: could not allocate memory\n");
      exit(EXIT_FAILURE);
   }

   status=emf_start(argv[3],length, 4096, &et); // space allocation initial, increment will never be used
   if(status){
      printf("cutemf: fatal error: in emf_start\n");
      exit(EXIT_FAILURE);
   }
   status=htable_create(128, 128, &eht);
   if(status){
      printf("cutemf: fatal error: in htable_create\n");
      exit(EXIT_FAILURE);
   }

   get_ints(argv[1],cutem,&cutN);
   if(!cutN || cutem[0] <=1){
      printf("cutemf: fatal error: record list empty, invalid, or includes first record\n");
      exit(EXIT_FAILURE);
   }

   contents = cut_recs(et, eht, contents,&length,cutem,cutN); /* copy records and free contents */
   
   status=emf_finish(et, eht);
   if(status){
      printf("cutemf: fatal error: emf_finish failed with status: %d\n", status);
      exit(EXIT_FAILURE);
   }

   emf_free(&et);
   htable_free(&eht);
   free(cutem);

   exit(EXIT_SUCCESS);
}


