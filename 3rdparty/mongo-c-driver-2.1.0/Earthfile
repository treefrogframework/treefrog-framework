VERSION --arg-scope-and-set --pass-args --use-function-keyword 0.7
LOCALLY

# Allow setting the "default" container image registry to use for image short names (e.g. to Amazon ECR).
ARG --global default_search_registry=docker.io

IMPORT ./tools/ AS tools

# For target names, descriptions, and build parameters, run the "doc" Earthly subcommand.
# Example use: <earthly> +build --env=u22 --sasl=off --tls=OpenSSL --c_compiler=gcc

# COPY_SOURCE :
#   Copy source files required for the build into the specified "--into" directory
COPY_SOURCE:
    FUNCTION
    ARG --required into
    COPY --dir \
        build/ \
        CMakeLists.txt \
        COPYING \
        NEWS \
        README.rst \
        src/ \
        THIRD_PARTY_NOTICES \
        VERSION_CURRENT \
        "$into"

# CONFIGURE :
#   Configure the project in $source_dir into $build_dir with a common set of configuration options
CONFIGURE:
    FUNCTION
    ARG --required source_dir
    ARG --required build_dir
    ARG --required tls
    ARG --required sasl
    RUN cmake -S "$source_dir" -B "$build_dir" -G "Ninja Multi-Config" \
        -D ENABLE_MAINTAINER_FLAGS=ON \
        -D ENABLE_SHM_COUNTERS=ON \
        -D ENABLE_SASL=$(echo $sasl | __str upper) \
        -D ENABLE_SNAPPY=ON \
        -D ENABLE_SRV=ON \
        -D ENABLE_ZLIB=BUNDLED \
        -D ENABLE_SSL=$(echo $tls | __str upper) \
        -D ENABLE_COVERAGE=ON \
        -D ENABLE_DEBUG_ASSERTIONS=ON \
        -Werror

