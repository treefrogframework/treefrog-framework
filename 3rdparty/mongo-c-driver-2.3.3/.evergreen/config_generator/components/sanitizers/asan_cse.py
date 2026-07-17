from config_generator.components.cse.openssl import SaslCyrusOpenSSLCompile
from config_generator.components.sanitizers.asan import TAG
from config_generator.etc.compile import generate_compile_tasks
from config_generator.etc.sanitizers.test import generate_test_tasks

# pylint: disable=line-too-long
# fmt: off
COMPILE_MATRIX = [
    ('rhel8-latest',     'clang', None, ['cyrus']),
]

# CSFLE requires 4.2+. QE requires 7.0+ and are skipped on "server" tasks.
TEST_MATRIX = [
    # rhel8-latest provides 4.2 through latest.
    ('rhel8-latest', 'clang', None, 'cyrus', ['auth'], ['server', 'replica'], ['4.2', '4.4', '5.0', '6.0', '7.0', '8.0', 'rapid', 'latest']),
]
# fmt: on
# pylint: enable=line-too-long


MORE_TAGS = ['cse', 'asan']


def tasks():
    res = []

    SSL = 'openssl'
    SASL_TO_FUNC = {
        'cyrus': SaslCyrusOpenSSLCompile,
    }

    res += generate_compile_tasks(SSL, TAG, SASL_TO_FUNC, COMPILE_MATRIX, MORE_TAGS)

    res += generate_test_tasks(SSL, TAG, TEST_MATRIX, MORE_TAGS)

    res += generate_test_tasks(
        SSL, TAG, TEST_MATRIX, MORE_TAGS, MORE_TEST_TAGS=['with-mongocrypt'], MORE_VARS={'SKIP_CRYPT_SHARED_LIB': 'on'}
    )

    return res
