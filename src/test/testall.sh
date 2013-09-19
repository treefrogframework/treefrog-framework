#!/bin/bash

LD_LIBRARY_PATH=..
export LD_LIBRARY_PATH

for e in `ls -d *`; do
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
