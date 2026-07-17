#!/usr/bin/env bash

# This script prepares the Debian environment for the building of Debian packages.
# This script should be run with root privileges within the build environment
# (chroot or container)

set -euo pipefail

DEPENDENCIES=(
    # Build tools
    build-essential cmake pkgconf
    # First-party components
    libmongocrypt-dev
    # Third-party components
    libssl-dev libsnappy-dev libutf8proc-dev libzstd-dev zlib1g-dev libsasl2-dev
    # Documentation bits
    python3-sphinx python3-sphinx-design furo libjs-mathjax
    # Debian packaging
    quilt git-buildpackage fakeroot dpkg-dev debhelper python3-packaging
)

# Install package/build dependencies
apt-get update
apt-get -y install "${DEPENDENCIES[@]}"
