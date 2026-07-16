#!/usr/bin/env bash

set -o errexit
set -o pipefail

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]}")")"

declare mongoc_dir
mongoc_dir="$(to_absolute "${script_dir}/../..")"

declare install_dir="${mongoc_dir}/install-dir"
declare cmake_prefix_path="${install_dir}"

. "${script_dir:?}/install-build-tools.sh"
install_build_tools
export CMAKE_GENERATOR="Ninja"

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

configure_flags_append "-DCMAKE_PREFIX_PATH=${cmake_prefix_path}"
configure_flags_append "-DCMAKE_SKIP_RPATH=TRUE" # Avoid hardcoding absolute paths to dependency libraries.
configure_flags_append "-DENABLE_CLIENT_SIDE_ENCRYPTION=ON"
configure_flags_append "-DENABLE_DEBUG_ASSERTIONS=ON"
configure_flags_append "-DENABLE_MAINTAINER_FLAGS=ON"
configure_flags_append "-DENABLE_SASL=OFF"
configure_flags_append "-DENABLE_SNAPPY=OFF"
configure_flags_append "-DENABLE_SSL=AUTO" # For Client Side Encryption.
configure_flags_append "-DENABLE_ZLIB=OFF"
configure_flags_append "-DENABLE_ZSTD=OFF"

if [[ "${OSTYPE:?}" == darwin* ]]; then
  # Prevent CMake from confusing custom LLVM linker with Apple LLVM linker:
  #   ld64.lld: warning: ignoring unknown argument: -search_paths_first
  #   ld64.lld: warning: ignoring unknown argument: -headerpad_max_install_names
  #   ld64.lld: warning: -sdk_version is required when emitting min version load command
  configure_flags_append "-DMONGO_USE_LLD=OFF"
fi

declare -a flags

case "${MARCH:-}" in
i686)
  flags+=("-m32" "-march=i386")
  ;;
esac

case "${HOSTTYPE:-}" in
s390x)
  flags+=("-march=z196" "-mtune=zEC12")
  ;;
x86_64)
  flags+=("-m64" "-march=x86-64")
  ;;
powerpc64le)
  flags+=("-mcpu=power8" "-mtune=power8" "-mcmodel=medium")
  ;;
esac

# CMake and compiler environment variables.
export CC
export CXX
export CFLAGS
export CXXFLAGS

CFLAGS+=" ${flags+${flags[*]}}"
CXXFLAGS+=" ${flags+${flags[*]}}"

if [[ "${OSTYPE}" == darwin* ]]; then
  CFLAGS+=" -Wno-unknown-pragmas"
fi

if [[ "${OSTYPE}" == darwin* && "${HOSTTYPE}" == "arm64" ]]; then
  configure_flags_append "-DCMAKE_OSX_ARCHITECTURES=arm64"
fi

# shellcheck source=.evergreen/scripts/add-build-dirs-to-paths.sh
. "${script_dir}/add-build-dirs-to-paths.sh"

export PKG_CONFIG_PATH
PKG_CONFIG_PATH="${install_dir}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

echo "Installing libmongocrypt..."
# shellcheck source=.evergreen/scripts/compile-libmongocrypt.sh
"${script_dir}/compile-libmongocrypt.sh" "$(command -v cmake)" "${mongoc_dir}" "${install_dir}" &>output.txt || {
  cat output.txt 1>&2
  exit 1
}
echo "Installing libmongocrypt... done."

# scan-build binary is available in different locations depending on the distro.
# Search for a match in order of preference as listed.
declare -a scan_build_directories

# Prioritize Apple LLVM on MacOS to avoid confusing CMake with inconsistent
# compilers and linkers.
if [[ -d /usr/local/Cellar/llvm ]]; then
  for dir in /opt/homebrew/Cellar/llvm /usr/local/Cellar/llvm; do
    # Max depth: llvm/bin/scan-build. Sort: prefer newer versions.
    for bin in $(find "${dir}" -maxdepth 3 -name 'scan-build' 2>/dev/null | sort -rV); do
      if command -v "${bin}"; then
        scan_build_directories+=("$(dirname "${bin}")")
      fi
    done
  done
fi

if command -v scan-build && command -v clang && command -v clang++; then
  scan_build_binary="scan-build"
  CC="clang"
  CXX="clang++"
else
  scan_build_directories+=(
    # Prefer toolchain scan-build if available.
    "/opt/mongodbtoolchain/v4/bin"
    "/opt/mongodbtoolchain/v3/bin"

    # Use system scan-build otherwise.
    "/usr/bin"
  )

  declare scan_build_binary
  for dir in "${scan_build_directories[@]}"; do
    if command -v "${dir}/scan-build" && command -v "${dir}/clang" && command -v "${dir}/clang++"; then
      # Ensure compilers are consistent with scan-build binary. All three binaries
      # should be present in the same directory.
      scan_build_binary="${dir}/scan-build"
      CC="${dir}/clang"
      CXX="${dir}/clang++"
      break
    fi
  done
fi
: "${scan_build_binary:?"could not find a scan-build binary!"}"
: "${CC:?}"
: "${CXX:?}"

# Use ccache if able.
. "${script_dir:?}/find-ccache.sh"
find_ccache_and_export_vars "$(pwd)" || true

"${scan_build_binary}" --use-cc="${CC}" --use-c++="${CXX}" cmake "${configure_flags[@]}" .

if [[ "${OSTYPE}" == darwin* ]]; then
  # MacOS does not have nproc.
  nproc() { sysctl -n hw.logicalcpu; }
fi

# If scan-build emits warnings, continue the task and upload scan results before marking task as a failure.
declare -r continue_command='{"status":"failed", "type":"test", "should_continue":true, "desc":"scan-build emitted one or more warnings or errors"}'

# Put clang static analyzer results in scan/ and fail build if warnings found.
"${scan_build_binary}" --use-cc="${CC}" --use-c++="${CXX}" -o scan --status-bugs cmake --build . -- -j "$(nproc)" ||
  curl -sS -d "${continue_command}" -H "Content-Type: application/json" -X POST localhost:2285/task_status
