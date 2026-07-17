from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.etc.distros import compiler_to_vars, find_large_distro
from config_generator.etc.utils import bash_exec

# Compile-only task for CDRIVER-6346: build on Windows with UNICODE/_UNICODE defined.

# Enable C4133 as error to identify string pointer mismatch.
_UNICODE_FLAGS = "/we4133 /DUNICODE /D_UNICODE"


def tasks():
    return [
        EvgTask(
            name='unicode-compile',
            run_on=find_large_distro('windows-2022-latest').name,
            tags=['unicode-compile', 'compile'],
            commands=[
                bash_exec(
                    command_type=EvgCommandType.TEST,
                    add_expansions_to_env=True,
                    working_dir='mongoc',
                    script='.evergreen/scripts/compile.sh',
                ),
            ],
        )
    ]


def variants():
    return [
        BuildVariant(
            name='unicode-compile',
            display_name='UNICODE Compile',
            tasks=[EvgTaskRef(name='unicode-compile')],
            expansions={
                **compiler_to_vars('vs2022x64'),
                'CFLAGS': _UNICODE_FLAGS,
                'CXXFLAGS': _UNICODE_FLAGS,
            },
        ),
    ]
