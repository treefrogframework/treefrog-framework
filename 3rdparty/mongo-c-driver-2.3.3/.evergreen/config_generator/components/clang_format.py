from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.etc.distros import find_small_distro
from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec

TAG = 'clang-format'


class ClangFormat(Function):
    name = TAG
    commands = [
        bash_exec(
            command_type=EvgCommandType.TEST,
            working_dir='mongoc',
            env={
                'DRYRUN': '1',
            },
            script='uv run --frozen --only-group=format tools/format.py --mode=check',
        ),
    ]


def functions():
    return ClangFormat.defn()


def tasks():
    yield EvgTask(
        name=TAG,
        tags=[TAG],
        commands=[
            ClangFormat.call(),
        ],
    )


def variants():
    return [
        BuildVariant(
            name=TAG,
            display_name=TAG,
            run_on=[find_small_distro('ubuntu2204').name],
            tasks=[EvgTaskRef(name=f'.{TAG}')],
        ),
    ]
