"""
Run clang-format and header-fixup on the source code for the repository.

By default this script runs `clang-format` over most source files in the
repository (excluding some vendored code that we don't want to format).
Alteratively, a list of files can be given as positional arguments to
selectively format files. `--help` for more details.

It also fixes up `#include` directives to use angle bracket syntax if they have
a certain spelling. (See `INCLUDE_RE` in the script)
"""

import argparse
import functools
import itertools
import multiprocessing
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Iterable, Literal, Sequence


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    # By default, add two jobs to the CPU count since some work is waiting on disk
    dflt_jobs = multiprocessing.cpu_count() + 2
    parser.add_argument(
        "--jobs",
        "-j",
        type=int,
        help=f"Number of parallel jobs to run (default: {dflt_jobs})",
        metavar="<num-jobs>",
        default=dflt_jobs,
    )
    parser.add_argument(
        "--mode",
        choices=RunMode.__args__,
        help="Whether to apply changes, or simply check for formatting violations (default: apply)",
        default="apply",
    )
    parser.add_argument(
        "--clang-format-bin",
        help="The clang-format executable to be used (default: “clang-format”)",
        default="clang-format",
        metavar="<executable>",
    )
    parser.add_argument(
        "files",
        metavar="<filepath>",
        nargs="*",
        help="List of files to be selected for formatting. If omitted, the default set of files are selected",
    )
    args = parser.parse_args(argv)
    mode: RunMode = args.mode
    file_patterns: list[str] = args.files
    cf: str = args.clang_format_bin
    # Convert filepath patterns to a list of paths
    files: list[Path]
    try:
        match file_patterns:
            case []:
                files = list(all_our_sources())
            case patterns:
                files = [Path(p).resolve() for p in patterns]
    except Exception as e:
        raise RuntimeError("Failed to collect files for formatting (See above)") from e
    # Fail if no files matched
    assert files
    # Split the file list into groups to be dispatched
    num_jobs: int = min(args.jobs, len(files))
    groups = [files[n::num_jobs] for n in range(num_jobs)]
    print(f"Formatting {len(files)} files with {num_jobs} workers...", file=sys.stderr)

    # Bind the formatting arguments to the formatter function
    format_group = functools.partial(_format_files, mode=mode, clang_format=cf)
    # Run in a thread pool. Even though Python is single-threaded, most work will
    # be waiting on clang-format or disk I/O
    pool = ThreadPoolExecutor(max_workers=num_jobs)
    try:
        okay = all(pool.map(format_group, groups))
    except Exception as e:
        raise RuntimeError("Unexpected error while formatting files (See above)") from e
    if not okay:
        return 1
    return 0


RunMode = Literal["apply", "check"]
"Whether we should apply changes, or just check for violations"

#: This regex tells us which #include directives should be modified to use angle brackets
#: The regex is written to preserve whitespace and surrounding context. re.VERBOSE
#: allows us to use verbose syntax with regex comments.
INCLUDE_RE = r"""
    # Start of line
    ^

    # The #include directive
    (?P<directive>
        \s*  # Any whitespace at start of line
        [#] \s* include  # The "#" and "include", with any whitespace between
        \s+  # One ore more whitespace following "include"
    )

    " # Open quote
    # Match any path that does not start with a dot
    (?P<path> [^.] .*?)
    " # Close quote

    # Everything else on the line
    (?P<tail>.*)

    # End of line
    $
    """

REPO_DIR = Path(__file__).parent.parent.resolve()
"""
Directory for the root of the repository.

This path is constructed based on the path to this file itself, so moving this
script should modify the above expression
"""

SOURCE_PATTERNS = [
    "**/*.h",
    "**/*.hpp",
    "**/*.c",
    "**/*.cpp",
]
"""
Recursive source file patterns, based on file extensions.
"""

SOURCE_DIRS = [
    "src/common",
    "src/libbson",
    "src/libmongoc",
    "tests",
]
"""
Directories that contain our own source files (not vendored code)
"""

EXCLUDE_SOURCES = [
    "src/libbson/src/jsonsl/**/*",
]
"""
Globbing patterns that select files that are contained in our source directories,
but are vendored third-party code.
"""


