#!/bin/sh

cd `dirname $0`
cd filesmasher-master
make
cd ..

mkdir -p ../out

# Once a file is seen as corrupted by emf2svg-conv
# we alter if $MAX more times to try to make
# emf2svg-conv crash
MAX=100

burn_in_hell(){
    counter1=0
    while [ $counter1 -lt $MAX ]
    do
        tmp_emf=`mktemp -p ../out/`
        cp emf/`ls emf |shuf -n 1` ${tmp_emf} 
        counter2=0
        while [ $counter2 -lt $MAX ]
        do
            ../../emf2svg-conv -p -w 800 -h 600 -i ${tmp_emf} -o ../out/test.svg -p
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
}

if [ "$1" = "inf" ]
then
    while true
    do
        burn_in_hell
    done
else
    burn_in_hell
fi
