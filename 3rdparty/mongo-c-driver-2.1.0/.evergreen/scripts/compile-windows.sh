#!/usr/bin/env bash

set -o errexit
set -o pipefail

set -o igncr # Ignore CR in this script for Windows compatibility.

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths

check_var_opt BYPASS_FIND_CMAKE "OFF"
check_var_opt C_STD_VERSION # CMake default: 99.
check_var_opt CC
check_var_opt CMAKE_GENERATOR
check_var_opt CMAKE_GENERATOR_PLATFORM
check_var_opt COMPILE_LIBMONGOCRYPT "OFF"
check_var_opt EXTRA_CONFIGURE_FLAGS
check_var_opt RELEASE "OFF"
check_var_opt SASL "SSPI"   # CMake default: AUTO.
check_var_opt SNAPPY        # CMake default: AUTO.
check_var_opt SRV           # CMake default: AUTO.
check_var_opt SSL "WINDOWS" # CMake default: OFF.
check_var_opt ZSTD          # CMake default: AUTO.
check_var_opt ZLIB          # CMake default: AUTO.

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]}")")"

declare mongoc_dir
mongoc_dir="$(to_absolute "${script_dir}/../..")"

declare -a configure_flags

configure_flags_append() {
  configure_flags+=("${@:?}")
}

configure_flags_append_if_not_null() {
  declare var="${1:?}"
  if [[ -n "${!var:-}" ]]; then
    shift
    configure_flags+=("${@:?}")
  fi
}

declare install_dir="${mongoc_dir}/install-dir"

declare -a extra_configure_flags
IFS=' ' read -ra extra_configure_flags <<<"${EXTRA_CONFIGURE_FLAGS:-}"

## * Note: For additional configure-time context, the following lines can be
## * uncommented to enable CMake's debug output:
# configure_flags_append --log-level=debug
# configure_flags_append --log-context

configure_flags_append "-DCMAKE_INSTALL_PREFIX=$(native-path "${install_dir}")"
configure_flags_append "-DCMAKE_PREFIX_PATH=$(native-path "${install_dir}")"
configure_flags_append "-DENABLE_MAINTAINER_FLAGS=ON"

configure_flags_append_if_not_null C_STD_VERSION "-DCMAKE_C_STANDARD=${C_STD_VERSION:-}"
configure_flags_append_if_not_null SASL "-DENABLE_SASL=${SASL:-}"
configure_flags_append_if_not_null SNAPPY "-DENABLE_SNAPPY=${SNAPPY:-}"
configure_flags_append_if_not_null SRV "-DENABLE_SRV=${SRV:-}"
configure_flags_append_if_not_null ZLIB "-DENABLE_ZLIB=${ZLIB:-}"

declare build_config
if [[ "${RELEASE}" == "ON" ]]; then
  build_config="RelWithDebInfo"
else
  build_config="Debug"
  configure_flags_append "-DENABLE_DEBUG_ASSERTIONS=ON"
fi
configure_flags_append "-DCMAKE_BUILD_TYPE=${build_config:?}"

if [ "${SSL}" == "OPENSSL_STATIC" ]; then
  configure_flags_append "-DENABLE_SSL=OPENSSL" "-DOPENSSL_USE_STATIC_LIBS=ON"
else
  configure_flags_append "-DENABLE_SSL=${SSL}"
fi

declare cmake_binary
if [[ "${BYPASS_FIND_CMAKE:-}" == "OFF" ]]; then
  # shellcheck source=.evergreen/scripts/find-cmake-version.sh
  . "${script_dir}/find-cmake-latest.sh"

  cmake_binary="$(find_cmake_latest)"
else
  cmake_binary="cmake"
fi

"${cmake_binary:?}" --version

export CMAKE_BUILD_PARALLEL_LEVEL
CMAKE_BUILD_PARALLEL_LEVEL="$(nproc)"

declare build_dir
build_dir="cmake-build"

if [[ "${CC}" =~ 'gcc' ]]; then
  # MinGW has trouble compiling src/cpp-check.cpp without some assistance.
  configure_flags_append "-DCMAKE_CXX_STANDARD=11"

  env \
    "${cmake_binary:?}" \
    -S . \
    -B "${build_dir:?}" \
    -G "Ninja" \
    "${configure_flags[@]}" \
    "${extra_configure_flags[@]}"

  env "${cmake_binary:?}" --build "${build_dir:?}"
  env "${cmake_binary:?}" --build "${build_dir:?}" --target mongo_c_driver_tests
  env "${cmake_binary:?}" --build "${build_dir:?}" --target mongo_c_driver_examples
  exit 0
fi

# MSBuild needs additional assistance.
# https://devblogs.microsoft.com/cppblog/improved-parallelism-in-msbuild/
export UseMultiToolTask=1
export EnforceProcessCountAcrossBuilds=1

if [ "${COMPILE_LIBMONGOCRYPT}" = "ON" ]; then
  echo "Installing libmongocrypt..."
  # shellcheck source=.evergreen/scripts/compile-libmongocrypt.sh
  "${script_dir}/compile-libmongocrypt.sh" "${cmake_binary}" "$(native-path "${mongoc_dir}")" "${install_dir}" &>output.txt || {
    cat output.txt 1>&2
    exit 1
  }
  echo "Installing libmongocrypt... done."

  # Fail if the C driver is unable to find the installed libmongocrypt.
  configure_flags_append "-DENABLE_CLIENT_SIDE_ENCRYPTION=ON"
fi

# Use ccache if able.
. "${script_dir:?}/find-ccache.sh"
find_ccache_and_export_vars "$(pwd)" || true
if command -v "${CMAKE_C_COMPILER_LAUNCHER:-}" && [[ "${OSTYPE:?}" == cygwin ]]; then
  configure_flags_append "-DCMAKE_POLICY_DEFAULT_CMP0141=NEW"
  configure_flags_append "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
fi

"${cmake_binary:?}" -S . -B "${build_dir:?}" "${configure_flags[@]}" "${extra_configure_flags[@]}"
"${cmake_binary:?}" --build "${build_dir:?}" --config "${build_config:?}"
"${cmake_binary:?}" --install "${build_dir:?}" --config "${build_config:?}"

# For use by test tasks, which directly use the binary directory contents.
"${cmake_binary:?}" --build "${build_dir:?}" --config "${build_config:?}" --target mongo_c_driver_tests

# Also validate examples.
"${cmake_binary:?}" --build "${build_dir:?}" --config "${build_config:?}" --target mongo_c_driver_examples

if [[ "$EXTRA_CONFIGURE_FLAGS" != *"ENABLE_MONGOC=OFF"* ]]; then
  # Check public headers for extra warnings.
  "${cmake_binary:?}" --build "${build_dir:?}" --config "${build_config:?}" --target public-header-warnings
fi
