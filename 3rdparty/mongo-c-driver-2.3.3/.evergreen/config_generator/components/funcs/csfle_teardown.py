from shrub.v3.evg_command import EvgCommandType, ec2_assume_role

from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class CSFLETeardown(Function):
    name = 'csfle-teardown'
    command_type = EvgCommandType.SETUP
    commands = [
        bash_exec(
            command_type=command_type,
            working_dir='drivers-evergreen-tools/.evergreen/csfle',
            include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
            script='./teardown.sh',
        ),
    ]


def functions():
    return CSFLETeardown.defn()
