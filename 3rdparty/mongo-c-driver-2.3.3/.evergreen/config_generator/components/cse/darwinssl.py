from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_task import EvgTaskRef

from config_generator.etc.compile import generate_compile_tasks
from config_generator.etc.cse.compile import CompileCommon
from config_generator.etc.cse.test import generate_test_tasks

SSL = 'darwinssl'
TAG = f'cse-matrix-{SSL}'


# pylint: disable=line-too-long
# fmt: off
COMPILE_MATRIX = [
    ('macos-14-arm64', 'clang', None, ['cyrus']),
    ('macos-14',       'clang', None, ['cyrus']),
]

# QE (subset of CSFLE) requires 7.0+ and are skipped by "server" tasks.
TEST_MATRIX = [
    # Prefer macos-14-arm64 which is less resource-limited than macos-14. Provides 6.0+.
    ('macos-14-arm64', 'clang', None, 'cyrus', ['auth'], ['server', 'replica', 'sharded'], ['6.0', '7.0', '8.0', 'rapid', 'latest']),

    # Pre-6.0 coverage. Resource-limited: use sparingly.
    ('macos-14', 'clang', None, 'cyrus', ['auth'], ['sharded'], ['4.2', '4.4', '5.0']),
]
# fmt: on
# pylint: enable=line-too-long


class DarwinSSLCompileCommon(CompileCommon):
    ssl = 'DARWIN'


class SaslCyrusDarwinSSLCompile(DarwinSSLCompileCommon):
    name = 'cse-sasl-cyrus-darwinssl-compile'
    commands = DarwinSSLCompileCommon.compile_commands(sasl='CYRUS')


def functions():
    return SaslCyrusDarwinSSLCompile.defn()


def tasks():
    res = []

    SASL_TO_FUNC = {
        'cyrus': SaslCyrusDarwinSSLCompile,
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
