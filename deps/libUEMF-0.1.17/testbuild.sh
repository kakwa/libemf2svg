#!/bin/bash
# simple build script used for development
COPTS="-Werror=format-security -Wall -Wformat -Wformat-security -W -Wno-pointer-sign -std=c99 -pedantic -Wall -g"
echo  cutemf            ; gcc $COPTS -o cutemf            cutemf.c            uemf.c uemf_endian.c uemf_utf.c        -lm
echo  pmfdual2single    ; gcc $COPTS -o pmfdual2single    pmfdual2single.c    uemf.c uemf_endian.c uemf_utf.c upmf.c -lm
echo  reademf           ; gcc $COPTS -o reademf           reademf.c           uemf.c uemf_endian.c uemf_print.c uemf_utf.c upmf.c upmf_print.c -lm
echo  readwmf           ; gcc $COPTS -o readwmf           readwmf.c           uemf.c uemf_endian.c uemf_print.c uemf_utf.c upmf.c upmf_print.c uwmf.c uwmf_endian.c uwmf_print.c  -lm 
echo  testbed_emf       ; gcc $COPTS -o testbed_emf       testbed_emf.c       uemf.c uemf_endian.c uemf_print.c uemf_utf.c upmf.c upmf_print.c -lm
echo  testbed_pmf       ; gcc $COPTS -o testbed_pmf       testbed_pmf.c       uemf.c uemf_endian.c uemf_utf.c upmf.c upmf.h -lm
echo  testbed_wmf       ; gcc $COPTS -o testbed_wmf       testbed_wmf.c       uemf.c uemf_endian.c uemf_print.c uemf_utf.c upmf.c upmf_print.c uwmf.c uwmf_endian.c -lm
echo  test_mapmodes_emf ; gcc $COPTS -o test_mapmodes_emf test_mapmodes_emf.c uemf.c uemf_endian.c uemf_print.c uemf_utf.c upmf.c upmf_print.c -lm
