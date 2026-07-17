from __future__ import annotations

import functools
import re
from typing import Iterable, Literal, Mapping, NamedTuple, Optional, TypeVar

from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import (
    EvgCommandType,
    KeyValueParam,
    ec2_assume_role,
    expansions_update,
    subprocess_exec,
)
from shrub.v3.evg_task import EvgTask, EvgTaskRef

from config_generator.etc.function import Function

from ..etc.utils import BuiltInCommandWithRetry, all_possible, subprocess_exec_with_retry

T = TypeVar('T')

_ENV_PARAM_NAME = 'MONGOC_EARTHLY_FROM'
_ECR_HOST = '901841024863.dkr.ecr.us-east-1.amazonaws.com'
_CC_PARAM_NAME = 'MONGOC_EARTHLY_C_COMPILER'
'The name of the EVG expansion for the Earthly c_compiler argument'

EnvImage = Literal[
    'ubuntu:20.04',
    'ubuntu:22.04',
    'ubuntu:24.04',
    'almalinux:8',
    'almalinux:9',
    'almalinux:10',
    'alpine:3.19',
    'alpine:3.20',
    'alpine:3.21',
    'alpine:3.22',
    'archlinux',
    'quay.io/centos/centos:stream9',
    'quay.io/centos/centos:stream10',
]
'Base environment images to be built.'
CompilerName = Literal['gcc', 'clang']
'The name of the compiler program that is used for the build. Passed via --c_compiler to Earthly.'

# Other options: SSPI (Windows only), AUTO (not reliably test-able without more environments)
SASLOption = Literal['Cyrus', 'off']
'Valid options for the SASL configuration parameter'
TLSOption = Literal['OpenSSL', 'off']
"Options for the TLS backend configuration parameter (AKA 'ENABLE_SSL')"
CxxVersion = Literal['master', 'r4.1.0', 'none']
'C++ driver refs that are under CI test'
SnappyOption = Literal['false', 'true']
"""Should we enable Snappy compression in this build?"""

# A separator character, since we cannot use whitespace
_SEPARATOR = '\N{NO-BREAK SPACE}\N{BULLET}\N{NO-BREAK SPACE}'


def os_split(env: EnvImage) -> tuple[str, None | str]:
    """Convert the environment key into a pretty name+version pair"""
    match env:
        # Match 'alpine:3.18' 'alpine:53.123' etc.
        case alp if mat := re.match(r'alpine:(\d+\.\d+)', alp):
            return ('Alpine', mat[1])
        case 'archlinux':
            return 'ArchLinux', None
        # Match 'ubuntu:<version>'.
        case ubu if mat := re.match(r'ubuntu:(\d\d.*)', ubu):
            return 'Ubuntu', f'{mat[1]}'
        # Match 'centos:9', 'centos:stream10', etc.
        case cent if mat := re.match(r'.*centos:(?:stream)?(\d+)', cent):
            return 'CentOS', f'{mat[1]}'
        # Match 'almalinux:8', 'almalinux:10', etc.
        case alm if mat := re.match(r'almalinux:(\d+.*)', alm):
            return 'AlmaLinux', f'{mat[1]}'
        case _:
            raise ValueError(f'Failed to split OS env key {env=} into a name+version pair (unrecognized)')


def from_container_image(img: EnvImage) -> str:
    """
    Modify an unqualified FROM container identifier to route to our ECR host

    NOTE: This will be potentially unnecessary pending the completion of DEVPROD-21478
    """
    if '/' in img or img.startswith('+'):
        return img
    return f'{_ECR_HOST}/dockerhub/library/{img}'


