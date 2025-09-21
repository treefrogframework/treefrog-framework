# Generating the Evergreen Configuration Files

The scripts in this directory are used to generate the Evergreen configuration
files stored in `.evergreen/generated_configs/`.

The easiest way to execute these scripts is to use [uv](https://docs.astral.sh/uv/) to run the scripts.

**Note**: These scripts require Python 3.10 or newer.


## Setting Up

`uv` is required to run Python scripts. See ["Installing uv"](https://docs.astral.sh/uv/getting-started/installation/) for instructions on how to obtain `uv`.


## Running the Generator

The package provides the `mc-evg-generate` [entry point](https://packaging.python.org/en/latest/specifications/entry-points):

```sh
uv run --frozen mc-evg-generate
```

This command will ready the generation files and generate a new set of Evergreen
configs in `.evergreen/config_generator/generated_configs`.

The `mc-evg-generate` command executes `main` function defined in
`config_generator/generate.py`.


## Modifying the Configs

To modify the Evergreen configuration, update the Python scripts within the
`etc/`, `config_generator/`, and `legacy_config_generator/` directories, and
execute `mc-evg-generate` again.
