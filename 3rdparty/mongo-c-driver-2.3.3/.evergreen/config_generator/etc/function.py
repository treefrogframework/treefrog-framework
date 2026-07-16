from collections import ChainMap
from typing import ClassVar, Mapping

from shrub.v3.evg_command import EvgCommand, FunctionCall


class Function:
    name: ClassVar[str]
    commands: ClassVar[list[EvgCommand]]

    @classmethod
    def defn(cls) -> Mapping[str, list[EvgCommand]]:
        return {cls.name: cls.commands}

    @classmethod
    def default_call(cls, **kwargs) -> FunctionCall:
        return FunctionCall(func=cls.name, **kwargs)

    @classmethod
    def call(cls, **kwargs) -> FunctionCall:
        return cls.default_call(**kwargs)


def merge_defns(*args):
    return ChainMap(*args)
