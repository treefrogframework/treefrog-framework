#!/usr/bin/env bash

set -o errexit
set -o pipefail

# create all needed directories
mkdir abi-compliance
mkdir abi-compliance/current-install
mkdir abi-compliance/base-install
mkdir abi-compliance/dumps

declare head_commit today
# The 10 digits of the base commit
head_commit=$(git rev-parse --revs-only --short=10 "HEAD^{commit}")
# The YYYYMMDD date
today=$(date +%Y%m%d)

declare current base
current="$(cat VERSION_CURRENT)-${today:?}+git${head_commit:?}" # e.g. 2.3.4-dev
base=$(cat etc/prior_version.txt)                               # e.g. 1.2.3

current_verdir="$(echo "${current:?}" | perl -lne 'm|^(\d+\.\d+\.\d+).*$|; print $1')" # Strip any suffixes.
base_verdir="${base:?}"

# Double-check we are testing against the same API major version.
if [[ "${base_verdir:?}" != 2.* ]]; then
  echo "API major version mismatch: base version is ${base:?} but current version is ${current:?}" >&2
  exit 1
fi

declare working_dir
working_dir="$(pwd)"

export PATH
PATH="${MONGO_C_DRIVER_CACHE_DIR:?}/bin:${PATH:-}" # abi-compliance-checker

cmake_configure_flags=(
  "-DENABLE_STATIC=OFF"
  "-DENABLE_TESTS=OFF"
  "-DENABLE_EXAMPLES=OFF"
)

# build the current changes
env \
  CFLAGS="-g -Og" \
  EXTRA_CONFIGURE_FLAGS="-DCMAKE_INSTALL_PREFIX=./abi-compliance/current-install ${cmake_configure_flags[*]:?}" \
  .evergreen/scripts/compile.sh

# checkout the base release
git checkout "tags/${base:?}" -f

declare compile_script
compile_script=".evergreen/scripts/compile.sh"

# build the base release
env \
  CFLAGS="-g -Og" \
  EXTRA_CONFIGURE_FLAGS="-DCMAKE_INSTALL_PREFIX=./abi-compliance/base-install ${cmake_configure_flags[*]:?}" \
  bash "${compile_script}"

# check for abi compliance. Generates HTML Reports.
cd abi-compliance

cat >|old.xml <<DOC
<version>
  ${base:?}
</version>

<libs>
  $(pwd)/base-install/lib
</libs>

<add_include_paths>
  $(pwd)/base-install/include/bson-${base_verdir:?}/
  $(pwd)/base-install/include/mongoc-${base_verdir:?}/
</add_include_paths>

<headers>
  $(pwd)/base-install/include/bson-${base_verdir:?}/bson/bson.h
  $(pwd)/base-install/include/mongoc-${base_verdir:?}/mongoc/mongoc.h
</headers>
DOC

cat >|new.xml <<DOC
<version>
  ${current:?}
</version>

<libs>
  $(pwd)/current-install/lib
</libs>

<add_include_paths>
  $(pwd)/current-install/include/bson-${current_verdir:?}/
  $(pwd)/current-install/include/mongoc-${current_verdir:?}/
</add_include_paths>

<headers>
  $(pwd)/current-install/include/bson-${current_verdir:?}/bson/bson.h
  $(pwd)/current-install/include/mongoc-${current_verdir:?}/mongoc/mongoc.h
</headers>
DOC

# Allow task to upload the HTML report despite failed status.
if ! abi-compliance-checker -lib mongo-c-driver -old old.xml -new new.xml; then
  find . -name log.txt -exec cat {} + >&2 || true
  declare status
  status='{"status":"failed", "type":"test", "should_continue":true, "desc":"abi-compliance-checker emitted one or more errors"}'
  curl -sS -d "${status:?}" -H "Content-Type: application/json" -X POST localhost:2285/task_status || true
fi
