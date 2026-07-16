from shrub.v3.evg_command import EvgCommandType, ec2_assume_role

from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class CSFLESetup(Function):
    name = 'csfle-setup'
    command_type = EvgCommandType.SETUP
    commands = [
        # This command ensures future invocations of activate-kmstlsvenv.sh conducted in
        # parallel do not race to setup a venv environment; it has already been prepared.
        # This primarily addresses the situation where the "run tests" and "csfle-setup"
        # functions invoke 'activate-kmstlsvenv.sh' simultaneously.
        bash_exec(
            command_type=command_type,
            working_dir='drivers-evergreen-tools/.evergreen/csfle',
            script="""\
                set -o errexit
                echo "Preparing KMS TLS venv environment..."
                if [[ "$OSTYPE" =~ cygwin && ! -d kmstlsvenv ]]; then
                    # Avoid using Python 3.10 on Windows due to incompatible cipher suites.
                    # See CDRIVER-4530.
                    . ../venv-utils.sh
                    venvcreate "C:/python/Python39/python.exe" kmstlsvenv || # windows-2017
                    venvcreate "C:/python/Python38/python.exe" kmstlsvenv    # windows-2015
                    python -m pip install --upgrade boto3~=1.19 pykmip~=0.10.0 "sqlalchemy<2.0.0"
                    deactivate
                else
                    . ./activate-kmstlsvenv.sh
                    deactivate
                fi
                echo "Preparing KMS TLS venv environment... done."
            """,
        ),
        ec2_assume_role(role_arn='${aws_test_secrets_role}'),
        bash_exec(
            command_type=command_type,
            working_dir='drivers-evergreen-tools/.evergreen/csfle',
            include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
            script='./setup.sh',  # Creates secrets-export.sh. Starts servers on ports 5698, 9000, 9001, 9002, and 9003.
        ),
    ]


def functions():
    return CSFLESetup.defn()
