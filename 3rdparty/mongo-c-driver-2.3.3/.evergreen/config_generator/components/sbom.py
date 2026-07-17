from pydantic import ConfigDict
from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import (
    BuiltInCommand,
    EvgCommandType,
    KeyValueParam,
    ec2_assume_role,
    expansions_update,
    s3_put,
)
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.etc.distros import find_small_distro
from config_generator.etc.function import Function, merge_defns
from config_generator.etc.utils import bash_exec

TAG = 'sbom'


class CustomCommand(BuiltInCommand):
    command: str
    model_config = ConfigDict(arbitrary_types_allowed=True)


class SBOM(Function):
    name = 'sbom'
    commands = [
        # Authenticate with Kondukto.
        *[
            ec2_assume_role(
                command_type=EvgCommandType.SETUP,
                role_arn='${kondukto_role_arn}',
            ),
            bash_exec(
                command_type=EvgCommandType.SETUP,
                include_expansions_in_env=[
                    'AWS_ACCESS_KEY_ID',
                    'AWS_SECRET_ACCESS_KEY',
                    'AWS_SESSION_TOKEN',
                ],
                script="""\
                set -o errexit
                set -o pipefail
                kondukto_token="$(aws secretsmanager get-secret-value --secret-id "kondukto-token" --region "us-east-1" --query 'SecretString' --output text)"
                printf "KONDUKTO_TOKEN: %s\\n" "$kondukto_token" >|expansions.kondukto.yml
                """,
            ),
            expansions_update(
                command_type=EvgCommandType.SETUP,
                file='expansions.kondukto.yml',
            ),
        ],
        # Authenticate with Amazon ECR.
        *[
            # Avoid inadvertently using a pre-existing and potentially conflicting Podman config.
            # Note: podman understands and uses DOCKER_CONFIG despite the name.
            expansions_update(updates=[KeyValueParam(key='DOCKER_CONFIG', value='${workdir}/.docker')]),
            ec2_assume_role(role_arn='arn:aws:iam::901841024863:role/ecr-role-evergreen-ro'),
            bash_exec(
                command_type=EvgCommandType.SETUP,
                include_expansions_in_env=[
                    'AWS_ACCESS_KEY_ID',
                    'AWS_SECRET_ACCESS_KEY',
                    'AWS_SESSION_TOKEN',
                    'DOCKER_CONFIG',
                ],
                script='aws ecr get-login-password --region us-east-1 | podman login --username AWS --password-stdin 901841024863.dkr.ecr.us-east-1.amazonaws.com',
            ),
        ],
        bash_exec(
            command_type=EvgCommandType.TEST,
            working_dir='mongoc',
            include_expansions_in_env=[
                'branch_name',
                'DOCKER_CONFIG',
                'KONDUKTO_TOKEN',
            ],
            script='.evergreen/scripts/sbom.sh',
        ),
        s3_put(
            command_type=EvgCommandType.TEST,
            aws_key='${aws_key}',
            aws_secret='${aws_secret}',
            bucket='mciuploads',
            content_type='application/json',
            display_name='Augmented SBOM',
            local_file='mongoc/augmented-sbom.json',
            permissions='public-read',
            remote_file='${project}/${build_variant}/${revision}/${version_id}/${build_id}/sbom/augmented-sbom.json',
        ),
    ]


def functions():
    return merge_defns(
        SBOM.defn(),
    )


def tasks():
    distro_name = 'rhel80'
    distro = find_small_distro(distro_name)

    yield EvgTask(
        name='sbom',
        tags=[TAG, distro_name],
        run_on=distro.name,
        commands=[
            SBOM.call(),
        ],
    )


def variants():
    return [
        BuildVariant(
            name=TAG,
            display_name='SBOM',
            tasks=[EvgTaskRef(name=f'.{TAG}')],
        ),
    ]
