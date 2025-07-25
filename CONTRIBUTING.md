# Contribution guidelines

We love and welcome contributions!

In order to maintain a high quality, we run a number of checks on
different OS / Compiler combinations, and using different analysis tools.

## Prerequisites

Building and testing `sentry-native` currently requires the following tools:

- **CMake** and a supported C/C++ compiler, to actually build the code.
- **python** and **pytest**, to run integration tests.
- **clang-format** and **black**, to format the C/C++ and python code respectively.
- **curl** and **zlib** libraries (e.g. on Ubuntu: libcurl4-openssl-dev, libz-dev)

`pytest`, `clang-format` and `black` are installed as virtualenv dependencies automatically.

## Setting up Environment

    $ make setup

This sets up both git, including a pre-commit hook and submodules, and installs
a python virtualenv which is used to run tests and formatting.

## Formatting Code

    $ make format

This should be done automatically as part of the pre-commit hook, but can also
be done manually.

    $ black tests

In Powershell on Windows you can use

    $ .\scripts\run_formatters.ps1

to invoke both formatters.

## Running Tests

    $ make test

Creates a python virtualenv, and runs all the tests through `pytest`.

To run our `HTTP` proxy tests, one must add `127.0.0.1  sentry.native.test` to the `hosts` file. This is required since some transports bypass the proxy otherwise (for [example on Windows](https://learn.microsoft.com/en-us/windows/win32/wininet/enabling-internet-functionality#listing-the-proxy-bypass)).

**Running tests on Windows**:

The `make` scripts are not written with Windows in mind, since most Windows users use Visual Studio (Code) or CLion,
which all have elaborate build- and run-configuration capabilities that rarely require to fall back to CLI.

However, if you want to run the tests from Powershell we added a convenience script

    $ .\scripts\run_tests.ps1

that provides the ease of the `make`-based build with a couple of parameters which you can query with

    $ Get-Help .\scripts\run_tests.ps1 -detailed

It depends on `.\scripts\update_test_discovery.ps1` which updates the unit-test index like the `make` target of the same
name.

**Running integration tests manually**:

    $ pytest --verbose --maxfail=1 --capture=no tests/

When all the python dependencies have been installed, the integration test suite
can also be invoked directly.

The `maxfail` parameter will abort after the first failure, and `capture=no`
will print the complete compiler output, and test log.

**Running unit tests**:

    $ make test-unit

Unit tests also have a dedicated `make` target, if they need to be run separately
from the integration tests.

**Running unit tests manually**:

    $ cmake -B build -D CMAKE_RUNTIME_OUTPUT_DIRECTORY=$(pwd)/build
    $ cmake --build build --target sentry_test_unit
    $ ./build/sentry_test_unit

The unit tests are a separate executable target and can be built and run on
their own.

## How to interpret CI failures

The way that tests are run unfortunately does not make it immediately obvious from
the summary what the actual failure is, especially for compile-time failures.
In such cases, it is good to scan the test output _from top to bottom_ and find
the offending compile error.

When running tests locally, one can use the `--maxfail=1` / `-x` parameter to
abort after the first failure.

## Integration Test Parameters

The integration test suite runs the `sentry_example` target using a variety of
different compile-time parameters, and asserts different use-cases.

Some of its behavior is controlled by env-variables:

- `ERROR_ON_WARNINGS`: Turns on `-Werror` for gcc compatible compilers.
  This is also the default for `MSVC` on windows.
- `RUN_ANALYZER`: Runs the code with/through one or more of the given analyzers.
  This accepts a comma-separated list, and currently has support for:
  - `asan`: Uses clangs AddressSanitizer and runs integration tests with the
    `detect_leaks` flag.
  - `scan-build`: Runs the build through the `scan-build` tool.
  - `code-checker`: Uses the [`CodeChecker`](https://github.com/Ericsson/codechecker)
    tool for builds.
  - `kcov`: Uses [`kcov`](https://github.com/SimonKagstrom/kcov) to collect
    code-coverage statistics.
  - `valgrind`: Uses [`valgrind`](https://valgrind.org/) to check for memory
    issues such as leaks.
  - `gcc`: Use the `-fanalyzer` flag of `gcc > 10`.
    This is currently not stable enough to use, as it leads to false positives
    and internal compiler errors.
- `TEST_X86`: Passes flags to CMake to enable a 32-bit (cross-)compile.
- `ANDROID_API` / `ANDROID_NDK` / `ANDROID_ARCH`: Instructs the test runner to
  build using the given Android `NDK` version, targeting the given `API` and
  `ARCH`. The test runner assumes an already running simulator matching the
  `ARCH`, and will run the tests on that.

**Analyzer Requirements**:

Some tools, such as `kcov` and `valgrind` have their own distribution packages.
Clang-based tools may require an up-to-date clang, and a separate `clang-tools`
packages.
`CodeChecker` has its own
[install instructions](https://github.com/Ericsson/codechecker#install-guide)
with a list of needed dependencies.

**Running examples manually**:

    $ cmake -B build -D CMAKE_RUNTIME_OUTPUT_DIRECTORY=$(pwd)/build
    $ cmake --build build --target sentry_example
    $ ./build/sentry_example log capture-event

The example can be run manually with a variety of commands to test different
scenarios. Additionally, it will use the `SENTRY_DSN` env-variable, and can thus
also be used to capture events/crashes directly to sentry.

The example currently supports the following commands:

- `capture-event`: Captures an event.
- `crash`: Triggers a crash to be captured.
- `log`: Enables debug logging.
- `release-env`: Uses the `SENTRY_RELEASE` env-variable for the release,
  instead of a hardcoded value.
- `attachment`: Adds file and byte attachments, which are currently defined as the
  `CMakeCache.txt` file, which is part of the CMake build folder, and a byte array
  named as `bytes.bin`.
- `attach-after-init`: Same as `attachment` but after the SDK has been initialized.
- `stdout`: Uses a custom transport which dumps all envelopes to `stdout`.
- `no-setup`: Skips all scope and breadcrumb initialization code.
- `start-session`: Starts a new release-health session.
- `overflow-breadcrumbs`: Creates a large number of breadcrumbs that overflow
  the maximum allowed number.
- `capture-multiple`: Captures a number of events.
- `sleep`: Introduces a 10 second sleep.
- `add-stacktrace`: Adds the current thread stacktrace to the captured event.
- `disable-backend`: Disables the build-configured crash-handler backend.
- `before-send`: Installs a `before_send()` callback that retains the event.
- `discarding-before-send`: Installs a `before_send()` callback that discards the event.
- `on-crash`: Installs an `on_crash()` callback that retains the crash event.
- `discarding-on-crash`: Installs an `on_crash()` callback that discards the crash event.
- `override-sdk-name`: Changes the SDK name via the options at runtime.
- `stack-overflow`: Provokes a stack-overflow.
- `http-proxy`: Uses a localhost `HTTP` proxy on port 8080.
- `http-proxy-auth`: Uses a localhost `HTTP` proxy on port 8080 with `user:password` as authentication.
- `http-proxy-ipv6`: Uses a localhost `HTTP` proxy on port 8080 using IPv6 notation.
- `proxy-empty`: Sets the `proxy` option to the empty string `""`.
- `socks5-proxy`: Uses a localhost `SOCKS5` proxy on port 1080.
- `capture-transaction`: Captures a transaction.
  - `update-tx-from-header`: Updates the transaction with trace header `"2674eb52d5874b13b560236d6c79ce8a-a0f9fdf04f1a63df"` (`trace_id`-`parent_span_id`).
  - `scope-transaction-event`: Scopes the created transaction and captures an additional event.
- `before-transaction`: Installs a `before_transaction()` callback that updates the transaction title.
- `discarding-before-transaction`: Installs a `before_transaction()` callback that discards the transaction.
- `traces-sampler`: Installs a traces sampler callback function when used alongside `capture-transaction`.
- `attach-view-hierarchy`: Adds a `view-hierarchy.json` attachment file, giving it the proper `attachment_type` and `content_type`.
 This file can be found in `./tests/fixtures/view-hierachy.json`.
- `set-trace`: Sets the scope `propagation_context`'s trace data to the given `trace_id="aaaabbbbccccddddeeeeffff00001111"` and `parent_span_id=""f0f0f0f0f0f0f0f0"`.
- `capture-with-scope`: Captures an event with a local scope.
- `attach-to-scope`: Same as `attachment` but attaches the file to the local scope.
- `clear-attachments`: Clears all attachments from the global scope.
- `capture-user-feedback`: Captures a user feedback event.

Only on Linux using crashpad:
- `crashpad-wait-for-upload`: Couples application shutdown to complete the upload in the `crashpad_handler`.

Only on Windows using crashpad with its WER handler module:

- `fastfail`: Crashes the application using the `__fastfail` intrinsic directly, thus by-passing SEH.
- `stack-buffer-overrun`: Triggers the Windows Control Flow Guard, which also fast fails and in turn by-passes SEH.

## Running Benchmarks

    $ make benchmark

Creates a python virtualenv, and runs all the benchmarks through `pytest`.

**Running benchmarks manually**:

    $ pytest --verbose --capture=no tests/benchmark.py

When all the python dependencies have been installed, the benchmarks can also be
invoked directly.

## Handling locks

There are a couple of rules based on the current usage of mutexes in the Native SDK that should always be 
applied in order not to have to fight boring concurrency bugs:

* we use recursive mutexes throughout the code-base
* these primarily allow us to call public interfaces from internal code instead of having a layer in-between
* but they come at the risk of less clarity whether a lock release still leaves a live lock 
* they should not be considered as convenience:
  * reduce the amount of recursive locking to an absolute minimum
  * instead of retrieval via global locks, pass shared state like `options` or `scope` around in internal helpers 
  * or better yet: extract what you need into locals, then release the lock early
* we provide lexical scope macros `SENTRY_WITH_OPTIONS` and `SENTRY_WITH_SCOPE` (and variants) as convenience wrappers
* if you use them be aware of the following:
  * as mentioned above, while the macros are convenience, their lexical scope should be as short as possible
  * avoid nesting them unless strictly necessary
  * if you nest them (directly or via callees), the `options` lock **must always be acquired before** the `scope` lock
  * never early-return or jump (via `goto` or `return`) from within a `SENTRY_WITH_*` block: doing so skips the corresponding release or cleanup
  * in particular, since `options` are readonly after `sentry_init()` the lock is only acquired to increment the refcount for the duration of `SENTRY_WITH_OPTIONS`
  * however, `SENTRY_WITH_SCOPE` (and variants) always hold the lock for the entirety of their lexical scope