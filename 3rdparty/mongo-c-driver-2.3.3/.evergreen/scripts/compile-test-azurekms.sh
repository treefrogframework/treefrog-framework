#!/usr/bin/env bash
set -o errexit
set -o pipefail
set -o nounset

# Working directory is expected to be mongo-c-driver repo.
ROOT=$(pwd)
INSTALL_DIR=$ROOT/install

if [[ "${distro_id:?}" == "debian11-small" ]]; then
  # Temporary workaround for lack of uv on `debian11`. TODO: remove after DEVPROD-23011 is resolved.
  uv_dir="$(mktemp -d)"
  python3 -m virtualenv "${uv_dir:?}"
  # shellcheck source=/dev/null
  (. "${uv_dir:?}/bin/activate" && python -m pip install uv)
  PATH="${uv_dir:?}/bin:${PATH:-}"
  command -V uv >/dev/null
fi

. .evergreen/scripts/install-build-tools.sh
install_build_tools
export CMAKE_GENERATOR="Ninja"

echo "Installing libmongocrypt ... begin"
.evergreen/scripts/compile-libmongocrypt.sh "$(command -v cmake)" "$ROOT" "$INSTALL_DIR" &>output.txt || {
  cat output.txt 1>&2
  exit 1
}
echo "Installing libmongocrypt ... end"

# Use ccache if able.
. .evergreen/scripts/find-ccache.sh
find_ccache_and_export_vars "$(pwd)" || true

echo "Compile test-azurekms ... begin"
# Disable unnecessary dependencies. test-azurekms is copied to a remote host for testing, which may not have all dependent libraries.
cmake \
  -DENABLE_SASL=OFF \
  -DENABLE_SNAPPY=OFF \
  -DENABLE_ZSTD=OFF \
  -DENABLE_ZLIB=OFF \
  -DENABLE_SRV=OFF \
  -DENABLE_CLIENT_SIDE_ENCRYPTION=ON \
  -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
  .
cmake --build . --target test-azurekms
echo "Compile test-azurekms ... end"
