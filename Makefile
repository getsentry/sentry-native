export SENTRY_NATIVE_PYTHON_VERSION := python3.7

all: test

update-test-discovery:
	@perl -ne 'print if s/SENTRY_TEST\(([^)]+)\)/XX(\1)/' tests/unit/*.c | sort | uniq > tests/unit/tests.inc
.PHONY: update-test-discovery

build/Makefile: CMakeLists.txt
	@mkdir -p build
	@cd build; cmake ..

build: build/Makefile
	@cmake --build build --parallel
.PHONY: build

build/sentry_tests: build
	@cmake --build build --target sentry_tests --parallel

test: update-test-discovery test-integration
.PHONY: test

test-integration: setup-venv
	.venv/bin/pytest tests --verbose
.PHONY: test-integration

test-leaks: update-test-discovery CMakeLists.txt
	@mkdir -p leak-build
	@cd leak-build; cmake -DWITH_ASAN_OPTION=ON -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_LINKER=/usr/local/opt/llvm/bin/clang ..
	@cmake --build leak-build --target sentry_tests --parallel
	@ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=leak-suppressions.txt ./leak-build/sentry_tests
.PHONY: test-leaks

clean: build/Makefile
	@$(MAKE) -C build clean
.PHONY: clean

setup: setup-git
.PHONY: setup

setup-git:
	git submodule update --init --recursive
.PHONY: setup-git

setup-venv: .venv/bin/python
.PHONY: setup-venv

.venv/bin/python: Makefile integration-test-requirements.txt
	@rm -rf .venv
	@which virtualenv || sudo pip install virtualenv
	virtualenv -p $$SENTRY_NATIVE_PYTHON_VERSION .venv
	.venv/bin/pip install --upgrade --requirement integration-test-requirements.txt

format:
	@clang-format -i src/*.c src/*.h src/*/*.c src/*/*.h tests/*.c tests/*.h
.PHONY: format
