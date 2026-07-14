all: test

GIT_COMMON_DIR := $(shell git rev-parse --git-common-dir)

VENV_BIN = $(if $(filter Windows_NT,$(OS)),.venv/Scripts,.venv/bin)
LOGGER_SENTRY_BACKEND ?= $(or $(SENTRY_BACKEND),none)
LOGGER_SENTRY_TRANSPORT ?= $(or $(SENTRY_TRANSPORT),none)
SENTRY_BATCHER_BUFFER_COUNT ?= 2
SENTRY_BATCHER_BUFFER_SIZE ?= 100
SENTRY_BATCHER_TIMING ?= OFF

update-test-discovery:
	@perl -ne 'print if s/SENTRY_TEST\(([^)]+)\).*/XX(\1)/' tests/unit/*.c | LC_ALL=C sort | grep -v define | uniq > tests/unit/tests.inc
.PHONY: update-test-discovery

build/Makefile: CMakeLists.txt
	@mkdir -p build
	@cd build; cmake ..

build: build/Makefile
	@cmake --build build --parallel
.PHONY: build

test: update-test-discovery test-integration
.PHONY: test

test-unit: update-test-discovery CMakeLists.txt
	@mkdir -p unit-build
	@cd unit-build; cmake \
		-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$(PWD)/unit-build \
		-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO=$(PWD)/unit-build \
		-DSENTRY_BACKEND=none \
		..
	@cmake --build unit-build --target sentry_test_unit --parallel --config RelWithDebInfo
	./unit-build/sentry_test_unit
.PHONY: test-unit

test-integration: setup-venv
	$(VENV_BIN)/pytest tests --verbose
.PHONY: test-integration

test-leaks: update-test-discovery CMakeLists.txt
	@mkdir -p leak-build
	@cd leak-build; cmake \
		-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$(PWD)/leak-build \
		-DSENTRY_BACKEND=none \
		-DWITH_ASAN_OPTION=ON \
		-DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang \
		-DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ \
		-DCMAKE_LINKER=/usr/local/opt/llvm/bin/clang \
		..
	@cmake --build leak-build --target sentry_test_unit --parallel
	@ASAN_OPTIONS=detect_leaks=1 ./leak-build/sentry_test_unit
.PHONY: test-leaks

benchmark: setup-venv
	$(VENV_BIN)/pytest tests/benchmark.py --verbose
.PHONY: benchmark

LOGGER_BUILD_STAMP = logger-build/.stamp-$(LOGGER_SENTRY_BACKEND)-$(LOGGER_SENTRY_TRANSPORT)-$(SENTRY_BATCHER_BUFFER_COUNT)-$(SENTRY_BATCHER_BUFFER_SIZE)-$(SENTRY_BATCHER_TIMING)

ifneq ($(filter test-logger,$(firstword $(MAKECMDGOALS))),)
LOGGER_GOAL_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
LOGGER_ARGS ?= $(LOGGER_GOAL_ARGS)
$(LOGGER_GOAL_ARGS):
	@:
endif

$(LOGGER_BUILD_STAMP): FORCE_LOGGER_CONFIG CMakeLists.txt tests/logs/logger.cpp
	@mkdir -p logger-build
	@cd logger-build; cmake \
		-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$(PWD)/logger-build \
		-DSENTRY_BACKEND=$(LOGGER_SENTRY_BACKEND) \
		-DSENTRY_TRANSPORT=$(LOGGER_SENTRY_TRANSPORT) \
		-DSENTRY_BATCHER_BUFFER_COUNT=$(SENTRY_BATCHER_BUFFER_COUNT) \
		-DSENTRY_BATCHER_BUFFER_SIZE=$(SENTRY_BATCHER_BUFFER_SIZE) \
		-DSENTRY_BATCHER_TIMING=$(SENTRY_BATCHER_TIMING) \
		-DSENTRY_BUILD_EXAMPLES=ON \
		..
	@touch $@

FORCE_LOGGER_CONFIG:
.PHONY: FORCE_LOGGER_CONFIG

test-logger: $(LOGGER_BUILD_STAMP)
	@cmake --build logger-build --target sentry_logger --parallel --config RelWithDebInfo
	@./logger-build/sentry_logger $(LOGGER_ARGS)
.PHONY: test-logger

clean: build/Makefile
	@$(MAKE) -C build clean
.PHONY: clean

setup: setup-git setup-venv
.PHONY: setup

setup-git: $(GIT_COMMON_DIR)/hooks/pre-commit
	git submodule update --init --recursive
.PHONY: setup-git

setup-venv: .venv/.stamp
.PHONY: setup-venv

$(GIT_COMMON_DIR)/hooks/pre-commit:
	@cd $(GIT_COMMON_DIR)/hooks && ln -sf $(PWD)/scripts/git-precommit-hook.sh pre-commit

.venv/.stamp: Makefile tests/requirements.txt
	@rm -rf .venv
	python3 -m venv .venv
	$(VENV_BIN)/pip install --upgrade --requirement tests/requirements.txt
	@touch $@

format: setup-venv
	@find examples include src tests/unit \
		\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \) -print0 \
		| xargs -0 $(VENV_BIN)/clang-format -i
	@$(VENV_BIN)/black tests
.PHONY: format

style: setup-venv
	@$(VENV_BIN)/python ./scripts/check-clang-format.py \
		--clang-format-executable $(VENV_BIN)/clang-format \
		-r examples include src tests/unit
	@$(VENV_BIN)/black --diff --check tests
.PHONY: style
