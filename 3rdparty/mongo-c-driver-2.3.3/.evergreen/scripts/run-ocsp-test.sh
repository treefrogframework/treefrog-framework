#!/usr/bin/env bash

# Test runner for OCSP revocation checking.
#
# Closely models the tests described in the specification:
# https://github.com/mongodb/specifications/tree/master/source/ocsp-support/tests#integration-tests-permutations-to-be-tested.
# Based on the test case, this may start a mock responder process.
# Preconditions:
# - A mock responder configured for the test case is running (use run-ocsp-responder.sh – before running mongod).
# - mongod is running with the correct configuration. (use integration-tests.sh or spawn one manually).
#
# Environment variables:
#
# TEST_COLUMN
#   Required. Corresponds to a column of the test matrix. Set to one of the following:
#   TEST_1, TEST_2, TEST_3, TEST_4, SOFT_FAIL_TEST, MALICIOUS_SERVER_TEST_1, MALICIOUS_SERVER_TEST_2
# CERT_TYPE
#   Required. Set to either rsa or ecdsa.
# MONGODB_PORT
#   Optional. A custom port to connect to. Defaults to 27017.
#
# Example:
# TEST_COLUMN=TEST_1 CERT_TYPE=rsa ./run-ocsp-test.sh
#

set -o errexit
set -o pipefail

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths

check_var_req TEST_COLUMN
check_var_req CERT_TYPE

check_var_opt MONGODB_PORT "27017"

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]}")")"

declare mongoc_dir
mongoc_dir="$(to_absolute "${script_dir:?}/../..")"

declare mongoc_build_dir="${mongoc_dir:?}/cmake-build"
declare mongoc_install_dir="${mongoc_dir:?}/install-dir"
declare openssl_install_dir="${mongoc_dir:?}/openssl-install-dir"

declare responder_required
case "${TEST_COLUMN:?}" in
TEST_1) responder_required="valid" ;;
TEST_2) responder_required="invalid" ;;
TEST_3) responder_required="valid" ;;
TEST_4) responder_required="invalid" ;;
MALICIOUS_SERVER_TEST_1) responder_required="invalid" ;;
esac
: "${responder_required:-}"

on_exit() {
  echo "Cleaning up"
  if [[ -n "${responder_required:-}" ]]; then
    echo "Responder logs:"
    cat "${mongoc_dir:?}/responder.log"
  fi
}
trap on_exit EXIT

declare mongoc_ping="${mongoc_build_dir:?}/src/libmongoc/mongoc-ping"

# Add libmongoc-1.0 and libbson-1.0 to library path, so mongoc-ping can find them at runtime.
if [[ "${OSTYPE}" == "cygwin" ]]; then
  export PATH
  PATH+=":${mongoc_build_dir:?}/src/libmongoc/Debug"
  PATH+=":${mongoc_build_dir:?}/src/libbson/Debug"

  chmod -f +x src/libmongoc/Debug/* src/libbson/Debug/* || true

  mongoc_ping="${mongoc_build_dir:?}/src/libmongoc/Debug/mongoc-ping.exe"
elif [[ "${OSTYPE}" == darwin* ]]; then
  export DYLD_LIBRARY_PATH
  DYLD_LIBRARY_PATH="${mongoc_build_dir:?}/src/libmongoc:${DYLD_LIBRARY_PATH:-}"
  DYLD_LIBRARY_PATH="${mongoc_build_dir:?}/src/libbson:${DYLD_LIBRARY_PATH:-}"
else
  export LD_LIBRARY_PATH
  LD_LIBRARY_PATH="${mongoc_build_dir:?}/src/libmongoc:${LD_LIBRARY_PATH:-}"
  LD_LIBRARY_PATH="${mongoc_build_dir:?}/src/libbson:${LD_LIBRARY_PATH:-}"
fi

command -V "${mongoc_ping:?}"

# Custom OpenSSL library may be installed. Only prepend to LD_LIBRARY_PATH when
# necessary to avoid conflicting with system binary requirements.
if [[ -d "${openssl_install_dir:?}" ]]; then
  if [[ -d "${openssl_install_dir:?}/lib64" ]]; then
    LD_LIBRARY_PATH="${openssl_install_dir:?}/lib64:${LD_LIBRARY_PATH:-}"
    DYLD_LIBRARY_PATH="${openssl_install_dir:?}/lib64:${DYLD_LIBRARY_PATH:-}"
  else
    LD_LIBRARY_PATH="${openssl_install_dir:?}/lib:${LD_LIBRARY_PATH:-}"
    DYLD_LIBRARY_PATH="${openssl_install_dir:?}/lib:${DYLD_LIBRARY_PATH:-}"
  fi
  export LD_LIBRARY_PATH DYLD_LIBRARY_PATH
fi

expect_success() {
  echo "Should succeed:"
  if ! "${mongoc_ping:?}" "${MONGODB_URI:?}"; then
    echo "Unexpected failure" 1>&2
    exit 1
  fi
}

expect_failure() {
  echo "Should fail:"
  if "${mongoc_ping:?}" "${MONGODB_URI:?}" >output.txt 2>&1; then
    echo "Unexpected - succeeded but it should not have" 1>&2
    cat output.txt
    exit 1
  else
    echo "failed as expected"
  fi

  # libmongoc really should give a better error message for a revocation failure...
  # It is not at all obvious what went wrong.
  if ! grep "No suitable servers found" output.txt >/dev/null; then
    echo "Unexpected error, expecting TLS handshake failure" 1>&2
    cat output.txt
    exit 1
  fi
}

case "${OSTYPE:?}" in
darwin*)
  find ~/Library/Keychains -name 'ocspcache.sqlite3' -exec sqlite3 "{}" 'DELETE FROM responses ;' \; >/dev/null || true
  ;;
cygwin)
  certutil -urlcache "*" delete >/dev/null || true
  ;;
esac

# Always add the tlsCAFile
declare ca_path="${mongoc_dir:?}/.evergreen/ocsp/${CERT_TYPE:?}/ca.pem"
declare base_uri="mongodb://localhost:${MONGODB_PORT:?}/?tls=true&tlsCAFile=${ca_path:?}"

# Only a handful of cases are expected to fail.
case "${TEST_COLUMN:?}" in
TEST_1 | TEST_3 | SOFT_FAIL_TEST)
  MONGODB_URI="${base_uri:?}" expect_success
  ;;
TEST_2 | TEST_4 | MALICIOUS_SERVER_TEST_1 | MALICIOUS_SERVER_TEST_2)
  MONGODB_URI="${base_uri:?}" expect_failure
  ;;
esac

# With insecure options, connection should always succeed
MONGODB_URI="${base_uri:?}&tlsInsecure=true" expect_success

# With insecure options, connection should always succeed
MONGODB_URI="${base_uri:?}&tlsAllowInvalidCertificates=true" expect_success
