#!/usr/bin/env bash
set -o errexit
set -o pipefail
set -o nounset

export MONGOC_TEST_OIDC="ON"
export MONGOC_TEST_USER="$OIDC_ADMIN_USER"
export MONGOC_TEST_PASSWORD="$OIDC_ADMIN_PWD"
export MONGOC_TEST_URI="$MONGODB_URI"
export MONGOC_TEST_SSL="ON"
export MONGOC_TEST_OIDC_K8S="ON"

# Install required OpenSSL runtime library.
sudo apt install -y libssl-dev

./cmake-build/src/libmongoc/test-libmongoc -d --no-fork -l '/auth/unified/*' -l '/oidc/*'
