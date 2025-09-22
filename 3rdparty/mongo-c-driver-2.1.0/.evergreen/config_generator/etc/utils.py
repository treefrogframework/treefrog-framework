import itertools
from importlib import import_module
from inspect import isclass
from pathlib import Path
from textwrap import dedent
from typing import (Any, Iterable, Literal, Mapping, Type, TypeVar,
                    Union, cast)

import yaml
from shrub.v3.evg_command import EvgCommandType, subprocess_exec
from shrub.v3.evg_project import EvgProject
from shrub.v3.shrub_service import ConfigDumper
from shrub.v3.evg_task import EvgTaskRef
from typing_extensions import get_args, get_origin, get_type_hints

T = TypeVar('T')


# Equivalent to EvgTaskRef but defines additional properties.
class TaskRef(EvgTaskRef):
    """
    An evergreen task reference model that also includes additional properties.

    (The shrub.py model is missing some properties)
    """

    batchtime: int | None = None


# Automatically formats the provided script and invokes it in Bash.
def bash_exec(
    script,
    *,
    include_expansions_in_env: Iterable[str] | None = None,
    working_dir: str | None = None,
    command_type: EvgCommandType | None = None,
    retry_on_failure: bool | None = None,
    env: Mapping[str, str] | None = None,
    **kwargs,
):
    ret = subprocess_exec(
        binary="bash",
        args=["-c", dedent(script)],
        include_expansions_in_env=list(include_expansions_in_env) if include_expansions_in_env else None,
        working_dir=working_dir,
        command_type=command_type,
        env=dict(env) if env else None,
        **kwargs,
    )

    if retry_on_failure is not None:
        ret.params |= {"retry_on_failure": retry_on_failure}

    return ret


def all_components():
    res = []

    # .evergreen/config_generator/etc/utils.py -> .evergreen/config_generator/components
    components_dir = Path(__file__).parent.parent / 'components'

    all_paths = components_dir.glob('**/*.py')

    for path in sorted(all_paths):
        component_path = path.relative_to(components_dir)
        component_str = str(component_path.with_suffix(''))  # Drop '.py'.
        component_str = component_str.replace('/', '.')  # 'a/b' -> 'a.b'
        module_name = f'config_generator.components.{component_str}'
        res.append(import_module(module_name))

    return res


# Helper function to print component name for diagnostic purposes.
def component_name(component):
    component_prefix = 'config_generator.components.'
    res = component.__name__[len(component_prefix):]
    return res


def write_to_file(yml, filename):
    # .evergreen/config_generator/etc/utils.py -> .evergreen
    evergreen_dir = Path(__file__).parent.parent.parent
    filename = evergreen_dir / 'generated_configs' / filename

    with open(filename.resolve(), 'w', encoding='utf-8') as file:
        file.write(yml)


class Dumper(ConfigDumper):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # List all tags on a single line.
        self.FLOW_TAG_COUNT = float('inf')

    # Make an effort to order fields in a readable manner.
    # Ordering applies to *all* mappings regardless of the parent node.
    def represent_mapping(self, tag, mapping, flow_style=False):
        if len(mapping) == 2 and 'key' in mapping and 'value' in mapping:
            flow_style = True

        before = [
            'name',
            'display_name',
            'command',
            'type',
            'run_on',
            'tags',
            'depends_on',
            'binary',
            'working_dir',
        ]

        after = [
            'commands',
            'args',
        ]

        ordered = {
            field: mapping.pop(field) for field in before if field in mapping
        }

        suffix = {
            field: mapping.pop(field) for field in after if field in mapping
        }

        ordered.update(sorted(mapping.items()))
        ordered.update(suffix)

        return self.represent_special_mapping(tag, ordered.items(), flow_style)


def to_yaml(project: EvgProject) -> str:
    return yaml.dump(
        project.dict(exclude_none=True, exclude_unset=True, by_alias=True),
        Dumper=Dumper,
        default_flow_style=False,
        width=float('inf'),
    )


def all_possible(typ: Type[T]) -> Iterable[T]:
    """
    Given a finite type, enumerate all possible values of that type.
    The following are "finite" types:

    - Literal[...] types
    - Union[...] of finite types
    - NamedTuple where each field is a finite type
    - None
    """
    origin = get_origin(typ)
    if typ is type(None):
        yield cast(T, None)
    elif origin is Literal:
        # It is a literal type, so enumerate its literal operands
        yield from get_args(typ)
        return
    elif origin == Union:
        args = get_args(typ)
        yield from itertools.chain.from_iterable(map(all_possible, args))
    elif isclass(typ) and issubclass(typ, tuple):
        # Iter each NamedTuple field:
        fields: Iterable[tuple[str, type]] = get_type_hints(typ).items()
        # Generate lists of pairs of field names and their possible values
        all_pairs: Iterable[Iterable[tuple[str, str]]] = (
            # Generate a (key, opt) pair for each option for 'key'
            [(key, opt) for opt in all_possible(typ)]
            # Over each field and type thereof:
            for key, typ in fields
        )
        # Now generate the cross product of all alternative for all fields:
        matrix: Iterable[dict[str, Any]] = map(dict, itertools.product(*all_pairs))
        for items in matrix:
            # Reconstruct as a NamedTuple:
            yield typ(**items)  # type: ignore
    else:
        raise TypeError(
            f'Do not know how to do "all_possible" of type {typ!r} ({origin=})'
        )
