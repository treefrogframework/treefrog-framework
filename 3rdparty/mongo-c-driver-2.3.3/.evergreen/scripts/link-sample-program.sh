#!/usr/bin/env bash
set -o errexit # Exit the script with error if any of the commands fail

# Supported/used environment variables:
#   LINK_STATIC                Whether to statically link to libmongoc
#   BUILD_SAMPLE_WITH_CMAKE    Link program w/ CMake. Default: use pkg-config.
#   ENABLE_SSL                 Set -DENABLE_SSL
#   ENABLE_SNAPPY              Set -DENABLE_SNAPPY
#   CMAKE                      Path to cmake executable.

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
  if command -v "${CMAKE_C_COMPILER_LAUNCHER:-}" && [[ "${OSTYPE:?}" == cygwin ]]; then
    configure_flags_append "-DCMAKE_POLICY_DEFAULT_CMP0141=NEW"
    configure_flags_append "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
  fi
fi

if [ "$ENABLE_SNAPPY" ]; then
  SNAPPY_CMAKE_OPTION="-DENABLE_SNAPPY=ON"
else
  SNAPPY_CMAKE_OPTION="-DENABLE_SNAPPY=OFF"
fi

if [ "$ENABLE_SSL" ]; then
  if [ "$OS" = "darwin" ]; then
    SSL_CMAKE_OPTION="-DENABLE_SSL:BOOL=DARWIN"
  else
    SSL_CMAKE_OPTION="-DENABLE_SSL:BOOL=OPENSSL"
  fi
else
  SSL_CMAKE_OPTION="-DENABLE_SSL:BOOL=OFF"
fi

if [ "$LINK_STATIC" ]; then
  STATIC_CMAKE_OPTION="-DENABLE_STATIC=ON -DENABLE_TESTS=ON"
else
  STATIC_CMAKE_OPTION="-DENABLE_STATIC=OFF -DENABLE_TESTS=OFF"
fi

ZSTD="AUTO"

cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCMAKE_PREFIX_PATH=$INSTALL_DIR/lib/cmake -DBUILD_TESTING=OFF $SSL_CMAKE_OPTION $SNAPPY_CMAKE_OPTION $STATIC_CMAKE_OPTION -DENABLE_ZSTD=$ZSTD "$SCRATCH_DIR"
cmake --build . --parallel
cmake --build . --parallel --target install

# Revert ccache options, they no longer apply.
unset CCACHE_BASEDIR CCACHE_NOHASHDIR

ls -l $INSTALL_DIR/lib

if [ "$OS" = "darwin" ] && [ "${HOSTTYPE:?}" != "arm64" ]; then
  if test -f $INSTALL_DIR/bin/mongoc$major-stat; then
    echo "mongoc$major-stat shouldn't have been installed"
    exit 1
  fi
else
  if test ! -f $INSTALL_DIR/bin/mongoc$major-stat; then
    echo "mongoc$major-stat missing!"
    exit 1
  else
    echo "mongoc$major-stat check ok"
  fi
fi

if [ "$BUILD_SAMPLE_WITH_CMAKE" ]; then
  # Test our CMake package config file with CMake's find_package command.
  if [ "$BUILD_SAMPLE_WITH_CMAKE_DEPRECATED" ]; then
    EXAMPLE_DIR=$SRCROOT/src/libmongoc/examples/cmake-deprecated/find_package
  else
    EXAMPLE_DIR=$SRCROOT/src/libmongoc/examples/cmake/find_package
  fi

  if [ "$LINK_STATIC" ]; then
    EXAMPLE_DIR="${EXAMPLE_DIR}_static"
  fi

  cd $EXAMPLE_DIR
  cmake -DCMAKE_PREFIX_PATH=$INSTALL_DIR/lib/cmake .
  cmake --build .
else
  # Test our pkg-config file.
  export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig
  cd $SRCROOT/src/libmongoc/examples

  if [ "$LINK_STATIC" ]; then
    echo "pkg-config output:"
    echo $(pkg-config --libs --cflags mongoc$major-static)
    env major=$major ./compile-with-pkg-config-static.sh
  else
    echo "pkg-config output:"
    echo $(pkg-config --libs --cflags mongoc$major)
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

echo "ldd hello_mongoc:"
$LDD hello_mongoc

./hello_mongoc
