all: test

update-test-discovery:
	@perl -ne 'print if s/SENTRY_TEST\(([^)]+)\)/XX(\1)/' tests/*.c | sort | uniq > tests/tests.inc
.PHONY: update-test-discovery

build/Makefile: CMakeLists.txt
	@mkdir -p build
	@cd build; cmake ..

build: build/Makefile
	@$(MAKE) -C build
.PHONY: build

build/sentry_tests: build
	@$(MAKE) -C build sentry_tests

test: update-test-discovery build/sentry_tests
	@./build/sentry_tests
.PHONY: test

test-leaks: update-test-discovery CMakeLists.txt
	@mkdir -p leak-build
	@cd leak-build; cmake -DWITH_ASAN_OPTION=ON -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_LINKER=/usr/local/opt/llvm/bin/clang ..
	@$(MAKE) -C leak-build sentry_tests
	@ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=leak-suppressions.txt ./leak-build/sentry_tests
.PHONY: test-leaks

clean: build/Makefile
	@$(MAKE) -C build clean
.PHONY: clean

format:
	@clang-format -i src/*.c src/*.h src/*/*.c src/*/*.h tests/*.c tests/*.h
.PHONY: format
