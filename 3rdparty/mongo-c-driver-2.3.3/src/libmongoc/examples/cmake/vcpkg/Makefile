.SILENT:

THIS_FILE := $(realpath $(lastword $(MAKEFILE_LIST)))
THIS_DIR := $(dir $(THIS_FILE))

VCPKG_ROOT ?= /opt/vcpkg

.PHONY: test-classic
test-classic:
	vcpkg install mongo-c-driver
	rm -rf -- $(THIS_DIR)/_build
	cmake \
		-S . \
		-B _build \
		-D CMAKE_PROJECT_INCLUDE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
	cmake --build _build --config=Debug
	cmake --build _build --config=Debug --target test

.PHONY: test-manifest-mode
test-manifest-mode:
	test -f vcpkg.json \
		|| (echo "Expects a vcpkg.json file in the project directory before running this target." 1>&2 \
			&& false)
	cmake \
		-S . \
		-B _build \
		-D CMAKE_PROJECT_INCLUDE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
	cmake --build _build --config=Debug
	cmake --build _build --config=Debug --target test
