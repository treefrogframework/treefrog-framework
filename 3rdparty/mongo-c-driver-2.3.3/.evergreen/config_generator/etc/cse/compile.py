from typing import ClassVar

from shrub.v3.evg_command import EvgCommand, EvgCommandType, KeyValueParam, expansions_update

from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class CompileCommon(Function):
    ssl: ClassVar[str | None]

    @classmethod
    def compile_commands(cls, sasl=None) -> list[EvgCommand]:
        updates = []

        if cls.ssl:
            updates.append(KeyValueParam(key='SSL', value=cls.ssl))

        if sasl:
            updates.append(KeyValueParam(key='SASL', value=sasl))

        return [
            expansions_update(updates=updates),
            bash_exec(
                command_type=EvgCommandType.TEST,
                script='EXTRA_CONFIGURE_FLAGS="-DENABLE_PIC=ON ${EXTRA_CONFIGURE_FLAGS}" .evergreen/scripts/compile.sh',
                working_dir='mongoc',
                add_expansions_to_env=True,
                env={
                    'COMPILE_LIBMONGOCRYPT': 'ON',
                },
            ),
        ]
