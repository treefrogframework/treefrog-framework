from shrub.v3.evg_command import EvgCommandType

from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class FetchDET(Function):
    name = 'fetch-det'
    commands = [
        bash_exec(
            command_type=EvgCommandType.SETUP,
            script="""\
                if [[ ! -d drivers-evergreen-tools ]]; then
                    git clone --depth=1 https://github.com/mongodb-labs/drivers-evergreen-tools.git
                fi
            """,
        ),
        # Make shell scripts executable.
        bash_exec(
            command_type=EvgCommandType.SETUP,
            working_dir='drivers-evergreen-tools',
            script='find .evergreen -type f -name "*.sh" -exec chmod +rx "{}" \;',
        ),
    ]


def functions():
    return FetchDET.defn()
