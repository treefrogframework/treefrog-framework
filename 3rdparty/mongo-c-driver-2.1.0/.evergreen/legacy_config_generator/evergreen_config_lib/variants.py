# Copyright 2009-present MongoDB, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from collections import OrderedDict as OD

from evergreen_config_generator.variants import Variant


mobile_flags = (
    " -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY"
    " -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY"
    " -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER"
    " -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY"
)


def days(n: int) -> int:
    "Calculate the number of minutes in the given number of days"
    return n * 24 * 60


all_variants = [
    Variant(
        "abi-compliance-check",
        "ABI Compliance Check",
        ["ubuntu2004-small", "ubuntu2004-medium", "ubuntu2004-large"],
        ["abi-compliance-check"],
    ),
    Variant(
        "smoke",
        "Smoke Tests",
        "ubuntu2204-small",
        [
            "make-docs",
            "kms-divergence-check",
            "release-compile",
            "debug-compile-no-counters",
            "compile-tracing",
            "link-with-cmake",
            "link-with-cmake-ssl",
            "link-with-cmake-snappy",
            "verify-headers",
            OD([("name", "link-with-cmake-mac"), ("distros", ["macos-14-arm64"])]),
            OD([("name", "link-with-cmake-windows"), ("distros", ["windows-vsCurrent-large"])]),
            OD([("name", "link-with-cmake-windows-ssl"), ("distros", ["windows-vsCurrent-large"])]),
            OD([("name", "link-with-cmake-windows-snappy"), ("distros", ["windows-vsCurrent-large"])]),
            OD([("name", "link-with-cmake-mingw"), ("distros", ["windows-vsCurrent-large"])]),
            OD([("name", "link-with-pkg-config"), ("distros", ["ubuntu2004-test"])]),
            OD([("name", "link-with-pkg-config-mac"), ("distros", ["macos-14-arm64"])]),
            "link-with-pkg-config-ssl",
            "link-with-bson",
            OD([("name", "link-with-bson-windows"), ("distros", ["windows-vsCurrent-large"])]),
            OD([("name", "link-with-bson-mac"), ("distros", ["macos-14-arm64"])]),
            OD([("name", "link-with-bson-mingw"), ("distros", ["windows-vsCurrent-large"])]),
            "check-headers",
            "debug-compile-with-warnings",
            OD([("name", "build-and-test-with-toolchain"), ("distros", ["debian11-small"])]),
            "install-libmongoc-after-libbson",
        ],
        {
            # Disable the MongoDB legacy shell download, which is not available in 5.0 for u22
            "SKIP_LEGACY_SHELL": "1"
        },
        tags=["pr-merge-gate"],
    ),
    Variant(
        "openssl",
        "OpenSSL",
        "archlinux-build",
        [
            "build-and-run-authentication-tests-openssl-1.0.1",
            "build-and-run-authentication-tests-openssl-1.0.2",
            "build-and-run-authentication-tests-openssl-1.1.0",
            "build-and-run-authentication-tests-openssl-1.0.1-fips",
        ],
        {},
    ),
    Variant(
        "clang37",
        "clang 3.7 (Archlinux)",
        "archlinux-test",
        [
            "release-compile",
            "debug-compile-sasl-openssl",
            "debug-compile-nosasl-openssl",
            ".authentication-tests .openssl",
        ],
        {"CC": "clang"},
    ),
    Variant(
        "clang100-i686",
        "clang 10.0 (i686) (Ubuntu 20.04)",
        "ubuntu2004-test",
        [
            "release-compile",
            "debug-compile-nosasl-nossl",
            ".debug-compile !.sspi .nossl .nosasl",
            ".latest .nossl .nosasl",
        ],
        {"CC": "clang", "MARCH": "i686"},
    ),
    Variant(
        "gcc82rhel",
        "GCC 8.2 (RHEL 8.0)",
        "rhel80-test",
        [
            ".hardened",
            ".compression !.snappy !.zstd",
            "release-compile",
            "debug-compile-nosasl-nossl",
            "debug-compile-nosasl-openssl",
            "debug-compile-sasl-openssl",
            ".authentication-tests .openssl",
            ".latest .nossl",
        ],
        {"CC": "gcc"},
    ),
    Variant(
        "gcc102",
        "GCC 10.2 (Debian 11.0)",
        "debian11-large",
        ["release-compile", "debug-compile-nosasl-nossl", ".latest .nossl"],
        {"CC": "gcc"},
    ),
    Variant(
        "gcc94-i686",
        "GCC 9.4 (i686) (Ubuntu 20.04)",
        "ubuntu2004-test",
        ["release-compile", "debug-compile-nosasl-nossl", ".latest .nossl .nosasl"],
        {"CC": "gcc", "MARCH": "i686"},
    ),
    Variant(
        "gcc94",
        "GCC 9.4 (Ubuntu 20.04)",
        "ubuntu2004-test",
        [
            ".compression !.zstd",
            "debug-compile-nosrv",
            "release-compile",
            "debug-compile-nosasl-nossl",
            "debug-compile-sasl-openssl",
            "debug-compile-nosasl-openssl",
            ".authentication-tests .openssl",
            ".authentication-tests .asan",
            ".test-coverage",
            ".latest .nossl",
            "retry-true-latest-server",
            "test-dns-openssl",
            "test-dns-auth-openssl",
            "test-dns-loadbalanced-openssl",
        ],
        {"CC": "gcc"},
    ),
    Variant(
        "darwin",
        "*Darwin, macOS (Apple LLVM)",
        "macos-14-arm64",
        [
            ".compression !.snappy",
            "release-compile",
            "debug-compile-nosasl-nossl",
            "debug-compile-nosrv",
            "debug-compile-sasl-darwinssl",
            "debug-compile-nosasl-nossl",
            ".authentication-tests .darwinssl",
            ".latest .nossl",
            "test-dns-darwinssl",
            "test-dns-auth-darwinssl",
            "debug-compile-lto",
            "debug-compile-lto-thin",
            "debug-compile-aws",
            "test-aws-openssl-regular-latest",
        ],
        {"CC": "clang"},
    ),
    Variant(
        "darwin-intel",
        "*Darwin, Intel macOS (Apple LLVM)",
        "macos-14",
        [
            "debug-compile-aws",
            "debug-compile-rdtscp",
            "test-aws-openssl-regular-4.4",
        ],
        {"CC": "clang"},
    ),
    Variant(
        "windows-2017-32",
        "Windows (i686) (VS 2017)",
        "windows-vsCurrent-large",
        ["debug-compile-nosasl-nossl", ".latest .nossl .nosasl"],
        {"CC": "Visual Studio 15 2017"},
    ),
    Variant(
        "windows-2017",
        "Windows (VS 2017)",
        "windows-vsCurrent-large",
        [
            "release-compile",
            "debug-compile-nosasl-nossl",
            "debug-compile-nosasl-openssl",
            "debug-compile-sspi-winssl",
            "debug-compile-nosrv",
            ".latest .nossl",
            ".nosasl .latest .nossl",
            ".compression !.snappy !.zstd !.latest",
            "test-dns-winssl",
            "test-dns-auth-winssl",
            "debug-compile-aws",
            "test-aws-openssl-regular-4.4",
            "test-aws-openssl-regular-latest",
            # Authentication tests with OpenSSL on Windows are only run on the vs2017 variant.
            # Older vs variants fail to verify certificates against Atlas tests.
            ".authentication-tests .openssl !.sasl",
            ".authentication-tests .winssl",
        ],
        {"CC": "Visual Studio 15 2017 Win64"},
    ),
    Variant(
        "mingw-windows2016",
        "MinGW-W64 (Windows Server 2016)",
        "windows-vsCurrent-large",
        ["debug-compile-nosasl-nossl", ".latest .nossl .nosasl .server"],
        {"CC": "mingw"},
    ),
    Variant(
        "rhel8-power",
        "Power (ppc64le) (RHEL 8)",
        "rhel8-power-large",
        [
            "release-compile",
            "debug-compile-nosasl-nossl",
            "debug-compile-sasl-openssl",
            ".latest .nossl",
            "test-dns-openssl",
        ],
        {"CC": "gcc"},
        patchable=False,
        batchtime=days(1),
    ),
    Variant(
        "arm-ubuntu2004",
        "*ARM (aarch64) (Ubuntu 20.04)",
        "ubuntu2004-arm64-large",
        [
            ".compression !.snappy !.zstd",
            "release-compile",
            "debug-compile-nosasl-nossl",
            "debug-compile-nosasl-openssl",
            "debug-compile-sasl-openssl",
            ".authentication-tests .openssl",
            ".latest .nossl",
            "test-dns-openssl",
        ],
        {"CC": "gcc"},
        batchtime=days(1),
    ),
    Variant(
        "zseries-rhel8",
        "*zSeries",
        "rhel8-zseries-large",
        [
            "release-compile",
            #      '.compression', --> TODO: waiting on ticket CDRIVER-3258
            "debug-compile-nosasl-nossl",
            "debug-compile-nosasl-openssl",
            "debug-compile-sasl-openssl",
            ".authentication-tests .openssl",
            ".latest .nossl",
        ],
        {"CC": "gcc"},
        patchable=False,
        batchtime=days(1),
    ),
    Variant(
        "clang100ubuntu",
        "clang 10.0 (Ubuntu 20.04)",
        "ubuntu2004-test",
        [
            "debug-compile-sasl-openssl-static",
            ".authentication-tests .asan",
        ],
        {"CC": "clang"},
    ),
    # Run AWS tests for MongoDB 4.4 and 5.0 on Ubuntu 20.04. AWS setup scripts
    # expect Ubuntu 20.04+. MongoDB 4.4 and 5.0 are not available on 22.04.
    Variant(
        "aws-ubuntu2004",
        "AWS Tests (Ubuntu 20.04)",
        "ubuntu2004-small",
        [
            "debug-compile-aws",
            ".test-aws .4.4",
            ".test-aws .5.0",
        ],
        {"CC": "clang"},
    ),
    Variant(
        "aws-ubuntu2204",
        "AWS Tests (Ubuntu 22.04)",
        "ubuntu2004-small",
        [
            "debug-compile-aws",
            ".test-aws .6.0",
            ".test-aws .7.0",
            ".test-aws .8.0",
            ".test-aws .latest",
        ],
        {"CC": "clang"},
    ),
    Variant("mongohouse", "Mongohouse Test", "ubuntu2204-small", ["debug-compile-sasl-openssl", "test-mongohouse"], {}),
    Variant(
        "ocsp",
        "OCSP tests",
        "ubuntu2004-small",
        [
            OD([("name", "debug-compile-nosasl-openssl")]),
            OD([("name", "debug-compile-nosasl-openssl-static")]),
            OD([("name", "debug-compile-nosasl-darwinssl"), ("distros", ["macos-14-arm64"])]),
            OD([("name", "debug-compile-nosasl-winssl"), ("distros", ["windows-vsCurrent-large"])]),
            OD([("name", ".ocsp-openssl")]),
            OD([("name", ".ocsp-darwinssl"), ("distros", ["macos-14-arm64"])]),
            OD([("name", ".ocsp-winssl"), ("distros", ["windows-vsCurrent-large"])]),
            OD([("name", "debug-compile-nosasl-openssl-1.0.1")]),
            OD([("name", ".ocsp-openssl-1.0.1")]),
        ],
        {},
        batchtime=days(7),
        display_tasks=[
            {
                "name": "ocsp-openssl",
                "execution_tasks": [".ocsp-openssl"],
            },
            {
                "name": "ocsp-darwinssl",
                "execution_tasks": [".ocsp-darwinssl"],
            },
            {
                "name": "ocsp-winssl",
                "execution_tasks": [".ocsp-winssl"],
            },
            {
                "name": "ocsp-openssl-1.0.1",
                "execution_tasks": [".ocsp-openssl-1.0.1"],
            },
        ],
    ),
    Variant(
        "packaging",
        "Linux Distro Packaging",
        "debian12-latest-small",
        [
            "debian-package-build",
            OD([("name", "rpm-package-build"), ("distros", ["rhel90-arm64-small"])]),
        ],
        {},
        tags=["pr-merge-gate"],
    ),
    # Test 7.0+ with Ubuntu 20.04+ since MongoDB 7.0 no longer ships binaries for Ubuntu 18.04.
    Variant(
        "versioned-api-ubuntu2004",
        "Versioned API Tests (Ubuntu 20.04)",
        "ubuntu2004-test",
        [
            "debug-compile-nosasl-openssl",
            "debug-compile-nosasl-nossl",
            ".versioned-api .5.0",
            ".versioned-api .6.0",
            ".versioned-api .7.0",
            ".versioned-api .8.0",
        ],
        {},
    ),
]