class EarthlyVariant(NamedTuple):
    """
    Define a "variant" that runs under a set of Earthly parameters. These are
    turned into real EVG variants later on. The Earthly arguments are passed via
    expansion parameters.
    """

    from_: EnvImage
    compiler: CompilerName

    @property
    def display_name(self) -> str:
        """The pretty name for this variant"""
        base: str
        match os_split(self.from_):
            case name, None:
                base = name
            case name, version:
                base = f'{name} {version}'
        toolchain: str
        match self.compiler:
            case 'clang':
                toolchain = 'LLVM/Clang'
            case 'gcc':
                toolchain = 'GCC'
        return f'{base} ({toolchain})'

    @property
    def task_selector_tag(self) -> str:
        """
        The task tag that is used to select the tasks that want to run on this
        variant.
        """
        return f'{self.from_}-{self.compiler}'

    @property
    def expansions(self) -> Mapping[str, str]:
        """
        Expansion values that are defined for the build variant that is generated
        from this object.
        """
        return {
            _CC_PARAM_NAME: self.compiler,
            _ENV_PARAM_NAME: from_container_image(self.from_),
        }

    def as_evg_variant(self) -> BuildVariant:
        return BuildVariant(
            name=f'{self.task_selector_tag}',
            tasks=[EvgTaskRef(name=f'.{self.task_selector_tag}')],
            display_name=self.display_name,
            expansions=dict(self.expansions),
        )


class Configuration(NamedTuple):
    """
    Represent a complete set of argument values to give to the Earthly task
    execution. Each field name matches the ARG in the Earthfile.

    Adding/removing fields will add/remove dimensions on the task matrix.

    Some Earthly parameters are not encoded here, but are rather part of the variant (EarthlyVariant).
    """

    sasl: SASLOption
    tls: TLSOption
    test_mongocxx_ref: CxxVersion
    snappy: SnappyOption

    @property
    def suffix(self) -> str:
        return _SEPARATOR.join(f'{k}={v}' for k, v in self._asdict().items())


# Authenticate with DevProd-provided Amazon ECR instance to use as pull-through cache for DockerHub.
class DockerLoginAmazonECR(Function):
    name = 'docker-login-amazon-ecr'
    commands = [
        # Avoid inadvertently using a pre-existing and potentially conflicting Docker config.
        expansions_update(updates=[KeyValueParam(key='DOCKER_CONFIG', value='${workdir}/.docker')]),
        ec2_assume_role(role_arn='arn:aws:iam::901841024863:role/ecr-role-evergreen-ro'),
        subprocess_exec(
            binary='bash',
            command_type=EvgCommandType.SETUP,
            include_expansions_in_env=[
                'AWS_ACCESS_KEY_ID',
                'AWS_SECRET_ACCESS_KEY',
                'AWS_SESSION_TOKEN',
                'DOCKER_CONFIG',
            ],
            args=[
                '-c',
                f'aws ecr get-login-password --region us-east-1 | docker login --username AWS --password-stdin {_ECR_HOST}',
            ],
        ),
    ]


def task_filter(env: EarthlyVariant, conf: Configuration) -> bool:
    """
    Control which tasks are actually defined by matching on the platform and
    configuration values.
    """
    match env, conf:
        # Anything else: Allow it to run:
        case _:
            return True


def variants_for(config: Configuration) -> Iterable[EarthlyVariant]:
    """Get all Earthly variants that are not excluded for the given build configuration"""
    all_envs = all_possible(EarthlyVariant)
    allow_env_for_config = functools.partial(task_filter, conf=config)
    return filter(allow_env_for_config, all_envs)


def earthly_exec(
    *,
    kind: Literal['test', 'setup', 'system'],
    target: str,
    platform: str | None = None,
    secrets: Mapping[str, str] | None = None,
    args: Mapping[str, str] | None = None,
    retry_on_failure: Optional[bool] = None,
) -> BuiltInCommandWithRetry:
    """Create a subprocess_exec command that runs Earthly with the given arguments"""
    env: dict[str, str] = {k: v for k, v in (secrets or {}).items()}
    return subprocess_exec_with_retry(
        './tools/earthly.sh',
        args=[
            # Use Amazon ECR as pull-through cache for DockerHub to avoid rate limits.
            f'--buildkit-image={_ECR_HOST}/dockerhub/earthly/buildkitd:v0.8.3',
            *(f'--secret={k}' for k in (secrets or ())),
            *([f'--platform={platform}'] if platform else ()),
            f'+{target}',
            # Use Amazon ECR as pull-through cache for DockerHub to avoid rate limits.
            f'--default_search_registry={_ECR_HOST}/dockerhub/library',
            *(f'--{arg}={val}' for arg, val in (args or {}).items()),
        ],
        command_type=EvgCommandType(kind),
        include_expansions_in_env=['DOCKER_CONFIG'],
        env=env if env else None,
        working_dir='mongoc',
        retry_on_failure=retry_on_failure,
    )


