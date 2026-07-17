from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_task import EvgTaskRef

from config_generator.etc.compile import generate_compile_tasks
from config_generator.etc.function import merge_defns
from config_generator.etc.sasl.compile import CompileCommon
from config_generator.etc.sasl.test import generate_test_tasks

SSL = 'winssl'
TAG = f'sasl-matrix-{SSL}'


# pylint: disable=line-too-long
# fmt: off
COMPILE_MATRIX = [
    # For test matrix.
    ('windows-vsCurrent',   'mingw',     None, ['sspi']),
    ('windows-vsCurrent',   'vs2022x64', None, ['sspi']),
    ('windows-vsCurrent',   'vs2022x86', None, ['sspi']),
    ('windows-2022-latest', 'mingw',     None, ['sspi']),
    ('windows-2022-latest', 'vs2022x64', None, ['sspi']),
    ('windows-2022-latest', 'vs2022x86', None, ['sspi']),

    # For compile only.
    ('windows-vsCurrent', 'vs2017x64', None, ['off', 'sspi']),
    ('windows-vsCurrent', 'vs2019x64', None, ['off', 'sspi']),
]

TEST_MATRIX = [
    ('windows-vsCurrent',   'vs2022x64', None, 'sspi', ['auth'], ['server', 'replica', 'sharded'], ['4.2', '4.4', '5.0', '6.0', '7.0',                         ]),
    ('windows-2022-latest', 'vs2022x64', None, 'sspi', ['auth'], ['server', 'replica', 'sharded'], [                                   '8.0', 'rapid', 'latest']),

    # sharded + min only.
    ('windows-vsCurrent',   'mingw',     None, 'sspi',  ['auth'], ['sharded'], ['4.2']),
    ('windows-vsCurrent',   'vs2022x86', None, 'sspi',  ['auth'], ['sharded'], ['4.2']),

    # sharded + latest only.
    ('windows-2022-latest', 'mingw',     None, 'sspi',  ['auth'], ['sharded'], ['rapid', 'latest']),
    ('windows-2022-latest', 'vs2022x86', None, 'sspi',  ['auth'], ['sharded'], ['rapid', 'latest']),
]
# fmt: on
# pylint: enable=line-too-long


class WinSSLCompileCommon(CompileCommon):
    ssl = 'WINDOWS'


class SaslOffWinSSLCompile(WinSSLCompileCommon):
    name = 'sasl-off-winssl-compile'
    commands = WinSSLCompileCommon.compile_commands(sasl='OFF')


class SaslSspiWinSSLCompile(WinSSLCompileCommon):
    name = 'sasl-sspi-winssl-compile'
    commands = WinSSLCompileCommon.compile_commands(sasl='SSPI')


def functions():
    return merge_defns(
        SaslOffWinSSLCompile.defn(),
        SaslSspiWinSSLCompile.defn(),
    )


def tasks():
    res = []

    SASL_TO_FUNC = {
        'off': SaslOffWinSSLCompile,
        'sspi': SaslSspiWinSSLCompile,
    }

    res += generate_compile_tasks(SSL, TAG, SASL_TO_FUNC, COMPILE_MATRIX)
    res += generate_test_tasks(SSL, TAG, TEST_MATRIX)

    return res


def variants():
    expansions = {}

    return [
        BuildVariant(
            name=TAG,
            display_name=TAG,
            tasks=[EvgTaskRef(name=f'.{TAG}')],
            expansions=expansions,
        ),
    ]
