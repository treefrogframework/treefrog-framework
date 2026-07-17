from itertools import product

from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType, FunctionCall
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.components.funcs.fetch_source import FetchSource
from config_generator.etc.distros import find_large_distro, make_distro_str
from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec

TAG = 'openssl-compat'

# pylint: disable=line-too-long
# fmt: off
OPENSSL_MATRIX = [
    ('ubuntu2404', 'gcc', ['shared', 'static'], ['1.0.2', '1.1.1', '3.0.9', '3.1.2', '3.2.5', '3.3.4', '3.4.2', '3.5.1', '4.0.0']),
]
# fmt: on

# pylint: disable=line-too-long
# fmt: off
OPENSSL_FIPS_MATRIX = [
    # https://openssl-library.org/source/
    # > The following OpenSSL version(s) are FIPS validated:
    # > - 3.1.2: FIPS 140-3
    # > - 3.0.9: FIPS 140-2
    # > - ...
    ('ubuntu2404', 'gcc', ['shared', 'static'], ['3.0.9', '3.1.2']),
]
# fmt: on


class OpenSSLSetup(Function):
    name = 'openssl-compat'
    commands = [
        bash_exec(
            command_type=EvgCommandType.SETUP,
            working_dir='mongoc',
            include_expansions_in_env=[
                'OPENSSL_ENABLE_FIPS',
                'OPENSSL_USE_STATIC_LIBS',
                'OPENSSL_VERSION',
            ],
            script='.evergreen/scripts/openssl-compat-setup.sh',
        ),
        bash_exec(
            command_type=EvgCommandType.SETUP,
            working_dir='mongoc',
            include_expansions_in_env=[
                'OPENSSL_USE_STATIC_LIBS',
                'OPENSSL_VERSION',
            ],
            script='.evergreen/scripts/openssl-compat-check.sh',
        ),
    ]


def functions():
    return OpenSSLSetup.defn()


def tasks():
    for distro_name, compiler, link_types, versions in OPENSSL_MATRIX:
        distro_str = make_distro_str(distro_name, compiler, None)

        for link_type, version in product(link_types, versions):
            vars = {'OPENSSL_VERSION': version}

            if link_type == 'static':
                vars |= {'OPENSSL_USE_STATIC_LIBS': 'ON'}

            commands = [
                FetchSource.call(),
                OpenSSLSetup.call(vars=vars),
                FunctionCall(func='run auth tests'),
            ]

            yield EvgTask(
                name=f'{TAG}-{version}-{link_type}-{distro_str}',
                run_on=find_large_distro(distro_name).name,
                tags=[TAG, f'openssl-{version}', f'openssl-{link_type}', distro_name, compiler],
                commands=commands,
            )

    for distro_name, compiler, link_types, versions in OPENSSL_FIPS_MATRIX:
        distro_str = make_distro_str(distro_name, compiler, None)

        for link_type, version in product(link_types, versions):
            vars = {'OPENSSL_VERSION': version, 'OPENSSL_ENABLE_FIPS': 'ON'}

            if link_type == 'static':
                vars |= {'OPENSSL_USE_STATIC_LIBS': 'ON'}

            yield EvgTask(
                name=f'{TAG}-fips-{version}-{link_type}-{distro_str}',
                run_on=find_large_distro(distro_name).name,
                tags=[TAG, f'openssl-fips-{version}', f'openssl-{link_type}', distro_name, compiler],
                commands=[
                    FetchSource.call(),
                    OpenSSLSetup.call(vars=vars),
                    FunctionCall(func='run auth tests'),
                ],
            )


def variants():
    return [
        BuildVariant(
            name=f'{TAG}-matrix',
            display_name='OpenSSL Compatibility Matrix',
            tasks=[EvgTaskRef(name=f'.{TAG}')],
        ),
    ]
