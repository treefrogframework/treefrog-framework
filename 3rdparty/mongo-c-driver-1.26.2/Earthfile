VERSION --arg-scope-and-set --pass-args 0.7
LOCALLY

# For target names, descriptions, and build parameters, run the "doc" Earthly subcommand.

# Injects utilities into the environment
INIT:
    COMMAND
    COPY --chmod=755 tools/str.sh /usr/local/bin/str

# PREP_CMAKE "warms up" the CMake installation cache for the current environment
PREP_CMAKE:
    COMMAND
    LET scratch=/opt/mongoc-cmake
    # Copy the minimal amount that we need, as to avoid cache invalidation
    COPY tools/use.sh tools/platform.sh tools/paths.sh tools/base.sh tools/download.sh \
        $scratch/tools/
    COPY .evergreen/scripts/find-cmake-version.sh \
        .evergreen/scripts/use-tools.sh \
        .evergreen/scripts/find-cmake-latest.sh \
        .evergreen/scripts/cmake.sh \
        $scratch/.evergreen/scripts/
    # "Install" a shim that runs our managed CMake executable:
    RUN printf '#!/bin/sh\n /opt/mongoc-cmake/.evergreen/scripts/cmake.sh "$@"\n' \
            > /usr/local/bin/cmake && \
        chmod +x /usr/local/bin/cmake
    # Executing any CMake command will warm the cache:
    RUN cmake --version

# version-current :
#   Create the VERSION_CURRENT file using Git. This file is exported as an artifact at /
version-current:
    # Run on Alpine, which does this work the fastest
    FROM alpine:3.18
    # Install Python and Git, the only things required for this job:
    RUN apk add git python3
    COPY --dir .git/ build/calc_release_version.py /s/
    # Calculate it:
    RUN cd /s/ && \
        python calc_release_version.py --next-minor > VERSION_CURRENT
    SAVE ARTIFACT /s/VERSION_CURRENT

# BUILD_AND_INSTALL executes the mongo-c-driver build and installs it to a prefix
BUILD_AND_INSTALL:
    COMMAND
    # The configuration to be built
    ARG config=RelWithDebInfo
    # The prefix at which to install the built result
    ARG install_prefix=/opt/mongo-c-driver
    # Build configuration parameters. Will be case-normalized for CMake usage.
    ARG --required sasl
    ARG --required tls
    LET source_dir=/opt/mongoc/source
    LET build_dir=/opt/mongoc/build
    COPY --dir \
        src/ \
        build/ \
        COPYING \
        CMakeLists.txt \
        README.rst \
        THIRD_PARTY_NOTICES \
        NEWS \
        "$source_dir"
    COPY +version-current/ $source_dir
    ENV CCACHE_HOME=/root/.cache/ccache
    RUN cmake -S "$source_dir" -B "$build_dir" -G "Ninja Multi-Config" \
        -D ENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
        -D ENABLE_MAINTAINER_FLAGS=ON \
        -D ENABLE_SHM_COUNTERS=ON \
        -D ENABLE_EXTRA_ALIGNMENT=OFF \
        -D ENABLE_SASL=$(echo $sasl | str upper) \
        -D ENABLE_SNAPPY=ON \
        -D ENABLE_SRV=ON \
        -D ENABLE_ZLIB=BUNDLED \
        -D ENABLE_SSL=$(echo $tls | str upper) \
        -D ENABLE_COVERAGE=ON \
        -D ENABLE_DEBUG_ASSERTIONS=ON \
        -Werror
    RUN --mount=type=cache,target=$CCACHE_HOME \
        env CCACHE_BASE="$source_dir" \
            cmake --build $build_dir --config $config
    RUN cmake --install $build_dir --prefix="$install_prefix" --config $config

