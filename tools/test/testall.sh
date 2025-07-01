#!/bin/bash

WORKDIR=$(cd $(dirname $0) && pwd)
#LD_LIBRARY_PATH=$WORKDIR/..
#export LD_LIBRARY_PATH

cd $WORKDIR

for d in `ls -d *`; do
  if [ -f "$d/Makefile" ]; then
    make -k -C $d clean
  fi
done

[ -f Makefile ] && make -k distclean
rm -f Makefile

for dir in `ls -d */`; do
  d=`basename $dir`
  if [ -x "$d/$d" ]; then
    echo "-------------------------------------------------"
    echo "Testing $d/$d ..."

    cd $d
    if ./$d; then
      echo "Test passed."
    else
      echo "Test failed!!!"
      echo
      exit 1
    fi
    cd ..
  fi
done

echo
echo "All tests passed. Congratulations!"
