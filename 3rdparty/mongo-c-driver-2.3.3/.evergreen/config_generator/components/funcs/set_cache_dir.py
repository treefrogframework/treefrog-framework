from shrub.v3.evg_command import EvgCommandType, expansions_update

from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class SetCacheDir(Function):
    name = 'set-cache-dir'
    commands = [
        bash_exec(
            command_type=EvgCommandType.SETUP,
            script="""\
                if [[ -n "$XDG_CACHE_DIR" ]]; then
                    cache_dir="$XDG_CACHE_DIR" # XDG Base Directory specification.
                elif [[ -n "$LOCALAPPDATA" ]]; then
                    cache_dir="$LOCALAPPDATA" # Windows.
                elif [[ -n "$USERPROFILE" ]]; then
                    cache_dir="$USERPROFILE/.cache" # Windows (fallback).
                elif [[ -d "$HOME/Library/Caches" ]]; then
                    cache_dir="$HOME/Library/Caches" # MacOS.
                elif [[ -n "$HOME" ]]; then
                    cache_dir="$HOME/.cache" # Linux-like.
                elif [[ -d ~/.cache ]]; then
                    cache_dir="~/.cache" # Linux-like (fallback).
                else
                    cache_dir="$(pwd)/.cache" # EVG task directory (fallback).
                fi

                mkdir -p "$cache_dir/mongo-c-driver" || exit
                cache_dir="$(cd "$cache_dir/mongo-c-driver" && pwd)" || exit

                printf "MONGO_C_DRIVER_CACHE_DIR: %s\\n" "$cache_dir" >|expansions.set-cache-dir.yml
            """,
        ),
        expansions_update(command_type=EvgCommandType.SETUP, file='expansions.set-cache-dir.yml'),
    ]


def functions():
    return SetCacheDir.defn()
