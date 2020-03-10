<p align="center">
  <a href="https://sentry.io" target="_blank" align="center">
    <img src="https://sentry-brand.storage.googleapis.com/sentry-logo-black.png" width="280">
  </a>
  <br />
</p>

# Official Sentry SDK for C/C++ <!-- omit in toc -->

The _Sentry Native SDK_ is an error and crash reporting client for native
applications, optimized for C and C++. Sentry allows to add tags, breadcrumbs
and arbitrary custom context to enrich error reports.

**Note**: This SDK is being actively developed and still in Beta. We recommend
to check for updates regularly to benefit from latest features and bug fixes.
Please see [Known Issues](#known-issues).

## Table of Contents <!-- omit in toc -->

- [Downloads](#downloads)
  - [What is Inside](#what-is-inside)
- [Platform and Feature Support](#platform-and-feature-support)
- [Building and Installation](#building-and-installation)
  - [Compile-Time Options](#compile-time-options)
  - [Build Targets](#build-targets)
- [Known Issues](#known-issues)
- [Sample Application](#sample-application)
- [Development](#development)
  - [Running Tests](#running-tests)

## Downloads

The SDK can be downloaded from the [Releases] page, which also lists the
changelog of every version.

[releases]: https://github.com/getsentry/sentry-native/releases

### What is Inside

The SDK bundle contains the following folders:

- `external`: These are external projects which are consumed via
  `git submodules`.
- `include`: Contains the Sentry header file. Set the include path to this
  directory or copy the header file to your source tree so that it is available
  during the build.
- `src`: Sources of the Sentry SDK required for building.

## Platform and Feature Support

The SDK currently supports and is tested on Linux, Windows and macOS as well as
Android.

The support target for Android is as low as API 16, with build support on
NDK 19, which is also verified via tests.

The SDK supports different features on the target platform:

- **HTTP Transport** is currently only supported on Linux and macOS,
  using `libcurl`. On other platforms, library users need to implement their
  own transport, based on the `function transport` API.
- **Crashpad Backend** is currently only supported on Windows and macOS.

## Building and Installation

The SDK is developed and shipped as a [CMake] project.
CMake will pick an appropriate compiler and buildsystem toolchain automatically
per platform, and can also be configured for cross-compilation.
System-wide installation of the resulting sentry library is also possible via
CMake.

Building the Crashpad Backend requires a `C++14` compatible compiler.

**Build example**:

```sh
# configure the cmake build into the `build` directory, with crashpad (on macOS)
$ cmake -B build -DSENTRY_BACKEND=crashpad
# build the project
$ cmake --build build --parallel
# install the resulting artifacts into a specific prefix
$ cmake --install build --prefix install
# which will result in the following (on macOS):
$ exa --tree install
install
├── bin
│  └── crashpad_handler
├── include
│  └── sentry.h
└── lib
   ├── libsentry.dylib
   └── libsentry.dylib.dSYM
```

Please refer to the CMake Manual for more details.

**Android**:

The CMake project can also be configured to correctly work with the Android NDK,
see the dedicated [CMake Guide] for details on how to integrate it with gradle
or use it on the command line.

[cmake]: https://cmake.org/cmake/help/latest/
[cmake guide]: https://developer.android.com/ndk/guides/cmake

### Compile-Time Options

The following options can be set when running the cmake generator, for example
using `cmake -D BUILD_SHARED_LIBS=OFF ..`.

- `BUILD_SHARED_LIBS` (Default: ON):
  By default, `sentry` is built as a shared library. Setting this option to
  `OFF` will build `sentry` as a static library instead.
- `SENTRY_BACKEND` (Default: depending on platform):
  Sentry can use different backends depending on platform.
  **crashpad**: This uses the out-of-process crashpad handler. It is currently
  only supported on Windows and macOS, and used as the default there.
  **inproc**: A small in-process handler which is supported on all platforms
  except Windows, and is used as default on Linux and Android.
  **none**: This builds `sentry-native` without a backend, so it does not handle
  crashes at all. It is primarily used for tests.

### Build Targets

- `sentry`: This is the main library and the only default build target.
- `crashpad_handler`: When configured with the `crashpad` backend, this is
  the out of process crash handler, which will need to be installed along with
  the projects executable.
- `sentry_test_unit`: These are the main unit-tests, which are conveniently built
  also by the toplevel makefile.
- `sentry_example`: This is a small example program highlighting the API, which
  can be controlled via command-line parameters, and is also used for
  integration tests.

## Known Issues

- Sentry with Crashpad cannot upload minidumps on Linux. This is due to a
  limitation in the upstream Crashpad project.
- Sentry with Breakpad cannot upload minidumps. The uploader was temporarily
  removed and will be restored in a future version.
- Attachments are currently in _Preview_ and may not be available to your
  organization. Please see [Event Attachments] for more information.

[event attachments]: https://docs.sentry.io/platforms/native/#event-attachments-preview

## Development

Some external dependencies, such as `crashpad` are used via `git submodules`.
When working with this repository, make sure to check out the submodules
recursively.

    $ git clone --recurse-submodules https://github.com/getsentry/sentry-native.git

Or when working with an existing clone:

    $ git submodule update --init --recursive

Alternatively, the `make setup` target will do that for you, and might be
extended in the future if other dependencies are added.

### Running Tests

The SDK ships with a unit-test suite based on [acutest]. Additionally, the
`sentry_example` executable is also used for integration tests which are run
via `pytest`.
The unit-tests are built as a separate `sentry_test_unit` executable.

On macOS and Linux, the _top-level Makefile_ contains a convenience command to
run tests:

```sh
make test
```

[acutest]: https://github.com/mity/acutest
