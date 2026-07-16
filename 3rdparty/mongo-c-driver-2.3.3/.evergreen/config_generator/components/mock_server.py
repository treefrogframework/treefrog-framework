from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.components.funcs.run_simple_http_server import RunSimpleHTTPServer
from config_generator.etc.utils import bash_exec


def tasks():
    return [
        EvgTask(
            name='mock-server-test',
            run_on='ubuntu2204-small',
            commands=[
                RunSimpleHTTPServer.call(),
                bash_exec(
                    command_type=EvgCommandType.TEST,
                    add_expansions_to_env=True,
                    working_dir='mongoc',
                    script='.evergreen/scripts/compile.sh',
                ),
                bash_exec(
                    command_type=EvgCommandType.TEST,
                    working_dir='mongoc',
                    script='.evergreen/scripts/run-mock-server-tests.sh',
                ),
            ],
        )
    ]


def variants():
    return [
        BuildVariant(
            name='mock-server-test',
            display_name='Mock Server Test',
            tasks=[EvgTaskRef(name='mock-server-test')],
            expansions={
                'CC': 'gcc',
                'ASAN': 'on',
                'CFLAGS': '-fno-omit-frame-pointer',
                'SANITIZE': 'address,undefined',
            },
        ),
    ]
