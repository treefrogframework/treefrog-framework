from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.components.funcs.find_cmake_latest import FindCMakeLatest

from config_generator.etc.distros import find_large_distro
from config_generator.etc.distros import make_distro_str
from config_generator.etc.distros import compiler_to_vars
from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


TAG = 'std-matrix'


# pylint: disable=line-too-long
# fmt: off
MATRIX = [
    ('rhel80',     'clang',    None, [99, 11, 17,   ]), # Clang 7.0
    ('ubuntu2004', 'clang-10', None, [99, 11, 17, 23]), # Clang 10.0 (max: C2x)
    ('rhel84',     'clang',    None, [99, 11, 17, 23]), # Clang 11.0 (max: C2x)
    ('ubuntu2204', 'clang-12', None, [99, 11, 17, 23]), # Clang 12.0 (max: C2x)
    ('rhel90',     'clang',    None, [99, 11, 17, 23]), # Clang 13.0 (max: C2x)
    ('rhel91',     'clang',    None, [99, 11, 17, 23]), # Clang 14.0 (max: C2x)
    ('rhel92',     'clang',    None, [99, 11, 17, 23]), # Clang 15.0 (max: C2x)
    ('rhel93',     'clang',    None, [99, 11, 17, 23]), # Clang 16.0 (max: C2x)
    ('rhel94',     'clang',    None, [99, 11, 17, 23]), # Clang 17.0 (max: C2x)
    ('rhel95',     'clang',    None, [99, 11, 17, 23]), # Clang 18.0 (max: C23)

    ('rhel7-latest', 'gcc',    None, [99, 11,       ]), # GCC 4.8 (max: C11)
    ('rhel80',       'gcc',    None, [99, 11, 17,   ]), # GCC 8.2 (max: C17)
    ('rhel84',       'gcc',    None, [99, 11, 17,   ]), # GCC 8.4 (max: C17)
    ('ubuntu2004',   'gcc-9',  None, [99, 11, 17, 23]), # GCC 9.4 (max: C2x)
    ('debian11',     'gcc-10', None, [99, 11, 17, 23]), # GCC 10.2 (max: C2x)
    ('rhel90',       'gcc',    None, [99, 11, 17, 23]), # GCC 11.2 (max: C2x)
    ('rhel92',       'gcc',    None, [99, 11, 17, 23]), # GCC 11.3 (max: C2x)
    ('rhel94',       'gcc',    None, [99, 11, 17, 23]), # GCC 11.4 (max: C2x)
    ('rhel95',       'gcc',    None, [99, 11, 17, 23]), # GCC 11.5 (max: C2x)
    ('ubuntu2404',   'gcc-13', None, [99, 11, 17, 23]), # GCC 13.3 (max: C2x)

    ('windows-vsCurrent', 'vs2015x64', None, [99, 11,   ]), # Max: C11
    ('windows-vsCurrent', 'vs2017x64', None, [99, 11,   ]), # Max: C11
    ('windows-vsCurrent', 'vs2019x64', None, [99, 11, 17]), # Max: C17
    ('windows-vsCurrent', 'vs2022x64', None, [99, 11, 17]), # Max: C17
]
# fmt: on
# pylint: enable=line-too-long


class StdCompile(Function):
    name = 'std-compile'
    commands = [
        bash_exec(
            command_type=EvgCommandType.TEST,
            add_expansions_to_env=True,
            working_dir='mongoc',
            script='.evergreen/scripts/compile-std.sh',
        ),
    ]


def functions():
    return StdCompile.defn()


def tasks():
    res = []

    for distro_name, compiler, arch, stds in MATRIX:
        compiler_type = compiler.split('-')[0]

        tags = [TAG, distro_name, compiler_type, 'compile']

        distro = find_large_distro(distro_name)

        compile_vars = None
        compile_vars = compiler_to_vars(compiler)

        if arch:
            tags.append(arch)
            compile_vars.update({'MARCH': arch})

        distro_str = make_distro_str(distro_name, compiler_type, arch)

        for std in stds:
            with_std = {'C_STD_VERSION': std}

            task_name = f'std-c{std}-{distro_str}-compile'

            res.append(
                EvgTask(
                    name=task_name,
                    run_on=distro.name,
                    tags=tags + [f'std-c{std}'],
                    commands=[
                        FindCMakeLatest.call(),
                        StdCompile.call(vars=compile_vars | with_std)
                    ],
                )
            )

    return res


def variants():
    return [
        BuildVariant(
            name=TAG,
            display_name=TAG,
            tasks=[EvgTaskRef(name=f'.{TAG}')],
        ),
    ]
