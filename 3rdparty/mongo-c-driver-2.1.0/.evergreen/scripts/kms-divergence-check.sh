#!/usr/bin/env bash

# kms-divergence-check.sh checks that the vendored copy of kms-message does not
# have additional changes not present in https://github.com/mongodb/libmongocrypt.

set -o errexit

# shellcheck source=.evergreen/scripts/use-tools.sh
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths

# `paths` defines `MONGOC_DIR`.
LIBMONGOCRYPT_DIR="$MONGOC_DIR/libmongocrypt-for-kms-divergence-check"

# LIBMONGOCRYPT_GITREF is expected to refer to the version of libmongocrypt
# where kms-message was last copied.
LIBMONGOCRYPT_GITREF="34a9572c416e0827a1fa988baf88411c4b5f2c7b"

cleanup() {
    if [ -d "$LIBMONGOCRYPT_DIR" ]; then
        rm -rf "$LIBMONGOCRYPT_DIR"
    fi
}

cleanup
trap cleanup EXIT

git clone -q https://github.com/mongodb/libmongocrypt "$LIBMONGOCRYPT_DIR"
cd "$LIBMONGOCRYPT_DIR"
git checkout "$LIBMONGOCRYPT_GITREF" --quiet
if ! diff -uNr "$LIBMONGOCRYPT_DIR/kms-message/" "$MONGOC_DIR/src/kms-message/" ; then
    echo "Unexpected differences found in KMS sources!"
    exit 1
else
    echo "No changes detected from KMS message at commit $LIBMONGOCRYPT_GITREF"
fi
