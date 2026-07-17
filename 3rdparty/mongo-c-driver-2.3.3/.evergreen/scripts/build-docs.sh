#!/usr/bin/env bash

set -o errexit # Exit the script with error if any of the commands fail

. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths
. "$MONGOC_DIR/.evergreen/scripts/install-build-tools.sh"

# Check that a CLion user didn't accidentally convert NEWS from UTF-8 to ASCII
grep "á" NEWS >/dev/null || (echo "NEWS file appears to have lost its UTF-8 encoding?" || exit 1)

build_dir=$MONGOC_DIR/_build/for-docs
uvx cmake --fresh -G "Ninja" -S "$MONGOC_DIR" -B "$build_dir" \
  -D ENABLE_MAN_PAGES=ON \
  -D ENABLE_HTML_DOCS=ON \
  -D ENABLE_ZLIB=BUNDLED
uvx cmake --build "$build_dir" \
  --parallel 8 \
  --target bson-doc \
  --target mongoc-doc
