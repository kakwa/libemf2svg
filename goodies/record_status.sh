#!/bin/sh

help(){
  echo "usage: `basename $0` -i <emf> -s <PART|SUP|IGN|UN>"
  echo ""
  echo "show record support status of an emf file"
  echo ""
  echo "ex:"
  echo "  `basename $0` -i tests/resources/emf/test-150.emf -s SUP"
  exit 1
}

if [ "`uname`" = "Darwin" ]
then
    RL=greadlink
else
    RL=readlink
fi

while getopts ":hi:s:" opt; do
  case $opt in

    h) help
        ;;
    s)
        STATUS="$OPTARG"
        ;;
    i)
        EMF="`$RL -f $OPTARG`"
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

cd `dirname $0`
../emf2svg-conv -i "${EMF}" -o /dev/null -v |grep -B 1 "${STATUS}" | grep '^U' | sed 's/\ .*//' |sort -u
