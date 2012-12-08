#!/bin/sh

BASEDIR=`dirname $0`
cd $BASEDIR/drivers

CP="cp -p"
if which install >/dev/null 2>&1; then
  CP="install -m 644 -p"
fi

if ! which tspawn >/dev/null 2>&1; then
  echo "tspawn: command not found"
  exit 1
fi

DRIVERPATH=`tspawn --show-driver-path 2>/dev/null`
if [ -d "$DRIVERPATH" ]; then
  for f in `ls *.dylib`; do
    $CP $f $DRIVERPATH
    echo "  installed $f"
  done
else
  echo "error: plugins directory not found."
  exit 1
fi
