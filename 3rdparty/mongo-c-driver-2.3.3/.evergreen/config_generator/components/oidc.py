from shrub.v3.evg_build_variant import BuildVariant
from shrub.v3.evg_command import EvgCommandType, KeyValueParam, ec2_assume_role, expansions_update
from shrub.v3.evg_task import EvgTask, EvgTaskRef
from shrub.v3.evg_task_group import EvgTaskGroup

from config_generator.components.funcs.fetch_det import FetchDET
from config_generator.components.funcs.fetch_source import FetchSource
from config_generator.components.funcs.run_tests import RunTests
from config_generator.components.sasl.openssl import SaslCyrusOpenSSLCompile
from config_generator.etc.distros import find_small_distro
from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class RunOIDCK8sTest(Function):
    name = 'run oidc k8s test'
    commands = [
        bash_exec(
            include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
            command_type=EvgCommandType.SETUP,
            env={
                'K8S_VARIANT': '${VARIANT}',
            },
            script='./drivers-evergreen-tools/.evergreen/auth_oidc/k8s/setup-pod.sh',
        ),
        bash_exec(
            include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
            command_type=EvgCommandType.TEST,
            env={
                'K8S_DRIVERS_TAR_FILE': '${OIDC_TEST_TARBALL}',
                'K8S_TEST_CMD': './.evergreen/scripts/oidc-k8s-test.sh',
            },
            script='./drivers-evergreen-tools/.evergreen/auth_oidc/k8s/run-driver-test.sh',
        ),
        bash_exec(
            include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
            command_type=EvgCommandType.SETUP,
            script='./drivers-evergreen-tools/.evergreen/auth_oidc/k8s/teardown-pod.sh',
        ),
    ]


def functions():
    return RunOIDCK8sTest.defn()


def task_groups():
    return [
        EvgTaskGroup(
            name='test-oidc-task-group',
            tasks=['oidc-auth-test-task'],
            setup_group_can_fail_task=True,
            teardown_group_can_fail_task=True,
            teardown_group_timeout_secs=180,  # 3 minutes
            setup_group=[
                FetchDET.call(),
                ec2_assume_role(role_arn='${aws_test_secrets_role}'),
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/setup.sh',
                ),
            ],
            teardown_group=[
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/teardown.sh',
                )
            ],
        ),
        EvgTaskGroup(
            name='test-oidc-azure-task-group',
            tasks=['oidc-azure-auth-test-task'],
            setup_group_can_fail_task=True,
            teardown_group_can_fail_task=True,
            teardown_group_timeout_secs=180,  # 3 minutes
            setup_group=[
                FetchDET.call(),
                ec2_assume_role(role_arn='${aws_test_secrets_role}'),
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
                    env={'AZUREOIDC_VMNAME_PREFIX': 'CDRIVER'},
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/azure/create-and-setup-vm.sh',
                ),
            ],
            teardown_group=[
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/azure/delete-vm.sh',
                ),
            ],
        ),
        EvgTaskGroup(
            name='test-oidc-gcp-task-group',
            tasks=['oidc-gcp-auth-test-task'],
            setup_group_can_fail_task=True,
            teardown_group_can_fail_task=True,
            teardown_group_timeout_secs=180,  # 3 minutes
            setup_group=[
                FetchDET.call(),
                ec2_assume_role(role_arn='${aws_test_secrets_role}'),
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
                    env={'GCPOIDC_VMNAME_PREFIX': 'CDRIVER'},
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/gcp/setup.sh',
                ),
            ],
            teardown_group=[
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/gcp/teardown.sh',
                ),
            ],
        ),
        EvgTaskGroup(
            name='test-oidc-k8s-task-group',
            tasks=['oidc-k8s-auth-test-task'],
            setup_group_can_fail_task=True,
            teardown_group_can_fail_task=True,
            teardown_group_timeout_secs=180,  # 3 minutes
            setup_group=[
                FetchDET.call(),
                ec2_assume_role(role_arn='${aws_test_secrets_role}'),
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    include_expansions_in_env=['AWS_ACCESS_KEY_ID', 'AWS_SECRET_ACCESS_KEY', 'AWS_SESSION_TOKEN'],
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/k8s/setup.sh',
                ),
            ],
            teardown_group=[
                bash_exec(
                    command_type=EvgCommandType.SETUP,
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/k8s/teardown.sh',
                ),
            ],
        ),
    ]


