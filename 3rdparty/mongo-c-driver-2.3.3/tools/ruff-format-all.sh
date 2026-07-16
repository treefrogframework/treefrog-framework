#!/usr/bin/env bash
#
# format.sh
#
# Usage:
#   uv run --frozen etc/ruff-format-all.sh
#
# This script is meant to be run from the project root directory.

set -o errexit
set -o pipefail

# Vendored scripts to keep in sync with upstream or external sources.
excludes=(
  --exclude build/bottle.py
  --exclude build/mongodl.py
)

# Scripts which require a different Python version than the one specified in pyproject.toml.
# See: https://github.com/astral-sh/ruff/issues/10457
py312=(
  src/libbson/tests/validate-tests.py
)

# Format imports: https://github.com/astral-sh/ruff/issues/8232
uv run --frozen --group format-scripts --isolated ruff check --select I --fix --target-version py312 "${py312[@]:?}"
uv run --frozen --group format-scripts ruff check --select I --fix --exclude "${py312[@]:?}" "${excludes[@]:?}"

# Format Python scripts.
uv run --frozen --group format-scripts ruff format "${excludes[@]:?}"
