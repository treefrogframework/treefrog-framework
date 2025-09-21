#!/usr/bin/env bash

set -o errexit
set -o pipefail

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths

check_var_opt CC
check_var_opt CMAKE_GENERATOR
check_var_opt CMAKE_GENERATOR_PLATFORM

check_var_req C_STD_VERSION
check_var_opt CFLAGS
check_var_opt CXXFLAGS
check_var_opt MARCH

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]}")")"

declare mongoc_dir
mongoc_dir="$(to_absolute "${script_dir}/../..")"

declare libmongocrypt_install_dir="${mongoc_dir}/libmongocrypt-install-dir"

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

configure_flags_append "-DCMAKE_PREFIX_PATH=${libmongocrypt_install_dir:?}"
configure_flags_append "-DCMAKE_SKIP_RPATH=TRUE" # Avoid hardcoding absolute paths to dependency libraries.
configure_flags_append "-DENABLE_CLIENT_SIDE_ENCRYPTION=ON"
configure_flags_append "-DENABLE_DEBUG_ASSERTIONS=ON"
configure_flags_append "-DENABLE_MAINTAINER_FLAGS=ON"

if [[ "${C_STD_VERSION}" == "latest" ]]; then
  [[ "${CMAKE_GENERATOR:-}" =~ "Visual Studio" ]] || {
    echo "C_STD_VERSION=latest to enable /std:clatest is only supported with Visual Studio generators" 1>&2
    exit 1
  }

  configure_flags_append "-DCMAKE_C_FLAGS=/std:clatest"
else
  configure_flags_append_if_not_null C_STD_VERSION "-DCMAKE_C_STANDARD=${C_STD_VERSION}"
fi

if [[ "${OSTYPE}" == darwin* && "${HOSTTYPE}" == "arm64" ]]; then
  configure_flags_append "-DCMAKE_OSX_ARCHITECTURES=arm64"
fi

if [[ "${CMAKE_GENERATOR:-}" =~ "Visual Studio" ]]; then
  # Avoid C standard conformance issues with Windows 10 SDK headers.
  # See: https://developercommunity.visualstudio.com/t/stdc17-generates-warning-compiling-windowsh/1249671#T-N1257345
  configure_flags_append "-DCMAKE_SYSTEM_VERSION=10.0.20348.0"
fi

declare -a flags

if [[ "${CMAKE_GENERATOR:-}" =~ "Visual Studio" ]]; then
  # Even with -DCMAKE_SYSTEM_VERSION=10.0.20348.0, winbase.h emits conformance warnings.
  flags+=('/wd5105')
fi

if [[ "${OSTYPE}" == darwin* ]]; then
  flags+=('-Wno-unknown-pragmas')
fi

# CMake and compiler environment variables.
export CFLAGS
export CXXFLAGS

CFLAGS+=" ${flags+${flags[*]}}"
CXXFLAGS+=" ${flags+${flags[*]}}"

# Ensure find-cmake-latest.sh is sourced *before* add-build-dirs-to-paths.sh
# to avoid interfering with potential CMake build configuration.
# shellcheck source=.evergreen/scripts/find-cmake-latest.sh
. "${script_dir}/find-cmake-latest.sh"
declare cmake_binary
cmake_binary="$(find_cmake_latest)"

declare mongoc_build_dir mongoc_install_dir
mongoc_build_dir="cmake-build"
mongoc_install_dir="cmake-install"

configure_flags_append "-DCMAKE_INSTALL_PREFIX=${mongoc_install_dir:?}"

# shellcheck source=.evergreen/scripts/add-build-dirs-to-paths.sh
. "${script_dir}/add-build-dirs-to-paths.sh"

if [[ "${OSTYPE}" == darwin* ]]; then
  # MacOS does not have nproc.
  nproc() {
    sysctl -n hw.logicalcpu
  }
fi

export CMAKE_BUILD_PARALLEL_LEVEL
CMAKE_BUILD_PARALLEL_LEVEL="$(nproc)"

if [[ "${CMAKE_GENERATOR:-}" =~ "Visual Studio" ]]; then
  # MSBuild needs additional assistance.
  # https://devblogs.microsoft.com/cppblog/improved-parallelism-in-msbuild/
  export UseMultiToolTask=1
  export EnforceProcessCountAcrossBuilds=1
fi

