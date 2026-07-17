from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class StopMongoOrchestration(Function):
    name = 'stop-mongo-orchestration'
    commands = [
        bash_exec(
            script="""\
                if [[ -d drivers-evergreen-tools ]]; then
                    cd drivers-evergreen-tools && .evergreen/run-mongodb.sh stop
                fi
            """
        ),
    ]


def functions():
    return StopMongoOrchestration.defn()
