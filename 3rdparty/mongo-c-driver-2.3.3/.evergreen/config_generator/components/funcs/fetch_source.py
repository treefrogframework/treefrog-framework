from shrub.v3.evg_command import EvgCommandType, expansions_update, git_get_project

from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class FetchSource(Function):
    name = 'fetch-source'
    command_type = EvgCommandType.SETUP
    commands = [
        git_get_project(command_type=command_type, directory='mongoc'),
        bash_exec(
            command_type=command_type,
            working_dir='mongoc',
            script="""\
                set -o errexit
                set -o pipefail
                if [ -n "${github_pr_number}" -o "${is_patch}" = "true" ]; then
                    VERSION=patch-${version_id}
                else
                    VERSION=latest
                fi
                echo "CURRENT_VERSION: $VERSION" > expansion.yml
            """,
        ),
        expansions_update(command_type=command_type, file='mongoc/expansion.yml'),
        # Scripts may not be executable on Windows.
        bash_exec(
            command_type=EvgCommandType.SETUP,
            working_dir='mongoc',
            script="""\
                for file in $(find .evergreen/scripts -type f); do
                    chmod +rx "$file" || exit
                done
            """,
        ),
    ]


def functions():
    return FetchSource.defn()
