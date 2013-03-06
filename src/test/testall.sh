#!/bin/sh

LD_LIBRARY_PATH=..
export LD_LIBRARY_PATH

for e in `ls -d *`; do
  if [ -x "$e/$e" ]; then
    echo "-------------------------------------------------"
    echo "Testing $e/$e ..."
    if $e/$e; then
      echo "Test passed."
    else
      echo "Test failed.\n"
      exit 1
    fi
  fi
done
echo

