#!/bin/sh

OUTDIR="./svgs"
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
       fprintf "$msg\n"
    fi
}

cd `dirname $0`
rm -rf $OUTDIR
mkdir -p $OUTDIR
for emf in `find $EMFDIR -name "*.emf"`
do
    ../../emf2svg -p -i $emf -o ${OUTDIR}/`basename ${emf}`.svg $VERBOSE_OPT
    verbose_print "Commande: ../../emf2svg -p -i $emf -o ${OUTDIR}/`basename ${emf}`.svg"
    xmllint --dtdvalid ./svg11-flat.dtd  --noout ${OUTDIR}/`basename ${emf}`.svg
    if [ $? -ne 0 ]
    then
        printf "[ERROR] emf2svg generate bad svg '${OUTDIR}/`basename ${emf}`.svg' from emf '$emf'\n"
        ret=1
    fi
done
if [ $ret -ne 0 ]
then
    printf "[FAIL] Check exited with error(s)\n"
else
    printf "[SUCCESS] Check Ok\n"
fi
exit $ret
