#!/usr/bin/env bash

if [[ "${OSTYPE}" == "cygwin" ]]; then
  .evergreen/scripts/compile-windows.sh
else
  .evergreen/scripts/compile-unix.sh
fi