# build :
#   Build libmongoc and libbson using the specified environment.
#
# The --env argument specifies the build environment among the `+env.xyz` environment
# targets, using --purpose=build for the build environment. Refer to the target
# list for a list of available build environments.
build:
    # env is an argument
    ARG --required env
    FROM --pass-args +env.$env --purpose=build
    # The configuration to be built
    ARG config=RelWithDebInfo
    # The prefix at which to install the built result
    ARG install_prefix=/opt/mongo-c-driver
    # Build configuration parameters. Will be case-normalized for CMake usage.
    ARG --required sasl
    ARG --required tls
    LET source_dir=/opt/mongoc/source
    LET build_dir=/opt/mongoc/build
    DO +COPY_SOURCE --into=$source_dir
    ENV CCACHE_HOME=/root/.cache/ccache
    DO --pass-args +CONFIGURE --source_dir=$source_dir --build_dir=$build_dir
    RUN --mount=type=cache,target=$CCACHE_HOME \
        env CCACHE_BASE="$source_dir" \
            cmake --build $build_dir --config $config
    RUN cmake --install $build_dir --prefix="$install_prefix" --config $config
    SAVE ARTIFACT /opt/mongoc/build/* /build-tree/
    SAVE ARTIFACT /opt/mongo-c-driver/* /root/

# test-example will build one of the libmongoc example projects using the build
# that comes from the +build target. Arguments for +build should also be provided
test-example:
    ARG --required env
    FROM --pass-args +env.$env --purpose=build
    # Grab the built library
    COPY --pass-args +build/root /opt/mongo-c-driver
    # Add the example files
    COPY --dir \
        src/libmongoc/examples/cmake \
        src/libmongoc/examples/hello_mongoc.c \
        /opt/mongoc-test/
    # Configure and build it
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
# file for the cxx-driver build. The default value is “0.0.0”
#
# Arguments for +build should be provided.
test-cxx-driver:
    ARG --required env
    ARG --required test_mongocxx_ref
    FROM --pass-args +env.$env --purpose=build
    ARG cxx_compiler
    IF test "$cxx_compiler" = ""
        # No cxx_compiler is set, so infer based on a possible c_compiler option
        ARG c_compiler
        IF test "$c_compiler" != ""
            # ADD_CXX_COMPILER will remap the C compiler name to an appropriate C++ name
            LET cxx_compiler="$c_compiler"
        ELSE
            LET cxx_compiler =  gcc
        END
    END
    ARG cxx_version_current=0.0.0
    DO tools+ADD_CXX_COMPILER --cxx_compiler=$cxx_compiler
    COPY --pass-args +build/root /opt/mongo-c-driver
    LET source=/opt/mongo-cxx-driver/src
    LET build=/opt/mongo-cxx-driver/bld
    GIT CLONE --branch=$test_mongocxx_ref https://github.com/mongodb/mongo-cxx-driver.git $source
    RUN echo $cxx_version_current > $source/build/VERSION_CURRENT
    RUN cmake -S $source -B $build -G Ninja -D CMAKE_PREFIX_PATH=/opt/mongo-c-driver -D CMAKE_CXX_STANDARD=17
    ENV CCACHE_HOME=/root/.cache/ccache
    ENV CCACHE_BASE=$source
    RUN --mount=type=cache,target=$CCACHE_HOME cmake --build $build

# PREP_CMAKE "warms up" the CMake installation cache for the current environment
PREP_CMAKE:
    FUNCTION
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
    RUN __alias cmake /opt/mongoc-cmake/.evergreen/scripts/cmake.sh
    # Executing any CMake command will warm the cache:
    RUN cmake --version

env-warmup:
    ARG --required env
    BUILD --pass-args +env.$env --purpose=build
    BUILD --pass-args +env.$env --purpose=test

# Simultaneously builds and tests multiple different platforms
multibuild:
    BUILD +run --targets "test-example" \
        --env=alpine3.16 --env=alpine3.17 --env=alpine3.18 --env=alpine3.19 \
        --env=u16 --env=u18 --env=u20 --env=u22 --env=centos7 \
        --env=archlinux \
        --tls=OpenSSL --tls=off \
        --sasl=Cyrus --sasl=off \
        --c_compiler=gcc --c_compiler=clang \
        --test_mongocxx_ref=master

# release-archive :
#   Create a release archive of the source tree. (Refer to dev docs)
release-archive:
    FROM $default_search_registry/library/alpine:3.20
    RUN apk add git bash
    ARG --required prefix
    ARG --required ref

    WORKDIR /s
    COPY --dir .git .

    # Get the commit hash that we are archiving. Use ^{commit} to "dereference" tag objects
    LET revision = $(git rev-parse "$ref^{commit}")
    RUN git restore --quiet --source=$revision -- VERSION_CURRENT
    LET version = $(cat VERSION_CURRENT)

    # Pick the waterfall project based on the tag
    COPY tools+tools-dir/__str /usr/local/bin/__str
    IF __str test "$version" -matches ".*\.0\$"
        # This is a minor release. Link to the build on the main project.
        LET base = "mongo_c_driver"
    ELSE
        # This is (probably) a patch release. Link to the build on the release branch.
        LET base = "mongo_c_driver_latest_release"
    END

    # The augmented SBOM must be manually obtained from a recent execution of
    # the `sbom` task in an Evergreen patch or commit build in advance.
    COPY etc/augmented-sbom.json cyclonedx.sbom.json

    # The full link to the build for this commit
    LET waterfall_url = "https://spruce.mongodb.com/version/${base}_${revision}"
    # Insert the URL into the SSDLC report
    COPY etc/ssdlc.md ssdlc_compliance_report.md
    RUN sed -i "
        s|@waterfall_url@|$waterfall_url|g
        s|@version@|$version|g
    " ssdlc_compliance_report.md
    # Generate the archive
    RUN git archive -o release.tar.gz \
        --prefix="$prefix/" \ # Set the archive path prefix
        "$revision" \ # Add the source tree
        --add-file cyclonedx.sbom.json \ # Add the SBOM
        --add-file ssdlc_compliance_report.md
    SAVE ARTIFACT release.tar.gz

# Obtain the signing public key. Exported as an artifact /c-driver.pub
signing-pubkey:
    FROM $default_search_registry/library/alpine:3.20
    RUN apk add curl
    RUN curl --location --silent --fail "https://pgp.mongodb.com/c-driver.pub" -o /c-driver.pub
    SAVE ARTIFACT /c-driver.pub

# sign-file :
#   Sign an arbitrary file. This uses internal MongoDB tools and requires authentication
#   to be used to access them. (Refer to dev docs)
sign-file:
    # Pull from Garasign:
    FROM 901841024863.dkr.ecr.us-east-1.amazonaws.com/release-infrastructure/garasign-gpg
    # Copy the file to be signed
    ARG --required file
    COPY $file /s/file
    # Run the GPG signing command. Requires secrets!
    RUN --secret GRS_CONFIG_USER1_USERNAME --secret GRS_CONFIG_USER1_PASSWORD \
        gpgloader && \
        gpg --yes --verbose --armor --detach-sign --output=/s/signature.asc /s/file
    # Export the detatched signature
    SAVE ARTIFACT /s/signature.asc /
    # Verify the file signature against the public key
    COPY +signing-pubkey/c-driver.pub /s/
    RUN touch /keyring && \
        gpg --no-default-keyring --keyring /keyring --import /s/c-driver.pub && \
        gpgv --keyring=/keyring /s/signature.asc /s/file

# signed-release :
#   Generate a signed release artifact. Refer to the "Earthly" page of our dev docs for more information.
#   (Refer to dev docs)
signed-release:
    FROM $default_search_registry/library/alpine:3.20
    RUN apk add git
    # The version of the release. This affects the filepaths of the output and is the default for --ref
    ARG --required version
    # The Git revision of the repository to be archived. By default, archives the tag of the given version
    ARG ref=refs/tags/$version
    # File stem and archive prefix:
    LET stem="mongo-c-driver-$version"
    WORKDIR /s
    # Run the commands "locally" so that the files can be transferred between the
    # targets via the host filesystem.
    LOCALLY
    # Clean out a scratch space for us to work with
    LET rel_dir = ".scratch/release"
    RUN rm -rf -- "$rel_dir"
    # Primary artifact files
    LET rel_tgz = "$rel_dir/$stem.tar.gz"
    LET rel_asc = "$rel_dir/$stem.tar.gz.asc"
    # Make the release archive:
    COPY (+release-archive/ --prefix=$stem --ref=$ref) $rel_dir/
    RUN mv $rel_dir/release.tar.gz $rel_tgz
    # Sign the release archive:
    COPY (+sign-file/signature.asc --file $rel_tgz) $rel_asc
    # Save them as an artifact.
    SAVE ARTIFACT $rel_dir /dist
    # Remove our scratch space from the host. Getting at the artifacts requires `earthly --artifact`
    RUN rm -rf -- "$rel_dir"

# This target is simply an environment in which the SilkBomb executable is available.
silkbomb:
    FROM 901841024863.dkr.ecr.us-east-1.amazonaws.com/release-infrastructure/silkbomb:2.0
    # Alias the silkbomb executable to a simpler name:
    RUN ln -s /python/src/sbom/silkbomb/bin /usr/local/bin/silkbomb

# sbom-generate :
#   Generate/update the etc/cyclonedx.sbom.json file from the etc/purls.txt file.
#
# This target will update the existing etc/cyclonedx.sbom.json file in-place based
# on the content of etc/purls.txt and etc/cyclonedx.sbom.json.
sbom-generate:
    FROM +silkbomb
    # Copy in the relevant files:
    WORKDIR /s
    COPY etc/purls.txt etc/cyclonedx.sbom.json /s/
    # Update the SBOM file:
    RUN silkbomb update \
        --refresh \
        --no-update-sbom-version \
        --purls purls.txt \
        --sbom-in cyclonedx.sbom.json \
        --sbom-out cyclonedx.sbom.json
    # Save the result back to the host:
    SAVE ARTIFACT /s/cyclonedx.sbom.json AS LOCAL etc/cyclonedx.sbom.json

# sbom-generate-new-serial-number:
#   Equivalent to +sbom-generate but includes the --generate-new-serial-number
#   flag to generate a new unique serial number and reset the SBOM version to 1.
#
# This target will update the existing etc/cyclonedx.sbom.json file in-place based
# on the content of etc/purls.txt and etc/cyclonedx.sbom.json.
sbom-generate-new-serial-number:
    FROM +silkbomb
    # Copy in the relevant files:
    WORKDIR /s
    COPY etc/purls.txt etc/cyclonedx.sbom.json /s/
    # Update the SBOM file:
    RUN silkbomb update \
        --refresh \
        --generate-new-serial-number \
        --purls purls.txt \
        --sbom-in cyclonedx.sbom.json \
        --sbom-out cyclonedx.sbom.json
    # Save the result back to the host:
    SAVE ARTIFACT /s/cyclonedx.sbom.json AS LOCAL etc/cyclonedx.sbom.json

# sbom-validate:
#   Validate the SBOM Lite for the given branch.
sbom-validate:
    FROM +silkbomb
    # Copy in the relevant files:
    WORKDIR /s
    COPY etc/purls.txt etc/cyclonedx.sbom.json /s/
    # Run the SilkBomb tool to download the artifact that matches the requested branch
    RUN silkbomb validate \
            --purls purls.txt \
            --sbom-in cyclonedx.sbom.json \
            --exclude jira

snyk:
    FROM --platform=linux/amd64 $default_search_registry/library/ubuntu:24.04
    RUN apt-get update && apt-get -y install curl
    RUN curl --location https://github.com/snyk/cli/releases/download/v1.1291.1/snyk-linux -o /usr/local/bin/snyk
    RUN chmod a+x /usr/local/bin/snyk

snyk-test:
    FROM +snyk
    WORKDIR /s
    # Take the scan from within the `src/` directory. This seems to help Snyk
    # actually find the external dependencies that live there.
    COPY --dir src .
    WORKDIR src/
    # Snaptshot the repository and run the scan
    RUN --no-cache --secret SNYK_TOKEN \
        snyk test --unmanaged --json > snyk.json
    SAVE ARTIFACT snyk.json

# snyk-monitor-snapshot :
#   Post a crafted snapshot of the repository to Snyk for monitoring. Refer to "Snyk Scanning"
#   in the dev docs for more details.
snyk-monitor-snapshot:
    FROM +snyk
    WORKDIR /s
    ARG remote="https://github.com/mongodb/mongo-c-driver.git"
    ARG --required branch
    ARG --required name
    IF test "$remote" = "local"
        COPY --dir src .
    ELSE
        GIT CLONE --branch $branch $remote clone
        RUN mv clone/src .
    END
    # Take the scan from within the `src/` directory. This seems to help Snyk
    # actually find the external dependencies that live there.
    WORKDIR src/
    # Snaptshot the repository and run the scan
    RUN --no-cache --secret SNYK_TOKEN --secret SNYK_ORGANIZATION \
        snyk monitor \
            --org=$SNYK_ORGANIZATION \
            --target-reference=$name \
            --unmanaged \
            --print-deps \
            --project-name=mongo-c-driver \
            --remote-repo-url=https://github.com/mongodb/mongo-c-driver

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
    FROM $default_search_registry/library/alpine:3.18
    RUN apk add cmake curl gcc g++ musl-dev ninja-is-really-ninja zip unzip tar \
                build-base git pkgconf perl bash linux-headers
    ENV VCPKG_ROOT=/opt/vcpkg-git
    ENV VCPKG_FORCE_SYSTEM_BINARIES=1
    GIT CLONE --branch=2023.06.20 https://github.com/microsoft/vcpkg $VCPKG_ROOT
    RUN $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics && \
        install -spD -m 755 $VCPKG_ROOT/vcpkg /usr/local/bin/
    LET src_dir=/opt/mongoc-vcpkg-example
    COPY src/libmongoc/examples/cmake/vcpkg/ $src_dir
    WORKDIR $src_dir

# verify-headers :
#   Execute CMake header verification on the sources
#
#   See `earthly.rst` for more details.
verify-headers:
    # We test against multiple different platforms, because glibc/musl versions may
    # rearrange their header contents and requirements, so we want to check against as
    # many as possible.
    BUILD +do-verify-headers-impl \
        --from +env.alpine3.19 \
        --from +env.u22 \
        --from +env.centos7 \
        --sasl=off --tls=off --cxx_compiler=gcc --c_compiler=gcc

do-verify-headers-impl:
    ARG --required from
    # We don't really care about the specifics of the build env/settings, so set some
    # reasonable defaults so the caller doesn't need to specify. In the future, it is
    # possible that we will need to test other environments and build settings.
    FROM --pass-args "$from" --purpose=build
    # Add C++ so we can test as C++ headers
    DO --pass-args tools+ADD_CXX_COMPILER
    DO +COPY_SOURCE --into=/s
    DO --pass-args +CONFIGURE --source_dir /s --build_dir /s/_build
    # The "all_verify_interface_header_sets" target is created automatically
    # by CMake for the VERIFY_INTERFACE_HEADER_SETS target property.
    RUN cmake --build /s/_build --target all_verify_interface_header_sets

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


# d88888b d8b   db db    db d888888b d8888b.  .d88b.  d8b   db .88b  d88. d88888b d8b   db d888888b .d8888.
# 88'     888o  88 88    88   `88'   88  `8D .8P  Y8. 888o  88 88'YbdP`88 88'     888o  88 `~~88~~' 88'  YP
# 88ooooo 88V8o 88 Y8    8P    88    88oobY' 88    88 88V8o 88 88  88  88 88ooooo 88V8o 88    88    `8bo.
# 88~~~~~ 88 V8o88 `8b  d8'    88    88`8b   88    88 88 V8o88 88  88  88 88~~~~~ 88 V8o88    88      `Y8b.
# 88.     88  V888  `8bd8'    .88.   88 `88. `8b  d8' 88  V888 88  88  88 88.     88  V888    88    db   8D
# Y88888P VP   V8P    YP    Y888888P 88   YD  `Y88P'  VP   V8P YP  YP  YP Y88888P VP   V8P    YP    `8888Y'

env.u16:
    DO --pass-args +UBUNTU_ENV --version=16.04

env.u18:
    DO --pass-args +UBUNTU_ENV --version=18.04

env.u20:
    DO --pass-args +UBUNTU_ENV --version=20.04

env.u22:
    DO --pass-args +UBUNTU_ENV --version=22.04

env.alpine3.16:
    DO --pass-args +ALPINE_ENV --version=3.16

env.alpine3.17:
    DO --pass-args +ALPINE_ENV --version=3.17

env.alpine3.18:
    DO --pass-args +ALPINE_ENV --version=3.18

env.alpine3.19:
    DO --pass-args +ALPINE_ENV --version=3.19

env.archlinux:
    FROM --pass-args tools+init-env --from $default_search_registry/library/archlinux
    RUN pacman-key --init
    ARG --required purpose

    RUN __install ninja snappy

    IF test "$purpose" = build
        RUN __install ccache
    END

    # We don't install SASL here, because it's pre-installed on Arch
    DO --pass-args tools+ADD_TLS
    DO --pass-args tools+ADD_C_COMPILER
    DO +PREP_CMAKE

env.centos7:
    DO --pass-args +CENTOS_ENV --version=7

ALPINE_ENV:
    FUNCTION
    ARG --required version
    FROM --pass-args tools+init-env --from $default_search_registry/library/alpine:$version
    # XXX: On Alpine, we just use the system's CMake. At time of writing, it is
    # very up-to-date and much faster than building our own from source (since
    # Kitware does not (yet) provide libmuslc builds of CMake)
    RUN __install bash curl cmake ninja musl-dev make
    ARG --required purpose

    IF test "$purpose" = "build"
        RUN __install snappy-dev ccache
    ELSE IF test "$purpose" = "test"
        RUN __install snappy
    END

    DO --pass-args tools+ADD_SASL
    DO --pass-args tools+ADD_TLS
    # Add "gcc" when installing Clang, since it pulls in a lot of runtime libraries and
    # utils that are needed for linking with Clang
    DO --pass-args tools+ADD_C_COMPILER --clang_pkg="gcc clang compiler-rt"

UBUNTU_ENV:
    FUNCTION
    ARG --required version
    FROM --pass-args tools+init-env --from $default_search_registry/library/ubuntu:$version
    RUN __install curl build-essential
    ARG --required purpose

    IF test "$purpose" = build
        RUN __install ninja-build gcc ccache libsnappy-dev zlib1g-dev
    ELSE IF test "$purpose" = test
        RUN __install libsnappy1v5 ninja-build
    END

    DO --pass-args tools+ADD_SASL
    DO --pass-args tools+ADD_TLS
    DO --pass-args tools+ADD_C_COMPILER
    DO +PREP_CMAKE

CENTOS_ENV:
    FUNCTION
    ARG --required version
    FROM --pass-args tools+init-env --from $default_search_registry/library/centos:$version
    # Update repositories to use vault.centos.org
    RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-* && \
        sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*
    RUN yum -y --enablerepo=extras install epel-release && yum -y update
    RUN yum -y install curl gcc gcc-c++ make
    ARG --required purpose

    IF test "$purpose" = build
        RUN yum -y install ninja-build ccache snappy-devel zlib-devel
    ELSE IF test "$purpose" = test
        RUN yum -y install ninja-build snappy
    END

    DO --pass-args tools+ADD_SASL --cyrus_dev_pkg="cyrus-sasl-devel" --cyrus_rt_pkg="cyrus-sasl-lib"
    DO --pass-args tools+ADD_TLS --openssl_dev_pkg="openssl-devel" --openssl_rt_pkg="openssl-libs"
    DO --pass-args tools+ADD_C_COMPILER
    DO +PREP_CMAKE
