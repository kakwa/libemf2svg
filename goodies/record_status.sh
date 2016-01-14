#!/bin/sh

help(){
  echo "usage: `basename $0` <args>"
  echo ""
  echo "<description>"
  echo "arguments:"
  echo "  <options>"
  exit 1
}

while getopts ":hi:s:" opt; do
  case $opt in

    h) help
        ;;
    s)
        STATUS="$OPTARG"
        ;;
    i)
        EMF="`readlink -f $OPTARG`"
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
