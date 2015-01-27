#!/bin/sh

OUTDIR="../out"
EMFDIR="./emf"
ret=0
VAGRIND_CMD="valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes --error-exitcode=42"

VERBOSE=1
ABSPATH=$( readlink -f "$(dirname $0)"  )
STOPONERROR="no"

help(){
    cat <<EOF
usage: `basename $0` [-h] [-v] [-e <emf dir>] [-s] [-n] 

Script checking memleaks, segfault and svg correctness of emf2svg

arguments:
  -h: diplays this help
  -v: verbose mode, print emf records
  -e: alternate emf dir (default '`readlink -f "$ABSPATH/$EMFDIR"`')
  -s: stop on first error
  -x: disable xmllint check (svg integrity)
  -n: disable valgrind (memleaks checks)
  -N: ignore return code of emf2svg (useful for checks on corrupted files)
EOF
    exit 1
}


while getopts ":hnNxvse:" opt; do
  case $opt in

    h) 
        help
        ;;
    n)
        VAGRIND_CMD=""
        ;;
    v)
        VERBOSE=0
        VERBOSE_OPT='--verbose'
        ;;
    e)
        EMFDIR=`readlink -f "$OPTARG" |sed "s%$ABSPATH%.%"`
        ;;
    s)
        STOPONERROR="yes"
        ;;
    x)
        XMLLINT="no"
        ;;
    N)
        NOEMF2SVGERR="yes"
        ;;
    \?)
        echo "Invalid option: -$OPTARG" >&2
        help
        exit 1
        ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
        help
        exit 1
        ;;
  esac
done

verbose_print(){
    msg="$1"
    if [ $VERBOSE -eq 0 ]
    then
       printf "$msg\n"
    fi
}

cd $ABSPATH
rm -rf $OUTDIR
mkdir -p $OUTDIR
for emf in `find $EMFDIR -type f -name "*.emf" |sort`
do
    verbose_print "\n############## `basename "${emf}"` ####################"
    verbose_print "Command: ../../emf2svg -p -i \"$emf\" -o ${OUTDIR}/`basename ${emf}`.svg"
    $VAGRIND_CMD ../../emf2svg -p -w 800 -h 600 -i "$emf" -o ${OUTDIR}/`basename "${emf}"`.svg $VERBOSE_OPT
    tmpret=$?
    if [ $tmpret -ne 0 ]
    then
        printf "[ERROR] emf2svg exited on error or memleaked or crashed converting emf '$emf'\n"
        [ $tmpret -eq 42 ] || [ $tmpret -eq 139 ] || ! [ "$NOEMF2SVGERR" = "yes" ] && ret=1
    fi
    if ! [ "$XMLLINT" = "no" ]
    then
        xmllint --dtdvalid ./svg11-flat.dtd  --noout ${OUTDIR}/`basename "${emf}"`.svg
        if [ $? -ne 0 ]
        then
            printf "[ERROR] emf2svg generate bad svg '${OUTDIR}/`basename "${emf}"`.svg' from emf '\"$emf\"'\n"
            ret=1
        fi
    fi
    verbose_print "\n#####################################################\n"
    [ "${STOPONERROR}" = "yes" ] && [ $ret -eq 1 ] && exit 1
done
if [ $ret -ne 0 ]
then
    printf "[FAIL] Check exited with error(s)\n"
else
    printf "[SUCCESS] Check Ok\n"
fi
exit $ret
