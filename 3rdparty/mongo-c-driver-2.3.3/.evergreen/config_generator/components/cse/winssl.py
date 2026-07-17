from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_task import EvgTaskRef

from config_generator.etc.compile import generate_compile_tasks
from config_generator.etc.cse.compile import CompileCommon
from config_generator.etc.cse.test import generate_test_tasks

SSL = 'winssl'
TAG = f'cse-matrix-{SSL}'


# pylint: disable=line-too-long
# fmt: off
COMPILE_MATRIX = [
    ('windows-vsCurrent',   'vs2022x64', None, ['sspi']),
    ('windows-2022-latest', 'vs2022x64', None, ['sspi']),
]

# QE (subset of CSFLE) requires 7.0+ and are skipped by "server" tasks.
TEST_MATRIX = [
    ('windows-vsCurrent',   'vs2022x64', None, 'sspi', ['auth'], ['server', 'replica', 'sharded'], ['4.2', '4.4', '5.0', '6.0', '7.0',                         ]),
    ('windows-2022-latest', 'vs2022x64', None, 'sspi', ['auth'], ['server', 'replica', 'sharded'], [                                   '8.0', 'rapid', 'latest']),
]
# fmt: on
# pylint: enable=line-too-long


class WinSSLCompileCommon(CompileCommon):
    ssl = 'WINDOWS'


class SaslSspiWinSSLCompile(WinSSLCompileCommon):
    name = 'cse-sasl-sspi-winssl-compile'
    commands = WinSSLCompileCommon.compile_commands(sasl='SSPI')


def functions():
    return SaslSspiWinSSLCompile.defn()


def tasks():
    res = []

    SASL_TO_FUNC = {
        'sspi': SaslSspiWinSSLCompile,
    }

    MORE_TAGS = ['cse']

    res += generate_compile_tasks(SSL, TAG, SASL_TO_FUNC, COMPILE_MATRIX, MORE_TAGS)

    res += generate_test_tasks(SSL, TAG, TEST_MATRIX)

    return res


def variants():
    expansions = {
        'CLIENT_SIDE_ENCRYPTION': 'on',
    }

    return [
        BuildVariant(
            name=TAG,
            display_name=TAG,
            tasks=[EvgTaskRef(name=f'.{TAG}')],
            expansions=expansions,
        ),
    ]
