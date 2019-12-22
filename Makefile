update-test-discovery:
	perl -ne 'print if s/SENTRY_TEST\(([^)]+)\)/XX(\1)/' tests/*.c | sort | uniq > tests/tests.inc
.PHONY: update-test-discovery

build/Makefile: CMakeLists.txt
	mkdir -p build
	cd build; cmake ..

test: build/Makefile update-test-discovery
	$(MAKE) -C build
	./build/sentry_tests
