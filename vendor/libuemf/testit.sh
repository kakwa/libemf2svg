#!/bin/bash
# (The preceding line may need to be modified.  On Solaris it is probably /usr/bin/bash)
#
# Test all programs against reference.  If all is working correctly
# each output line should end with "are identical".
#
# NOTE! On windows text files may differ by having \n\r instead of \n, which is
# why there is a "b" in the diff.
#
# Edit the following 4 lines as needed.
#    diff is gnu diff
#    extract & execinput are from http://sourceforge.net/projects/drmtools/
#    EPATH is the path to the binaries built by this package
#EPATH=/usr/local/bin
EPATH=${PWD}/bin
#EPATH=.
USEDIFF=`which diff`
EXTRACT=/usr/local/bin/extract
EXECINPUT=/usr/local/bin/execinput
export LD_LIBRARY_PATH=${PWD}/lib

PROBLEMS=0
if [ ! -x $EPATH/testbed_emf ]
then
   echo "testit.sh: set EPATH in script, testbed_emf not found!"
   PROBLEMS=1
fi
if [ ! -x $EXTRACT ]
then
   echo "testit.sh: drm_tools 'extract' program not found. "
   PROBLEMS=1
fi
if [ ! -x $EXECINPUT ]
then
   echo "testit.sh: drm_tools 'extract' program not found. "
   PROBLEMS=1
fi
if [ $PROBLEMS -eq 1 ]
then
   exit
fi
#
$EPATH/testbed_emf 4 >/dev/null
mv test_libuemf.emf test_libuemf30.emf
$EPATH/testbed_emf 0 >/dev/null
$EPATH/reademf test_libuemf.emf >test_libuemf_emf.txt
$EPATH/reademf test_libuemf30.emf >test_libuemf30_emf.txt
$EPATH/testbed_wmf 0  >/dev/null
$EPATH/readwmf test_libuemf.wmf >test_libuemf_wmf.txt 
$EPATH/test_mapmodes_emf -vX 2000 -vY 1000 >/dev/null
$EPATH/testbed_pmf 0 >/dev/null
$EPATH/reademf test_libuemf_p.emf >test_libuemf_p_emf.txt
ls -1 test*ref* | \
  $EXTRACT -fmt " $USEDIFF -bqs [1,] [rtds_ref:1,]" | \
  $EXECINPUT
#
# clean up
#
rm -f test_libuemf30.emf
rm -f test_libuemf.emf
rm -f test_libuemf_p.emf
rm -f test_libuemf.wmf
rm -f test_libuemf30_emf.txt
rm -f test_libuemf_emf.txt
rm -f test_libuemf_p_emf.txt
rm -f test_libuemf_wmf.txt
rm -f test_mm_anisotropic.emf
rm -f test_mm_hienglish.emf
rm -f test_mm_himetric.emf
rm -f test_mm_isotropic.emf
rm -f test_mm_loenglish.emf
rm -f test_mm_lometric.emf
rm -f test_mm_text.emf
rm -f test_mm_twips.emf
