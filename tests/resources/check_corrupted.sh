#!/bin/sh

cd `dirname $0`
cd filesmasher-master
make
cd ..

mkdir -p ../out
MAX=100

counter1=0
while [ $counter1 -lt $MAX ]
do
    tmp_emf=`mktemp -p ../out/`
    cp emf/`ls emf |shuf -n 1` ${tmp_emf} 
    counter2=0
    while [ $counter2 -lt $MAX ]
    do
        ../../emf2svg -p -w 800 -h 600 -i ${tmp_emf} -o ../out/test.svg -p
        ret=$?
        if [ $ret -eq 1 ]
        then
            counter2=$MAX
        elif [ $ret -gt 1 ]
        then
            ts=`date +"%F-%H%M%S"`
            printf "[ERROR] corrupted file 'bad_corrupted_${ts}.emf' caused something wrong\n"
            cp ${tmp_emf} ../out/bad_corrupted_${ts}.emf
            exit $ret
        fi
        filesmasher-master/filesmasher ${tmp_emf} 1 >/dev/null
        counter2=$(( $counter2 + 1 ))
    done
    rm "${tmp_emf}"
    counter1=$(( $counter1 + 1 ))
done
