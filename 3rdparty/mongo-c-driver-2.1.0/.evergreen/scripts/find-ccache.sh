#!/usr/bin/env bash

find_ccache_check() {
  : "${1:?}"

  if command -v "${1:?}" >/dev/null; then
    declare version
    version="$("${1:?}" --version | head -n 1)"
    echo "Found ${1:?}: ${version:-}" 1>&2
    echo "${1:?}"
    return 0
  fi

  return 1
}

# find_ccache
#
# Usage:
#   find_ccache
#   ccache_binary="$(find_ccache)"
#   ccache_binary="$(find_ccache 2>/dev/null)"
#
# Return 0 (true) if a ccache binary is found.
# Return a non-zero (false) value otherwise.
#
# If successful, print the name of the ccache binary to stdout (pipe 1).
# Otherwise, no output is printed to stdout.
#
# Diagnostic messages may be printed to stderr (pipe 2). Redirect to /dev/null
# with `2>/dev/null` to silence these messages.
find_ccache() {
  # Most distros provide ccache via system paths.
  {
    find_ccache_check ccache && return
  } || true

  # Some distros provide ccache via mongodbtoolchain.
  {
    find_ccache_check /opt/mongodbtoolchain/v4/bin/ccache && return
  } || {
    find_ccache_check /opt/mongodbtoolchain/v3/bin/ccache && return
  } || true

  # Could not find ccache.
  echo "Could not find a ccache binary." 1>&2
  return 1
}

# Find, export, and set ccache env vars in one command for convenience.
# Requires base_dir as first argument.
# Returns a non-zero (false) value if a ccache binary is not found.
# Redirects find_ccache's stderr to stdout to avoid output sync issues on EVG.
find_ccache_and_export_vars() {
  declare base_dir
  base_dir="${1:?"missing base_dir"}"

  declare ccache_binary
  ccache_binary="$(find_ccache)" 2>&1 || return

  export CMAKE_C_COMPILER_LAUNCHER="${ccache_binary:?}"
  export CMAKE_CXX_COMPILER_LAUNCHER="${ccache_binary:?}"

  # Allow reuse of ccache compilation results between different build directories.
  if [[ "${OSTYPE:?}" =~ cygwin ]]; then
    export CCACHE_BASEDIR="$(cygpath -aw ${base_dir:?})"
  else
    export CCACHE_BASEDIR="${base_dir:?}"
  fi
  export CCACHE_NOHASHDIR=1
}
