#!/bin/bash
# (The preceding line may need to be modified.  On Solaris it is probably /usr/bin/bash)
#
# Test all programs against reference.  If all is working correctly
# each output line should end with "are identical".
#
# Edit the following 3 lines as needed.
#    gdiff is gnu diff
#    extract & execinput are from http://sourceforge.net/projects/drmtools/
USEDIFF=/usr/bin/diff
EXTRACT=/usr/local/bin/extract
EXECINPUT=/usr/local/bin/execinput
#
./testbed_emf 4 >/dev/null
mv test_libuemf.emf test_libuemf30.emf
./testbed_emf 0 >/dev/null
./reademf test_libuemf.emf >test_libuemf_emf.txt
./reademf test_libuemf30.emf >test_libuemf30_emf.txt
./testbed_wmf 0  >/dev/null
./readwmf test_libuemf.wmf >test_libuemf_wmf.txt 
./test_mapmodes_emf -vX 2000 -vY 1000 >/dev/null
./testbed_pmf 0 >/dev/null
./reademf test_libuemf_p.emf >test_libuemf_p_emf.txt
ls -1 test*ref* | \
  $EXTRACT -fmt " $USEDIFF -s [1,] [rtds_ref:1,]" | \
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
