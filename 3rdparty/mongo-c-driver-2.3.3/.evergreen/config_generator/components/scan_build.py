from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType, FunctionCall
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.etc.distros import compiler_to_vars, find_large_distro, make_distro_str
from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec

TAG = 'scan-build-matrix'


# pylint: disable=line-too-long
# fmt: off
MATRIX = [
    ('macos-14-arm64',   'clang',    None  ),
    ('ubuntu2204-arm64', 'clang',    None  ),
    ('ubuntu2204',       'clang-12', 'i686'),
]
# fmt: on
# pylint: enable=line-too-long


class ScanBuild(Function):
    name = 'scan-build'
    commands = [
        bash_exec(
            command_type=EvgCommandType.TEST,
            add_expansions_to_env=True,
            redirect_standard_error_to_output=True,
            working_dir='mongoc',
            script='.evergreen/scripts/compile-scan-build.sh',
        ),
    ]


def functions():
    return ScanBuild.defn()


def tasks():
    res = []

    for distro_name, compiler, arch in MATRIX:
        tags = [TAG, distro_name, compiler]

        distro = find_large_distro(distro_name)

        compile_vars = None
        compile_vars = compiler_to_vars(compiler)

        if arch:
            tags.append(arch)
            compile_vars.update({'MARCH': arch})

        distro_str = make_distro_str(distro_name, compiler, arch)

        task_name = f'scan-build-{distro_str}'

        res.append(
            EvgTask(
                name=task_name,
                run_on=distro.name,
                tags=tags,
                commands=[
                    ScanBuild.call(vars=compile_vars if compile_vars else None),
                    FunctionCall(func='upload scan artifacts'),
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
