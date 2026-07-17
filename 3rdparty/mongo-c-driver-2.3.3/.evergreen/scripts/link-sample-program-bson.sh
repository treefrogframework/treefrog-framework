#!/usr/bin/env bash
set -o errexit # Exit the script with error if any of the commands fail

# Supported/used environment variables:
#   LINK_STATIC              Whether to statically link to libbson
#   BUILD_SAMPLE_WITH_CMAKE  Link program w/ CMake. Default: use pkg-config.

echo "LINK_STATIC=$LINK_STATIC BUILD_SAMPLE_WITH_CMAKE=$BUILD_SAMPLE_WITH_CMAKE"

DIR=$(dirname $0)

. "${DIR:?}/install-build-tools.sh"
install_build_tools
export CMAKE_GENERATOR="Ninja"

# The major version of the project. Appears in certain install filenames.
_full_version=$(cat "$DIR/../../VERSION_CURRENT")
version="${_full_version%-*}" # 1.2.3-dev → 1.2.3
major="${version%%.*}"        # 1.2.3     → 1
echo "major version: $major"
echo " full version: $version"

# Get the kernel name, lowercased
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
echo "OS: $OS"

if [ "$OS" = "darwin" ]; then
  LDD="otool -L"
else
  LDD=ldd
fi

SRCROOT=$(pwd)
SCRATCH_DIR=$(pwd)/.scratch
rm -rf "$SCRATCH_DIR"
mkdir -p "$SCRATCH_DIR"
cp -r -- "$SRCROOT"/* "$SCRATCH_DIR"

BUILD_DIR=$SCRATCH_DIR/build-dir
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

INSTALL_DIR=$SCRATCH_DIR/install-dir
rm -rf $INSTALL_DIR
mkdir -p $INSTALL_DIR

cd $BUILD_DIR

# Use ccache if able.
if [[ -f $DIR/find-ccache.sh ]]; then
  . $DIR/find-ccache.sh
  find_ccache_and_export_vars "$SCRATCH_DIR" || true
fi

if [ "$LINK_STATIC" ]; then
  # Our CMake system builds shared and static by default.
  cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DBUILD_TESTING=OFF -DENABLE_TESTS=OFF -DENABLE_MONGOC=OFF "$SCRATCH_DIR"
  cmake --build . --parallel
  cmake --build . --parallel --target install
else
  cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DBUILD_TESTING=OFF -DENABLE_TESTS=OFF -DENABLE_MONGOC=OFF -DENABLE_STATIC=OFF "$SCRATCH_DIR"
  cmake --build . --parallel
  cmake --build . --parallel --target install
fi

# Revert ccache options, they no longer apply.
unset CCACHE_BASEDIR CCACHE_NOHASHDIR

ls -l $INSTALL_DIR/lib

cd $SRCROOT

if [ "$BUILD_SAMPLE_WITH_CMAKE" ]; then
  EXAMPLE_DIR=$SRCROOT/src/libbson/examples/cmake/find_package

  if [ "$LINK_STATIC" ]; then
    EXAMPLE_DIR="${EXAMPLE_DIR}_static"
  fi

  cd $EXAMPLE_DIR
  cmake -DCMAKE_PREFIX_PATH=$INSTALL_DIR/lib/cmake .
  cmake --build . --parallel
else
  # Test our pkg-config file.
  export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig
  cd $SRCROOT/src/libbson/examples

  if [ "$LINK_STATIC" ]; then
    echo "pkg-config output:"
    echo $(pkg-config --libs --cflags bson$major-static)
    env major=$major ./compile-with-pkg-config-static.sh
  else
    echo "pkg-config output:"
    echo $(pkg-config --libs --cflags bson$major)
    env major=$major ./compile-with-pkg-config.sh
  fi
fi

if [ ! "$LINK_STATIC" ]; then
  if [ "$OS" = "darwin" ]; then
    export DYLD_LIBRARY_PATH=$INSTALL_DIR/lib
  else
    export LD_LIBRARY_PATH=$INSTALL_DIR/lib
  fi
fi

echo "ldd hello_bson:"
$LDD hello_bson

./hello_bson