def tasks():
    return [
        EvgTask(
            name='oidc-auth-test-task',
            run_on=[find_small_distro('ubuntu2404').name],
            commands=[
                FetchSource.call(),
                expansions_update(
                    updates=[
                        KeyValueParam(key='ASAN', value='on'),
                        KeyValueParam(key='CFLAGS', value='-fno-omit-frame-pointer'),
                        KeyValueParam(key='SANITIZE', value='address,undefined'),
                        KeyValueParam(key='CC', value='gcc'),
                        # OIDC test servers support both OIDC and user/password.
                        KeyValueParam(key='AUTH', value='auth'),  # Use user/password for default test clients.
                        KeyValueParam(key='OIDC', value='oidc'),  # Enable OIDC tests.
                        KeyValueParam(key='MONGODB_VERSION', value='latest'),
                        KeyValueParam(key='TOPOLOGY', value='replica_set'),
                    ]
                ),
                SaslCyrusOpenSSLCompile.call(),
                RunTests.call(),
            ],
        ),
        EvgTask(
            name='oidc-azure-auth-test-task',
            run_on=['debian11-small'],  # TODO: switch to 'debian11-latest' after DEVPROD-23011 fixed.
            commands=[
                FetchSource.call(),
                bash_exec(
                    working_dir='mongoc',
                    add_expansions_to_env=True,
                    command_type=EvgCommandType.TEST,
                    script='.evergreen/scripts/oidc-azure-compile.sh',
                ),
                expansions_update(file='mongoc/oidc-remote-test-expansion.yml'),
                bash_exec(
                    add_expansions_to_env=True,
                    command_type=EvgCommandType.TEST,
                    env={
                        'AZUREOIDC_DRIVERS_TAR_FILE': '${OIDC_TEST_TARBALL}',
                        'AZUREOIDC_TEST_CMD': 'source ./env.sh && ./.evergreen/scripts/oidc-azure-test.sh',
                    },
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/azure/run-driver-test.sh',
                ),
            ],
        ),
        EvgTask(
            name='oidc-gcp-auth-test-task',
            run_on=['debian11-small'],  # TODO: switch to 'debian11-latest' after DEVPROD-23011 fixed.
            commands=[
                FetchSource.call(),
                bash_exec(
                    working_dir='mongoc',
                    add_expansions_to_env=True,
                    command_type=EvgCommandType.TEST,
                    script='.evergreen/scripts/oidc-gcp-compile.sh',
                ),
                expansions_update(file='mongoc/oidc-remote-test-expansion.yml'),
                bash_exec(
                    add_expansions_to_env=True,
                    command_type=EvgCommandType.TEST,
                    env={
                        'GCPOIDC_DRIVERS_TAR_FILE': '${OIDC_TEST_TARBALL}',
                        'GCPOIDC_TEST_CMD': 'source ./secrets-export.sh && ./.evergreen/scripts/oidc-gcp-test.sh',
                    },
                    script='./drivers-evergreen-tools/.evergreen/auth_oidc/gcp/run-driver-test.sh',
                ),
            ],
        ),
        EvgTask(
            name='oidc-k8s-auth-test-task',
            run_on=['ubuntu2204-small'],
            commands=[
                FetchSource.call(),
                bash_exec(
                    working_dir='mongoc',
                    add_expansions_to_env=True,
                    command_type=EvgCommandType.TEST,
                    script='.evergreen/scripts/oidc-k8s-compile.sh',
                ),
                expansions_update(file='mongoc/oidc-remote-test-expansion.yml'),
                RunOIDCK8sTest.call(vars={'VARIANT': 'eks'}),
                RunOIDCK8sTest.call(vars={'VARIANT': 'gke'}),
                RunOIDCK8sTest.call(vars={'VARIANT': 'aks'}),
            ],
        ),
    ]


def variants():
    return [
        BuildVariant(
            name='oidc',
            display_name='OIDC',
            tasks=[
                EvgTaskRef(name='test-oidc-task-group'),
                EvgTaskRef(name='test-oidc-azure-task-group'),
                EvgTaskRef(name='test-oidc-gcp-task-group'),
                EvgTaskRef(name='test-oidc-k8s-task-group'),
            ],
        ),
    ]
