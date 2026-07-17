#!/usr/bin/env bash

set -o errexit
set -o pipefail

declare working_dir
working_dir="$(pwd)"

export PATH
PATH="${working_dir:?}/install/bin:${PATH:-}"

# Install prefix to use for ABI compatibility scripts.
if [[ ! -d "${working_dir}/install" ]]; then
  echo "Creating install directory at ${working_dir}/install"
  mkdir -p "${working_dir}/install"
fi

declare parallel_level
parallel_level="$(("$(nproc)" + 1))"

export PATH
PATH="${MONGO_C_DRIVER_CACHE_DIR:?}/bin:${PATH:-}" # abi-compliance-checker

# Obtain abi-compliance-checker.
echo "Fetching abi-compliance-checker..."
[[ -d "${MONGO_C_DRIVER_CACHE_DIR:?}/checker-2.3" ]] || {
  git clone -b "2.3" --depth 1 https://github.com/lvc/abi-compliance-checker.git "${MONGO_C_DRIVER_CACHE_DIR:?}/checker-2.3"
  pushd "${MONGO_C_DRIVER_CACHE_DIR:?}/checker-2.3"
  make -j "${parallel_level:?}" --no-print-directory install prefix="${MONGO_C_DRIVER_CACHE_DIR:?}"
  popd # "${MONGO_C_DRIVER_CACHE_DIR:?}/checker-2.3"
} >/dev/null
echo "Fetching abi-compliance-checker... done."

# Obtain ctags.
echo "Fetching ctags..."
[[ -d "${MONGO_C_DRIVER_CACHE_DIR:?}/ctags-6.0.0" ]] || {
  git clone -b "v6.0.0" --depth 1 https://github.com/universal-ctags/ctags.git "${MONGO_C_DRIVER_CACHE_DIR:?}/ctags-6.0.0"
  pushd "${MONGO_C_DRIVER_CACHE_DIR:?}/ctags-6.0.0"
  ./autogen.sh
  ./configure --prefix="${MONGO_C_DRIVER_CACHE_DIR:?}"
  make -j "${parallel_level:?}"
  make install
  popd # "${MONGO_C_DRIVER_CACHE_DIR:?}/ctags-6.0.0"
} >/dev/null
echo "Fetching ctags... done."

command -V abi-compliance-checker
