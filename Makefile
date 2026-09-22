PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

EXT_NAME=system_stats
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

.DEFAULT_GOAL := reldebug

include extension-ci-tools/makefiles/duckdb_extension.Makefile

CMAKE_FILES := CMakeLists.txt extension_config.cmake

format-all: format
	cmake-format -i $(CMAKE_FILES)
	cargo fmt --all

.PHONY: format-all
