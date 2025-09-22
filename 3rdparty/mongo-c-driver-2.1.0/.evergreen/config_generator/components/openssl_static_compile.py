from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.components.funcs.find_cmake_latest import FindCMakeLatest

from config_generator.etc.distros import find_large_distro
from config_generator.etc.distros import make_distro_str
from config_generator.etc.distros import compiler_to_vars
from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec

SSL = 'openssl-static'
TAG = f'{SSL}-matrix'


# pylint: disable=line-too-long
# fmt: off
MATRIX = [
  ('debian11',   'gcc', None),
  ('debian12',   'gcc', None),
  ('ubuntu2004', 'gcc', None),
  ('ubuntu2204', 'gcc', None),
  ('ubuntu2404', 'gcc', None),
]
# fmt: on
# pylint: enable=line-too-long


class StaticOpenSSLCompile(Function):
    name = 'openssl-static-compile'
    commands = [
        bash_exec(
            command_type=EvgCommandType.TEST,
            add_expansions_to_env=True,
            working_dir='mongoc',
            script='.evergreen/scripts/compile-openssl-static.sh',
        ),
    ]


def functions():
    return StaticOpenSSLCompile.defn()


def tasks():
    res = []

    for distro_name, compiler, arch, in MATRIX:
        tags = [TAG, distro_name, compiler]

        distro = find_large_distro(distro_name)

        compile_vars = None
        compile_vars = compiler_to_vars(compiler)

        if arch:
            tags.append(arch)
            compile_vars.update({'MARCH': arch})

        distro_str = make_distro_str(distro_name, compiler, arch)

        task_name = f'openssl-static-compile-{distro_str}'

        res.append(
            EvgTask(
                name=task_name,
                run_on=distro.name,
                tags=tags,
                commands=[
                    FindCMakeLatest.call(),
                    StaticOpenSSLCompile.call(vars=compile_vars if compile_vars else None),
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
