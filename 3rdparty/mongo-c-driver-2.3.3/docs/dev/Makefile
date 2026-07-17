.SILENT:

.PHONY: poetry-install html serve

default: html

THIS_FILE := $(realpath $(lastword $(MAKEFILE_LIST)))
THIS_DIR := $(shell dirname $(THIS_FILE))
MONGOC_DIR := $(shell dirname $(shell dirname $(THIS_DIR)))

BUILD_DIR := $(MONGOC_DIR)/_build

SPHINX_JOBS ?= auto
SPHINX_ARGS := -W -n -j "$(SPHINX_JOBS)" -a -b dirhtml

DOCS_SRC := $(THIS_DIR)
DOCS_OUT := $(BUILD_DIR)/docs/dev/html
html:
	uv run --frozen sphinx-build $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)

serve:
	uv run --frozen --with sphinx-autobuild sphinx-autobuild $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)