alpine-base:
    ARG --required version
    FROM alpine:$version
    # XXX: On Alpine, we just use the system's CMake. At time of writing, it is
    # very up-to-date and much faster than building our own from source (since
    # Kitware does not (yet) provide libmuslc builds of CMake)
    RUN apk add cmake ninja-is-really-ninja gcc musl-dev g++
    DO +INIT
    ARG --required tls
    ARG --required sasl
    IF str test "$sasl" -ieq "Cyrus"
        RUN apk add cyrus-sasl-dev
    END
    IF str test "$tls" -ieq "LibreSSL" || str test $tls -ieq auto
        RUN apk add libressl-dev
    ELSE IF str test "$tls" -ieq "OpenSSL"
        RUN apk add openssl-dev
    END

alpine-build-env-base:
    ARG --required version
    FROM +alpine-base --version=$version
    RUN apk add snappy-dev ccache

alpine-test-env-base:
    ARG --required version
    FROM +alpine-base --version=$version
    RUN apk add snappy

# alpine3.18-build-env :
#   A build environment based on Alpine Linux version 3.18
alpine3.18-build-env:
    FROM +alpine-build-env-base --version=3.18

alpine3.18-test-env:
    FROM +alpine-test-env-base --version=3.18

archlinux-base:
    FROM archlinux
    RUN pacman --sync --refresh --sysupgrade --noconfirm --quiet ninja gcc snappy
    DO +INIT
    ARG tls
    # We don't install libsasl2 here, because it's pre-installed on Arch
    IF str test "$tls" -ieq "LibreSSL" || str test $tls -ieq auto
        RUN pacman --sync --refresh --sysupgrade --noconfirm --quiet libressl
    END
    IF str test "$tls" -ieq "OpenSSL" || str test $tls -ieq auto
        RUN pacman --sync --refresh --sysupgrade --noconfirm --quiet openssl
    END

# archlinux-build-env :
#   A build environment based on Arch Linux
archlinux-build-env:
    FROM +archlinux-base
    DO +PREP_CMAKE
    RUN pacman --sync --refresh --sysupgrade --noconfirm --quiet ccache

archlinux-test-env:
    FROM +archlinux-base
    DO +PREP_CMAKE

ubuntu-base:
    ARG --required version
    FROM ubuntu:$version
    RUN apt-get update && apt-get -y install curl build-essential
    DO +INIT
    ARG --required sasl
    ARG --required tls
    IF str test "$sasl" -ieq Cyrus
        RUN apt-get update && apt-get -y install libsasl2-dev
    END
    IF str test "$tls" -ieq LibreSSL
        RUN echo "Ubuntu does not support LibreSSL" && exit 1
    ELSE IF str test $tls -ieq OpenSSL || str test $tls -ieq auto
        RUN apt-get update && apt-get -y install libssl-dev
    END

# u22-build-env :
#   A build environment based on Ubuntu 22.04
u22-build-env:
    FROM +ubuntu-base --version=22.04
    # Build dependencies:
    RUN apt-get update && apt-get -y install \
            ninja-build gcc ccache libsnappy-dev zlib1g-dev
    DO +PREP_CMAKE

u22-test-env:
    FROM +ubuntu-base --version=22.04
    RUN apt-get update && apt-get -y install libsnappy1v5 libsasl2-2 ninja-build
    DO +PREP_CMAKE

env-warmup:
    ARG --required env
    BUILD +$env-build-env
    BUILD +$env-test-env

