from shrub.v3.evg_build_variant import BuildVariant

from config_generator.etc.utils import TaskRef
from config_generator.etc.function import merge_defns
from config_generator.etc.compile import generate_compile_tasks

from config_generator.etc.sasl.compile import CompileCommon
from config_generator.etc.sasl.test import generate_test_tasks


SSL = 'openssl'
TAG = f'sasl-matrix-{SSL}'


# pylint: disable=line-too-long
# fmt: off
COMPILE_MATRIX = [
    # For test matrix.
    ('amazon2023-arm64-latest-large-m8g', 'gcc', None, ['cyrus']),
    ('rhel8-latest',      'gcc',        None, ['cyrus']),
    ('rhel8-power',       'gcc',        None, ['cyrus']),
    ('rhel8-zseries',     'gcc',        None, ['cyrus']),
    ('ubuntu2004-arm64',  'gcc',        None, ['cyrus']),
    ('windows-vsCurrent', 'vs2017x64',  None, ['cyrus']),

    # For compile only.
    ('debian11',   'gcc',      None, ['cyrus']),
    ('debian12',   'gcc',      None, ['cyrus']),
    ('rhel80',     'gcc',      None, ['cyrus']),
    ('ubuntu2004', 'clang',    None, ['cyrus']),
    ('ubuntu2204', 'gcc',      None, ['cyrus']),
    ('ubuntu2204', 'clang-12', None, ['cyrus']),
    ('ubuntu2404', 'gcc',      None, ['cyrus']),
    ('ubuntu2404', 'clang-14', None, ['cyrus']),
]

TEST_MATRIX = [
    ('rhel8-latest',  'gcc', None, 'cyrus', ['auth'], ['server'], ['4.2', '4.4', '5.0', '6.0', '7.0', '8.0', 'latest']),
    ('rhel8-power',   'gcc', None, 'cyrus', ['auth'], ['server'], ['4.2', '4.4', '5.0', '6.0', '7.0', '8.0', 'latest']),
    ('rhel8-zseries', 'gcc', None, 'cyrus', ['auth'], ['server'], [              '5.0', '6.0', '7.0', '8.0', 'latest']),

    ('ubuntu2004-arm64',  'gcc',       None, 'cyrus', ['auth'], ['server'], ['4.4', '5.0', '6.0', '7.0', '8.0', 'latest']),
    ('windows-vsCurrent', 'vs2017x64', None, 'cyrus', ['auth'], ['server'], [       'latest']),

    # Test with Graviton processor:
    ('amazon2023-arm64-latest-large-m8g', 'gcc',  None, 'cyrus', ['auth'], ['server', 'replica', 'sharded'], ['latest']),
]
# fmt: on
# pylint: enable=line-too-long


class OpenSSLCompileCommon(CompileCommon):
    ssl = 'OPENSSL'


class SaslCyrusOpenSSLCompile(OpenSSLCompileCommon):
    name = 'sasl-cyrus-openssl-compile'
    commands = OpenSSLCompileCommon.compile_commands(sasl='CYRUS')


def functions():
    return merge_defns(
        SaslCyrusOpenSSLCompile.defn(),
    )


SASL_TO_FUNC = {
    'cyrus': SaslCyrusOpenSSLCompile,
}

TASKS = [
    *generate_compile_tasks(SSL, TAG, SASL_TO_FUNC, COMPILE_MATRIX),
    *generate_test_tasks(SSL, TAG, TEST_MATRIX),
]


def tasks():
    res = TASKS.copy()

    # PowerPC and zSeries are limited resources.
    for task in res:
        if any(pattern in task.run_on for pattern in ["power", "zseries"]):
            task.patchable = False

    return res


def variants():
    expansions = {}

    tasks = []

    # PowerPC and zSeries are limited resources.
    for task in TASKS:
        if any(pattern in task.run_on for pattern in ["power", "zseries"]):
            tasks.append(
                TaskRef(
                    name=task.name,
                    batchtime=1440,   # 1 day
                )
            )
        else:
            tasks.append(task.get_task_ref())

    tasks.sort(key=lambda t: t.name)

    return [
        BuildVariant(
            name=TAG,
            display_name=TAG,
            tasks=tasks,
            expansions=expansions,
        ),
    ]
