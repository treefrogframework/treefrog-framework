#!/usr/bin/env bash
set -o errexit
set -o pipefail
set -o nounset

export MONGOC_TEST_OIDC="ON"
export MONGOC_TEST_USER="$OIDC_ADMIN_USER"
export MONGOC_TEST_PASSWORD="$OIDC_ADMIN_PWD"
export MONGOC_AZURE_RESOURCE="$AZUREOIDC_RESOURCE"

# Install required OpenSSL runtime library.
sudo apt install -y libssl-dev

./cmake-build/src/libmongoc/test-libmongoc -d -l '/auth/unified/*' -l '/oidc/*'