# build :
#   Build libmongoc and libbson using the specified environment.
#
# The --env argument specifies the build environment, using “+${env}-build-env”
# as the build environment target. Refer to the list of targets for a list of
# available environments.
build:
    # env is an argument
    ARG --required env
    FROM +$env-build-env
    DO --pass-args +BUILD_AND_INSTALL
    SAVE ARTIFACT /opt/mongoc/build/* /build-tree/
    SAVE ARTIFACT /opt/mongo-c-driver/* /root/

# test-example will build one of the libmongoc example projects using the build
# that comes from the +build target.
test-example:
    ARG --required env
    FROM +$env-test-env
    # Grab the built
    COPY +build/root /opt/mongo-c-driver
    COPY --dir \
        src/libmongoc/examples/cmake \
        src/libmongoc/examples/cmake-deprecated \
        src/libmongoc/examples/hello_mongoc.c \
        /opt/mongoc-test/
    RUN cmake \
            -S /opt/mongoc-test/cmake/find_package \
            -B /bld \
            -G Ninja \
            -D CMAKE_PREFIX_PATH=/opt/mongo-c-driver
    RUN cmake --build /bld

# test-cxx-driver :
#   Clone and build the mongo-cxx-driver project, using the current mongo-c-driver
#   for the build.
#
# The “--test_mongocxx_ref” argument must be a clone-able Git ref. The driver source
# will be cloned at this point and built.
#
# The “--cxx_version_current” argument will be inserted into the VERSION_CURRENT
# file for the cxx-driver build.
test-cxx-driver:
    ARG --required env
    ARG --required test_mongocxx_ref
    ARG cxx_version_current=0.0.0
    FROM +$env-build-env
    COPY +build/root /opt/mongo-c-driver
    LET source=/opt/mongo-cxx-driver/src
    LET build=/opt/mongo-cxx-driver/bld
    GIT CLONE --branch=$test_mongocxx_ref https://github.com/mongodb/mongo-cxx-driver.git $source
    RUN echo $cxx_version_current > $source/build/VERSION_CURRENT
    RUN cmake -S $source -B $build -G Ninja -D CMAKE_PREFIX_PATH=/opt/mongo-c-driver -D CMAKE_CXX_STANDARD=17
    ENV CCACHE_HOME=/root/.cache/ccache
    ENV CCACHE_BASE=$source
    RUN --mount=type=cache,target=$CCACHE_HOME cmake --build $build

# Simultaneously builds and tests multiple different platforms
multibuild:
    BUILD +test-example --env=u22 --env=archlinux --env=alpine3.18 \
            --sasl=Cyrus --sasl=off \
            --tls=OpenSSL --tls=off
    # Note: At time of writing, Ubuntu does not support LibreSSL, so run those
    #   tests on a separate BUILD line that does not include Ubuntu:
    BUILD +test-example --env=archlinux --env=alpine3.18 \
            --sasl=Cyrus --sasl=off \
            --tls=LibreSSL

# test-vcpkg-classic :
#   Builds src/libmongoc/examples/cmake/vcpkg by using vcpkg to download and
#   install a mongo-c-driver build in "classic mode". *Does not* use the local
#   mongo-c-driver repository.
test-vcpkg-classic:
    FROM +vcpkg-base
    RUN vcpkg install mongo-c-driver
    RUN rm -rf _build && \
        make test-classic

# test-vcpkg-manifest-mode :
#   Builds src/libmongoc/examples/cmake/vcpkg by using vcpkg to download and
#   install a mongo-c-driver package based on the content of a vcpkg.json manifest
#   that is injected into the project.
test-vcpkg-manifest-mode:
    FROM +vcpkg-base
    RUN apk add jq
    RUN jq -n ' { \
            name: "test-app", \
            version: "1.2.3", \
            dependencies: ["mongo-c-driver"], \
        }' > vcpkg.json && \
        cat vcpkg.json
    RUN rm -rf _build && \
        make test-manifest-mode

vcpkg-base:
    FROM alpine:3.18
    RUN apk add cmake curl gcc g++ musl-dev ninja-is-really-ninja zip unzip tar \
                build-base git pkgconf perl bash linux-headers
    ENV VCPKG_ROOT=/opt/vcpkg-git
    ENV VCPKG_FORCE_SYSTEM_BINARIES=1
    GIT CLONE --branch=2023.06.20 https://github.com/microsoft/vcpkg $VCPKG_ROOT
    RUN sh $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics && \
        install -spD -m 755 $VCPKG_ROOT/vcpkg /usr/local/bin/
    LET src_dir=/opt/mongoc-vcpkg-example
    COPY src/libmongoc/examples/cmake/vcpkg/ $src_dir
    WORKDIR $src_dir

# run :
#   Run one or more targets simultaneously.
#
# The “--targets” argument should be a single-string space-separated list of
# target names (not including a leading ‘+’) identifying targets to mark for
# execution. Targets will be executed concurrently. Other build arguments
# will be forwarded to the executed targets.
run:
    LOCALLY
    ARG --required targets
    FOR __target IN $targets
        BUILD +$__target
    END
