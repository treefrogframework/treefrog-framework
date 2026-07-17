from shrub.v3.evg_task import EvgTask

from config_generator.components.funcs.upload_build import UploadBuild
from config_generator.etc.distros import compiler_to_vars, find_large_distro, make_distro_str


def generate_compile_tasks(SSL, TAG, SASL_TO_FUNC, MATRIX, MORE_TAGS=None, MORE_VARS=None):
    res = []

    MORE_TAGS = MORE_TAGS if MORE_TAGS else []
    MORE_VARS = MORE_VARS if MORE_VARS else {}

    for distro_name, compiler, arch, sasls in MATRIX:
        tags = [TAG, 'compile', distro_name, compiler] + MORE_TAGS

        distro = find_large_distro(distro_name)

        compile_vars = None
        compile_vars = compiler_to_vars(compiler)

        if arch:
            tags.append(arch)
            compile_vars.update({'MARCH': arch})

        compile_vars.update(MORE_VARS)

        distro_str = make_distro_str(distro_name, compiler, arch)

        for sasl in sasls:
            task_name = f'sasl-{sasl}-{SSL}-{distro_str}-compile'

            for tag in MORE_TAGS:
                task_name = f'{tag}-{task_name}'

            commands = []
            commands.append(SASL_TO_FUNC[sasl].call(vars=compile_vars if compile_vars else None))
            commands.append(UploadBuild.call())

            res.append(
                EvgTask(
                    name=task_name,
                    run_on=distro.name,
                    tags=tags + [f'sasl-{sasl}'],
                    commands=commands,
                )
            )

    return res