echo "Checking requested C standard is supported..."
pushd "$(mktemp -d)"
cat >CMakeLists.txt <<DOC
cmake_minimum_required(VERSION 3.30...4.0)
project(c_standard_latest LANGUAGES C)
set(c_std_version "${C_STD_VERSION:?}")
if(c_std_version STREQUAL "latest") # Special-case MSVC's /std:clatest flag.
  include(CheckCCompilerFlag)
  check_c_compiler_flag("/std:clatest" cflag_std_clatest)
  if(cflag_std_clatest)
    message(STATUS "/std:clatest is supported")
  else()
    message(FATAL_ERROR "/std:clatest is not supported")
  endif()
else()
  # Ensure "old" standard versions are not compared as "newer" than C11/C17/etc.
  set(old_std_versions 90 99)

  macro(success)
    message(STATUS "Latest C standard \${CMAKE_C_STANDARD_LATEST} is newer than \${c_std_version}")
  endmacro()
  macro(failure)
    message(FATAL_ERROR "Latest C standard \${CMAKE_C_STANDARD_LATEST} is older than \${c_std_version}")
  endmacro()

  if (CMAKE_C_STANDARD_LATEST IN_LIST old_std_versions AND c_std_standard IN_LIST old_std_versions)
    if (CMAKE_C_STANDARD_LATEST GREATER_EQUAL c_std_version)
      success() # Both are old: latest >= version
    else()
      failure() # Both are old: latest < version
    endif()
  elseif(CMAKE_C_STANDARD_LATEST IN_LIST old_std_versions)
    failure() # latest (old) < version (new)
  elseif(c_std_version IN_LIST old_std_versions)
    success() # latest (new) >= version (old)
  elseif(CMAKE_C_STANDARD_LATEST GREATER_EQUAL c_std_version)
    success() # Both are new: latest >= version.
  else()
    failure() # Both are new: latest < version.
  endif()
endif()
DOC
"${cmake_binary:?}" -S . -B build
popd # "$(tmpfile -d)"
echo "Checking requested C standard is supported... done."

echo "Installing libmongocrypt..."
# shellcheck source=.evergreen/scripts/compile-libmongocrypt.sh
"${script_dir}/compile-libmongocrypt.sh" "${cmake_binary}" "${mongoc_dir}" "${libmongocrypt_install_dir:?}" &>output.txt || {
  cat output.txt 1>&2
  exit 1
}
echo "Installing libmongocrypt... done."

# Use ccache if able.
. "${script_dir:?}/find-ccache.sh"
find_ccache_and_export_vars "$(pwd)" || true
if command -v "${CMAKE_C_COMPILER_LAUNCHER:-}" && [[ "${OSTYPE:?}" == cygwin ]]; then
  configure_flags_append "-DCMAKE_POLICY_DEFAULT_CMP0141=NEW"
  configure_flags_append "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
fi

echo "CFLAGS: ${CFLAGS}"
echo "configure_flags: ${configure_flags[*]}"

if [[ "${CMAKE_GENERATOR:-}" =~ "Visual Studio" ]]; then
  all_target="ALL_BUILD"
else
  all_target="all"
fi

# Ensure we're starting with a clean slate.
rm -rf "${mongoc_build_dir:?}" "${mongoc_install_dir:?}"

"${cmake_binary}" -S . -B "${mongoc_build_dir:?}" "${configure_flags[@]}"
"${cmake_binary}" --build "${mongoc_build_dir:?}" --config Debug \
  --target mongo_c_driver_tests \
  --target mongo_c_driver_examples \
  --target public-header-warnings \
  --target "${all_target:?}"
"${cmake_binary}" --install "${mongoc_build_dir:?}" --config Debug

# "lib" vs. "lib64"
if [[ -d "${mongoc_install_dir:?}/lib64" ]]; then
  lib_dir="lib64"
else
  lib_dir="lib"
fi

# This file should not be deleted!
touch "${mongoc_install_dir:?}/${lib_dir:?}/canary.txt"

# Linux/MacOS: uninstall.sh
# Windows:     uninstall.cmd
"${cmake_binary}" --build "${mongoc_build_dir:?}" --target uninstall

# No files should remain except canary.txt.
# No directories except top-level directories should remain.
echo "Checking results of uninstall..."
diff <(cd "${mongoc_install_dir:?}" && find . -mindepth 1 | sort) <(
  echo "./bin"
  echo "./include"
  echo "./${lib_dir:?}"
  echo "./${lib_dir:?}/canary.txt"
  echo "./share"
)
echo "Checking results of uninstall... done."
