#!/usr/bin/env bash
#
# format.sh
#
# Usage:
#   uv run --frozen etc/shfmt-format-all.sh
#
# This script is meant to be run from the project root directory.

set -o errexit
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd "${script_dir:?}/.." && pwd)"

command -v find >/dev/null

include=(
  "${root_dir:?}/.evergreen"
  "${root_dir:?}/src"
  "${root_dir:?}/tools"
)

exclude=(
  "${root_dir:?}/.evergreen/scripts/uv-installer.sh"
)

mapfile -t files < <(find "${include[@]:?}" -name '*.sh' -type f | grep -v "${exclude[@]:?}")

for file in "${files[@]:?}"; do
  uv run --frozen --group format-scripts shfmt -i 2 -w "${file:?}"
done
