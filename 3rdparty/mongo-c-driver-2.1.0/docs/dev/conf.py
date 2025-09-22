# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

from pathlib import Path
import re
from typing import Callable

from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.environment import BuildEnvironment

THIS_FILE = Path(__file__).resolve()
THIS_DIR = THIS_FILE.parent
REPO_ROOT = THIS_DIR.parent.parent

project = "MongoDB C Driver Development"
copyright = "2009-present, MongoDB, Inc."
author = "MongoDB, Inc"
release = (REPO_ROOT / "VERSION_CURRENT").read_text().strip()

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = []
templates_path = []
exclude_patterns = []
default_role = "any"

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "nature"
pygments_style = "sphinx"
html_static_path = []

rst_prolog = rf"""
.. role:: bash(code)
    :language: bash
"""


def annotator(
    annot: str,
) -> Callable[[BuildEnvironment, str, addnodes.desc_signature], str]:
    """
    Create a parse_node function that adds a parenthesized annotation to an object signature.
    """

    def parse_node(
        env: BuildEnvironment, sig: str, signode: addnodes.desc_signature
    ) -> str:
        signode += addnodes.desc_name(sig, sig)
        signode += addnodes.desc_sig_space()
        signode += addnodes.desc_annotation("", f"({annot})")
        return sig

    return parse_node


def parse_earthly_artifact(
    env: BuildEnvironment, sig: str, signode: addnodes.desc_signature
) -> str:
    """
    Parse and render the signature of an '.. earthly-artifact::' signature"""
    mat = re.match(r"(?P<target>\+.+?)(?P<path>/.*)$", sig)
    if not mat:
        raise RuntimeError(
            f"Invalid earthly-artifact signature: {sig!r} (expected “+<target>/<path> string)"
        )
    signode += addnodes.desc_addname(mat["target"], mat["target"])
    signode += addnodes.desc_name(mat["path"], mat["path"])
    signode += addnodes.desc_sig_space()
    signode += addnodes.desc_annotation("", "(Earthly Artifact)")
    return sig


def setup(app: Sphinx):
    app.add_object_type(  # type: ignore
        "earthly-target",
        "earthly-target",
        indextemplate="pair: earthly target; %s",
        parse_node=annotator("Earthly target"),
    )
    app.add_object_type(  # type: ignore
        "script",
        "script",
        indextemplate="pair: shell script; %s",
        parse_node=annotator("shell script"),
    )
    app.add_object_type(  # type: ignore
        "earthly-artifact",
        "earthly-artifact",
        indextemplate="pair: earthly artifact; %s",
        parse_node=parse_earthly_artifact,
    )
    app.add_object_type(  # type: ignore
        "file",
        "file",
        indextemplate="repository file; %s",
        parse_node=annotator("repository file"),
    )
