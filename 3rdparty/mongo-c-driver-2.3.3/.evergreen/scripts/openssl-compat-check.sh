#!/usr/bin/env bash

set -o errexit
set -o pipefail

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths

check_var_req OPENSSL_VERSION
check_var_opt OPENSSL_USE_STATIC_LIBS

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]}")")"

declare mongoc_dir mongoc_build_dir mongoc_install_dir
mongoc_dir="$(to_absolute "${script_dir}/../..")"
mongoc_build_dir="${mongoc_dir:?}/cmake-build"
mongoc_install_dir="${mongoc_dir:?}/install-dir"

declare openssl_install_dir
openssl_install_dir="${mongoc_dir:?}/openssl-install-dir"

declare -a openssl_cmake_flags=("-DOPENSSL_ROOT_DIR=${openssl_install_dir:?}")

if [[ "${OPENSSL_USE_STATIC_LIBS:-}" == "ON" ]]; then
  openssl_cmake_flags+=("-DOPENSSL_USE_STATIC_LIBS=TRUE")
fi

declare libmongocrypt_install_dir="${mongoc_dir}/install-dir"

. "${script_dir:?}/install-build-tools.sh"
install_build_tools
export CMAKE_GENERATOR="Ninja"

# libmongocrypt must use the same OpenSSL library.
echo "Installing libmongocrypt..."
# shellcheck source=.evergreen/scripts/compile-libmongocrypt.sh
"${script_dir}/compile-libmongocrypt.sh" "$(command -v cmake)" "${mongoc_dir:?}" "${mongoc_install_dir:?}" "${openssl_cmake_flags[@]:?}" &>output.txt || {
  cat output.txt 1>&2
  exit 1
}
echo "Installing libmongocrypt... done."

# Use ccache if able.
. "${script_dir:?}/find-ccache.sh"
find_ccache_and_export_vars "$(pwd)" || true

declare -a configure_flags

configure_flags_append() {
  configure_flags+=("${@:?}")
}

configure_flags_append "-DCMAKE_INSTALL_PREFIX=${mongoc_install_dir:?}"
configure_flags_append "-DCMAKE_BUILD_TYPE=Debug"
configure_flags_append "-DCMAKE_PREFIX_PATH=${libmongocrypt_install_dir:?}"
configure_flags_append "-DCMAKE_SKIP_RPATH=TRUE" # Avoid hardcoding absolute paths to dependency libraries.
configure_flags_append "-DENABLE_CLIENT_SIDE_ENCRYPTION=ON"
configure_flags_append "-DENABLE_SSL=OPENSSL"
configure_flags_append "-DENABLE_SASL=AUTO"
configure_flags+=("${openssl_cmake_flags[@]:?}")

echo "configure_flags: ${configure_flags[*]}"

echo "Configuring..."
cmake -S . -B "${mongoc_build_dir:?}" "${configure_flags[@]}" >/dev/null
echo "Configuring... done."

echo "Verifying the correct OpenSSL library was found..."
(
  log="$(perl -lne 'print $1 if m|^FIND_PACKAGE_MESSAGE_DETAILS_OpenSSL:INTERNAL=(.*)$|' "${mongoc_build_dir:?}/CMakeCache.txt")"
  pattern="^\[([^\]]*)\]\[([^\]]*)\]\[([^\]]*)\]\[([^\]]*)\]" # [library][include][?][version]

  library="$(echo "${log:?}" | perl -lne "print \$1 if m|${pattern:?}|")"
  version="$(echo "${log:?}" | perl -lne "print \$4 if m|${pattern:?}|")"

  [[ "${library:-}" =~ "${openssl_install_dir:?}" ]] || {
    echo "expected \"${openssl_install_dir:?}\" in \"${library:-}\""
    exit 1
  } >&2

  if [[ "${OPENSSL_USE_STATIC_LIBS:-}" == "ON" ]]; then
    [[ "${library:-}" =~ "libcrypto.a" ]] || {
      echo "expected \"libcrypto.a\" in \"${library:-}\""
      exit 1
    } >&2
  else
    [[ "${library:-}" =~ "libcrypto.so" ]] || {
      echo "expected \"libcrypto.so\" in \"${library:-}\""
      exit 1
    } >&2
  fi

  [[ "${version:-}" =~ "${OPENSSL_VERSION:?}" ]] || {
    echo "expected \"${OPENSSL_VERSION:?}\" in \"${version:-}\""
    exit 1
  } >&2
)
echo "Verifying the correct OpenSSL library was found... done."

echo "Building..."
cmake --build "${mongoc_build_dir:?}" --target all mongoc-ping test-mongoc-gssapi >/dev/null
echo "Building... done."

echo "Installing..."
cmake --install "${mongoc_build_dir:?}"
echo "Installing... done."
