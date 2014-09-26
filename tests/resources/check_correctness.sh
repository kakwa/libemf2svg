#!/bin/sh

OUTDIR="./svgs"
EMFDIR="./emf"
ret=0

cd `dirname $0`
rm -rf $OUTDIR
mkdir -p $OUTDIR
for emf in `find $EMFDIR -name "*.emf"`
do
    ../../emf2svg -p -i $emf -o ${OUTDIR}/`basename ${emf}`.svg
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
