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

from evergreen_config_generator.functions import Function, s3_put

from evergreen_config_lib import shell_mongoc

build_path = '${build_variant}/${revision}/${version_id}/${build_id}'

all_functions = OD(
    [
        (
            'install ssl',
            Function(
                shell_mongoc(
                    '.evergreen/scripts/install-ssl.sh',
                    test=False,
                    add_expansions_to_env=True,
                ),
            ),
        ),
        (
            'upload coverage',
            Function(
                shell_mongoc(
                    r"""
                    export AWS_ACCESS_KEY_ID=${aws_key}
                    export AWS_SECRET_ACCESS_KEY=${aws_secret}
                    aws s3 cp coverage s3://mciuploads/${project}/%s/coverage/ --recursive --acl public-read --region us-east-1
                    """
                    % (build_path,),
                    test=False,
                    silent=True,
                ),
                s3_put(
                    build_path + '/coverage/index.html',
                    aws_key='${aws_key}',
                    aws_secret='${aws_secret}',
                    local_file='mongoc/coverage/index.html',
                    bucket='mciuploads',
                    permissions='public-read',
                    content_type='text/html',
                    display_name='Coverage Report',
                ),
            ),
        ),
        (
            'upload scan artifacts',
            Function(
                shell_mongoc(
                    r"""
                    if find scan -name \*.html | grep -q html; then
                      (cd scan && find . -name index.html -exec echo "<li><a href='{}'>{}</a></li>" \;) >> scan.html
                    else
                      echo "No issues found" > scan.html
                    fi
                    """,
                ),
                shell_mongoc(
                    r"""
                    export AWS_ACCESS_KEY_ID=${aws_key}
                    export AWS_SECRET_ACCESS_KEY=${aws_secret}
                    aws s3 cp scan s3://mciuploads/${project}/%s/scan/ --recursive --acl public-read --region us-east-1
                    """
                    % (build_path,),
                    test=False,
                    silent=True,
                ),
                s3_put(
                    build_path + '/scan/index.html',
                    aws_key='${aws_key}',
                    aws_secret='${aws_secret}',
                    local_file='mongoc/scan.html',
                    bucket='mciuploads',
                    permissions='public-read',
                    content_type='text/html',
                    display_name='Scan Build Report',
                ),
            ),
        ),
        # Use "silent=True" to hide output since errors may contain credentials.
        (
            'run auth tests',
            Function(
                shell_mongoc(
                    '.evergreen/scripts/run-auth-tests.sh',
                    add_expansions_to_env=True,
                ),
            ),
        ),
        (
            'link sample program',
            Function(
                shell_mongoc(
                    r"""
                    # Compile a program that links dynamically or statically to libmongoc,
                    # using variables from pkg-config or CMake's find_package command.
                    export BUILD_SAMPLE_WITH_CMAKE=${BUILD_SAMPLE_WITH_CMAKE}
                    export ENABLE_SSL=${ENABLE_SSL}
                    export ENABLE_SNAPPY=${ENABLE_SNAPPY}
                    LINK_STATIC=  .evergreen/scripts/link-sample-program.sh
                    LINK_STATIC=1 .evergreen/scripts/link-sample-program.sh
                    """,
                    include_expansions_in_env=['distro_id'],
                ),
            ),
        ),
        (
            'link sample program bson',
            Function(
                shell_mongoc(
                    r"""
                    # Compile a program that links dynamically or statically to libbson,
                    # using variables from pkg-config or from CMake's find_package command.
                    BUILD_SAMPLE_WITH_CMAKE=  LINK_STATIC=  .evergreen/scripts/link-sample-program-bson.sh
                    BUILD_SAMPLE_WITH_CMAKE=  LINK_STATIC=1 .evergreen/scripts/link-sample-program-bson.sh
                    BUILD_SAMPLE_WITH_CMAKE=1 LINK_STATIC=  .evergreen/scripts/link-sample-program-bson.sh
                    BUILD_SAMPLE_WITH_CMAKE=1 LINK_STATIC=1 .evergreen/scripts/link-sample-program-bson.sh
                    """,
                    include_expansions_in_env=['distro_id'],
                ),
            ),
        ),
        (
            'link sample program MSVC',
            Function(
                shell_mongoc(
                    r"""
                    # Build libmongoc with CMake and compile a program that links
                    # dynamically or statically to it, using variables from CMake's
                    # find_package command.
                    export ENABLE_SSL=${ENABLE_SSL}
                    export ENABLE_SNAPPY=${ENABLE_SNAPPY}
                    LINK_STATIC=  cmd.exe /c .\\.evergreen\\scripts\\link-sample-program-msvc.cmd
                    LINK_STATIC=1 cmd.exe /c .\\.evergreen\\scripts\\link-sample-program-msvc.cmd
                    """,
                )
            ),
        ),
        (
            'link sample program mingw',
            Function(
                shell_mongoc(
                    r"""
                    # Build libmongoc with CMake and compile a program that links
                    # dynamically to it, using variables from pkg-config.exe.
                    PATH="/cygdrive/c/ProgramData/chocolatey/lib/winlibs/tools/mingw64/bin:$PATH" # mingw-w64 GCC
                    cmd.exe /c .\\.evergreen\\scripts\\link-sample-program-mingw.cmd
                    """,
                )
            ),
        ),
        (
            'link sample program MSVC bson',
            Function(
                shell_mongoc(
                    r"""
                    # Build libmongoc with CMake and compile a program that links
                    # dynamically or statically to it, using variables from CMake's
                    # find_package command.
                    export ENABLE_SSL=${ENABLE_SSL}
                    export ENABLE_SNAPPY=${ENABLE_SNAPPY}
                    LINK_STATIC=  cmd.exe /c .\\.evergreen\\scripts\\link-sample-program-msvc-bson.cmd
                    LINK_STATIC=1 cmd.exe /c .\\.evergreen\\scripts\\link-sample-program-msvc-bson.cmd
                    """,
                )
            ),
        ),
        (
            'link sample program mingw bson',
            Function(
                shell_mongoc(
                    r"""
                    # Build libmongoc with CMake and compile a program that links
                    # dynamically to it, using variables from pkg-config.exe.
                    PATH="/cygdrive/c/ProgramData/chocolatey/lib/winlibs/tools/mingw64/bin:$PATH" # mingw-w64 GCC
                    cmd.exe /c .\\.evergreen\\scripts\\link-sample-program-mingw-bson.cmd
                    """,
                )
            ),
        ),
        (
            'update codecov.io',
            Function(
                shell_mongoc(
                    r"""
                    # Note: coverage is currently only enabled on the ubuntu1804 distro.
                    # This script does not support MacOS, Windows, or non-x86_64 distros.
                    # Update accordingly if code coverage is expanded to other distros.
                    curl -Os https://uploader.codecov.io/latest/linux/codecov
                    chmod +x codecov
                    # -Z: Exit with a non-zero value if error.
                    # -g: Run with gcov support.
                    # -t: Codecov upload token.
                    # perl: filter verbose "Found" list and "Processing" messages.
                    ./codecov -Zgt "${codecov_token}" | perl -lne 'print if not m|^.*\.gcov(\.\.\.)?$|'
                    """,
                    test=False,
                ),
            ),
        ),
        (
            'compile coverage',
            Function(
                shell_mongoc(
                    'COVERAGE=ON .evergreen/scripts/compile.sh',
                    add_expansions_to_env=True,
                ),
            ),
        ),
        (
            'run aws tests',
            Function(
                # Assume role to get AWS secrets.
                {'command': 'ec2.assume_role', 'params': {'role_arn': '${aws_test_secrets_role}'}},
                shell_mongoc(
                    r"""
                    pushd ../drivers-evergreen-tools/.evergreen/auth_aws
                    ./setup_secrets.sh drivers/aws_auth
                    popd # ../drivers-evergreen-tools/.evergreen/auth_aws
                    """,
                    include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
                ),
                shell_mongoc(
                    r"""
                    pushd ../drivers-evergreen-tools/.evergreen/auth_aws
                    . ./activate-authawsvenv.sh
                    popd # ../drivers-evergreen-tools/.evergreen/auth_aws
                    .evergreen/scripts/run-aws-tests.sh
                    """,
                    include_expansions_in_env=['TESTCASE'],
                ),
            ),
        ),
    ]
)
