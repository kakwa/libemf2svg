#!/bin/sh


cd `dirname $0`
. ./colors.sh

percent(){
    printf "$(( $1 * 100 / $2  ))"
}

EMFC="../../src/lib/emf2svg.c"
PMFC="../../src/lib/pmf2svg_print.c"
count_emf_SUPPORTED=`grep -c FLAG_SUPPORTED $EMFC`
count_emf_IGNORED=`grep -c FLAG_IGNORED $EMFC`
count_emf_UNUSED=`grep -c FLAG_UNUSED $EMFC`
count_emf_PARTIAL=`grep -c FLAG_PARTIAL $EMFC`
total_emf=$(( $count_emf_SUPPORTED + $count_emf_IGNORED + $count_emf_UNUSED + $count_emf_PARTIAL  ))
printf "EMF RECORDS:\n"
printf "* ${BGre}Supported: ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_SUPPORTED" "`percent $count_emf_SUPPORTED $total_emf`"
printf "* ${BYel}Partial:   ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_PARTIAL"   "`percent $count_emf_PARTIAL   $total_emf`"
printf "* ${BBlu}Unused:    ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_UNUSED"    "`percent $count_emf_UNUSED    $total_emf`"
printf "* ${BRed}Ignored:   ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_IGNORED"   "`percent $count_emf_IGNORED   $total_emf`"
printf "* Total:    % 3d\n" $total_emf


printf "\n"

count_emf_SUPPORTED=`grep -c FLAG_SUPPORTED $PMFC`
count_emf_IGNORED=`grep -c FLAG_IGNORED     $PMFC`
count_emf_UNUSED=`grep -c FLAG_UNUSED       $PMFC`
count_emf_PARTIAL=`grep -c FLAG_PARTIAL     $PMFC`
total_emf=$(( $count_emf_SUPPORTED + $count_emf_IGNORED + $count_emf_UNUSED + $count_emf_PARTIAL  ))
printf "EMF+ RECORDS:\n"
printf "* ${BGre}Supported: ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_SUPPORTED" "`percent $count_emf_SUPPORTED $total_emf`"
printf "* ${BYel}Partial:   ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_PARTIAL"   "`percent $count_emf_PARTIAL   $total_emf`"
printf "* ${BBlu}Unused:    ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_UNUSED"    "`percent $count_emf_UNUSED    $total_emf`"
printf "* ${BRed}Ignored:   ${RCol}% 3d [${BCya}% 3d%%${RCol}]\n"  "$count_emf_IGNORED"   "`percent $count_emf_IGNORED   $total_emf`"
printf "* Total:     % 3d\n" $total_emf


