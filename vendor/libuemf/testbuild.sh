#!/bin/bash
# simple build script used for development.
# Builds applications directly, no libraries built.
# (linux)
COPTS="-Werror=format-security -Wall -Wformat -Wformat-security -W -Wno-pointer-sign -std=c99 -pedantic -Wall -g"
CLIBS=-lm
# (Sparc)
# COPTS="-Werror=format-security -Wall -Wformat -Wformat-security -W -Wno-pointer-sign -DSOL8 -DWORDS_BIGENDIAN -std=c99 -pedantic -Wall -g"
# CLIBS="-lm -L/opt/csw/lib -liconv"
# (win32) Mingw
# COPTS="-Werror=format-security -Wall -Wformat -Wformat-security -W -Wno-pointer-sign -DWIN32 -std=c99 -pedantic -Wall -g"
# CLIBS="-lm -liconv"
echo  cutemf            ; gcc $COPTS -o cutemf            cutemf.c            uemf.c uemf_endian.c uemf_utf.c        $CLIBS
echo  pmfdual2single    ; gcc $COPTS -o pmfdual2single    pmfdual2single.c    uemf.c uemf_endian.c uemf_utf.c upmf.c $CLIBS
echo  reademf           ; gcc $COPTS -o reademf           reademf.c           uemf.c uemf_endian.c uemf_safe.c uemf_print.c uemf_utf.c upmf.c upmf_print.c $CLIBS
echo  readwmf           ; gcc $COPTS -o readwmf           readwmf.c           uemf.c uemf_endian.c uemf_safe.c uemf_print.c uemf_utf.c upmf.c upmf_print.c uwmf.c uwmf_endian.c uwmf_print.c  $CLIBS 
echo  testbed_emf       ; gcc $COPTS -o testbed_emf       testbed_emf.c       uemf.c uemf_endian.c uemf_safe.c uemf_print.c uemf_utf.c upmf.c upmf_print.c $CLIBS
echo  testbed_pmf       ; gcc $COPTS -o testbed_pmf       testbed_pmf.c       uemf.c uemf_endian.c uemf_safe.c uemf_utf.c upmf.c upmf.h $CLIBS
echo  testbed_wmf       ; gcc $COPTS -o testbed_wmf       testbed_wmf.c       uemf.c uemf_endian.c uemf_safe.c uemf_print.c uemf_utf.c upmf.c upmf_print.c uwmf.c uwmf_endian.c $CLIBS
echo  test_mapmodes_emf ; gcc $COPTS -o test_mapmodes_emf test_mapmodes_emf.c uemf.c uemf_endian.c uemf_safe.c uemf_print.c uemf_utf.c upmf.c upmf_print.c $CLIBS
