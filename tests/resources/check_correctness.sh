#!/bin/sh

OUTDIR="../out"
EMFDIR="./emf"
ret=0


VERBOSE=1

if [ "$1" = "-v" ] || [ "$1" = "--verbose" ]
then
    VERBOSE=0
    VERBOSE_OPT='--verbose'
fi

verbose_print(){
    msg="$1"
    if [ $VERBOSE -eq 0 ]
    then
       printf "$msg\n"
    fi
}

cd `dirname $0`
rm -rf $OUTDIR
mkdir -p $OUTDIR
for emf in `find $EMFDIR -name "*.emf"`
do
    verbose_print "\n############## `basename ${emf}` ####################"
    ../../emf2svg -p -w 800 -h 600 -i $emf -o ${OUTDIR}/`basename ${emf}`.svg $VERBOSE_OPT
    verbose_print "Command: ../../emf2svg -p -i $emf -o ${OUTDIR}/`basename ${emf}`.svg"
    xmllint --dtdvalid ./svg11-flat.dtd  --noout ${OUTDIR}/`basename ${emf}`.svg
    valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes --error-exitcode=1 ../../emf2svg -p -w 800 -h 600 -i $emf -o ${OUTDIR}/`basename ${emf}`.svg $VERBOSE_OPT
    if [ $? -ne 0 ]
    then
        printf "[ERROR] emf2svg generate bad svg '${OUTDIR}/`basename ${emf}`.svg' from emf '$emf'\n"
        ret=1
    fi
    verbose_print "\n#####################################################\n"
done
if [ $ret -ne 0 ]
then
    printf "[FAIL] Check exited with error(s)\n"
else
    printf "[SUCCESS] Check Ok\n"
fi
exit $ret