def all_our_sources() -> set[Path]:
    """
    Obtain a set of all first-party source files in the repository.
    """
    # Every file in our main source dirs:
    everything = set(
        itertools.chain.from_iterable(
            REPO_DIR.joinpath(subdir).glob(p) for p in SOURCE_PATTERNS for subdir in SOURCE_DIRS
        )
    )
    # Every file in our main source dirs that we want to exclude:
    excluded = set(itertools.chain.from_iterable(REPO_DIR.glob(exc) for exc in EXCLUDE_SOURCES))
    # Remove excluded from everything. Return that.
    return everything - excluded


def _include_subst_fn(fpath: Path):
    "Create a regex substitution function that prints a message for the file when a substitution is made"

    parent_dir = fpath.parent

    def f(mat: re.Match[str]) -> str:
        # See groups in INCLUDE_RE
        target = mat["path"]
        abs_target = parent_dir / target
        if abs_target.is_file():
            # This should be a relative include:
            newl = f'{mat["directive"]}"./{target}"{mat["tail"]}'
        else:
            newl = f"{mat['directive']}<{target}>{mat['tail']}"
        print(f" - {fpath}: update #include directive: {mat[0]!r} → {newl!r}", file=sys.stderr)
        return newl

    return f


def _fixup_includes(fpath: Path, *, mode: RunMode) -> bool:
    """
    Apply #include-fixup to the content of the given source file.
    """
    # Split into lines
    old_lines = fpath.read_text().split("\n")
    # Do a regex substitution on ever line:
    rx = re.compile(INCLUDE_RE, re.VERBOSE)
    new_lines = [rx.sub(_include_subst_fn(fpath), ln) for ln in old_lines]
    # Did we change anything?
    did_change = new_lines != old_lines
    # We changed something. What do we do next?
    match did_change, mode:
        case False, _:
            # No file changes. Nothing to do
            return True
        case _, "apply":
            # We are applying changes. Write the lines back into the file and tell
            # the caller that we succeeded
            fpath.write_text("\n".join(new_lines), newline="\n")
            return True
        case _, "check":
            # File changes, and we are only checking. Print an error message and indicate failure to the caller
            print(f"File [{fpath}] contains improper #include directives", file=sys.stderr)
            return False


def _format_files(files: Iterable[Path], *, mode: RunMode, clang_format: str) -> bool:
    """
    Run clang-format on some files, and fixup the #includes in those files.
    """

    def fixup_one(p: Path) -> bool:
        try:
            return _fixup_includes(p, mode=mode)
        except Exception as e:
            raise RuntimeError(f"Unexpected error while fixing-up the #includes on file [{p}] (See above)") from e

    # First update the `#include` directives, since that can change the sort order
    # that clang-format might want to apply
    if not all(list(map(fixup_one, files))):
        return False

    # Whether we check for format violations or modify the files in-place
    match mode:
        case "apply":
            mode_args = ["-i"]
        case "check":
            mode_args = ["--dry-run", "-Werror"]
    cmd = [clang_format, *mode_args, *map(str, files)]
    try:
        res = subprocess.run(cmd, check=False, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    except Exception as e:
        raise RuntimeError(f"Failed to spawn [{clang_format}] process for formatting files (See above)") from e
    sys.stderr.buffer.write(res.stdout)
    return res.returncode == 0


def _get_files_matching(pat: str) -> Sequence[Path]:
    """
    Obtain files according to a globbing pattern. Checks that at least one file
    matches.
    """

    try:
        if os.path.isabs(pat):
            # Given an absolute path, glob relative to the root directory
            root = Path(pat).root
            ret = tuple(Path(root).glob(str(Path(pat).relative_to(root))))
        else:
            # None-relative path, glob relative to CWD
            ret = tuple(Path.cwd().glob(pat))
    except Exception as e:
        raise RuntimeError(f"Failed to collect files for globbing pattern: “{pat}” (See above)") from e
    if not ret:
        raise RuntimeError(f"Globbing pattern “{pat}” did not match any files")
    return ret


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
