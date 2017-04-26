#!/bin/bash

WORKDIR=$(cd $(dirname $0) && pwd)
#LD_LIBRARY_PATH=$WORKDIR/..
#export LD_LIBRARY_PATH

cd $WORKDIR

for e in `ls -d *`; do
  if [ -f "$e/Makefile" ]; then
    make -C $e clean
  fi
done

[ -f Makefile ] && make distclean

qmake -r
make -j8
if [ "$?" != 0 ]; then
  echo
  echo "build error!"
  exit 1
fi

for dir in `ls -d */`; do
  e=`basename $dir`
  if [ -x "$e/$e" ]; then
    echo "-------------------------------------------------"
    echo "Testing $e/$e ..."

    cd $e
    if ./$e; then
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