def earthly_task(
    *,
    name: str,
    targets: Iterable[str],
    config: Configuration,
) -> EvgTask | None:
    """
    Create an EVG task which executes earthly using the given parameters. If this
    function returns `None`, then the task configuration is excluded from executing
    and no task should be defined.
    """
    # Attach tags to the task to allow build variants to select
    # these tasks by the environment of that variant.
    env_tags = sorted(e.task_selector_tag for e in sorted(variants_for(config)))
    if not env_tags:
        # All environments have been excluded for this configuration. This means
        # the task itself should not be run:
        return
    # Generate the build-arg arguments based on the configuration options. The
    # NamedTuple field names must match with the ARG keys in the Earthfile!
    earthly_args = config._asdict()
    earthly_args |= {
        # Add arguments that come from parameter expansions defined in the build variant
        'from': f'${{{_ENV_PARAM_NAME}}}',
        'compiler': f'${{{_CC_PARAM_NAME}}}',
        # Always include a C++ compiler in the build environment for better test coverage
        'with_cxx': 'true',
    }
    return EvgTask(
        name=name,
        commands=[
            DockerLoginAmazonECR.call(),
            # First, just build the "build-environment" which will prepare the build environment.
            # This won't generate any output, but allows EVG to track it as a separate build step
            # for timing and logging purposes. The subequent build step will cache-hit the
            # warmed-up build environments.
            earthly_exec(kind='setup', target='build-environment', args=earthly_args, retry_on_failure=True),
            earthly_exec(kind='setup', target='configure', args=earthly_args, retry_on_failure=True),
            # Now execute the main tasks:
            earthly_exec(
                kind='test',
                target='run',
                # The "targets" arg is for +run to specify which targets to run
                args={'targets': ' '.join(targets)} | earthly_args,
            ),
        ],  # type: ignore (The type annots on `commands` is wrong)
        tags=['earthly', 'pr-merge-gate', *env_tags],
        run_on=CONTAINER_RUN_DISTROS,
    )


CONTAINER_RUN_DISTROS = [
    'amazon2',
    'debian11-latest-large',
    'debian12-latest-large',
    'ubuntu2204-large',
    'ubuntu2404-large',
]


def functions():
    return DockerLoginAmazonECR.defn()


def tasks() -> Iterable[EvgTask]:
    for conf in all_possible(Configuration):
        # test-example is a target in all configurations
        targets = ['test-example']

        # test-cxx-driver is only a target in configurations with specified mongocxx versions
        if conf.test_mongocxx_ref != 'none':
            targets.append('test-cxx-driver')

        task = earthly_task(
            name=f'check:{conf.suffix}',
            targets=targets,
            config=conf,
        )
        if task is not None:
            yield task

    yield EvgTask(
        name='verify-headers',
        commands=[
            DockerLoginAmazonECR.call(),
            earthly_exec(kind='test', target='verify-headers'),
        ],
        tags=['pr-merge-gate'],
        run_on=CONTAINER_RUN_DISTROS,
    )
    for plat in ('amd64', 'i386'):
        yield EvgTask(
            name=f'debian-package-{plat}',
            commands=[
                DockerLoginAmazonECR.call(),
                earthly_exec(kind='test', target='deb.test', platform=f'linux/{plat}'),
            ],
            tags=['packaging', 'pr-merge-gate'],
            run_on=CONTAINER_RUN_DISTROS,
        )


def variants() -> Iterable[BuildVariant]:
    yield from (ev.as_evg_variant() for ev in all_possible(EarthlyVariant))
