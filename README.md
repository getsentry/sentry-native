# Sentry-Native <!-- omit in toc -->

Sentry-Native is a wrapper around two most popular crash-reporting frameworks: [Breakpad](https://chromium.googlesource.com/breakpad/breakpad/) and [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md). It provides an abstraction layer that unlocks certain Sentry features, such as setting tags and adding breadcrumbs to your crashes. Sentry-Native is also responsible for sending the crashes directly to Sentry in an efficient manner.

Platforms we are going to support eventually: MacOS X, Linux (32/64 bit), Windows (32/64 bit), Android, iOS.

## Table of Contents <!-- omit in toc -->

- [Downloads](#downloads)
  - [What is inside](#what-is-inside)
- [Build Prerequisites](#build-prerequisites)
  - [MacOS](#macos)
  - [Windows](#windows)
- [Building Crashpad + Sentry-Native](#building-crashpad--Sentry-Native)
  - [MacOS](#macos-1)
  - [Windows](#windows-1)
  - [Sample Application](#sample-application)
- [Development: Generating Build Files](#development-generating-build-files)
  - [Additional Dependencies](#additional-dependencies)
  - [MacOS](#macos-2)
  - [Windows](#windows-2)

## Downloads

Packages with source code can be downloaded from the [release](https://github.com/getsentry/sentry-native/releases) page.

### What is inside

At the moment, we package and distribute the following components:

- Sentry-Native source code
- Crashpad source code
- Build files for MacOS ("make" files) and Windows (Visual Studio solution)

If any of the provided build files do not work for you, please refer to [Development: Generating Build Files](#development-generating-build-files)

## Build Prerequisites

In order to build the provided source files, you will need the following software:

### MacOS

- [Xcode](https://developer.apple.com/xcode/) (can be installed from Mac App Store)
- [GNU make](https://www.gnu.org/software/make/)
- [clang](https://clang.llvm.org/)

### Windows

- Microsoft Visual Studio 2017 (or later)

## Building Crashpad + Sentry-Native

**WARNING:** The following commands will work for the **packaged** version of Sentry-Native (see [Downloads](#downloads)). We generate `gen_macosx` and `gen_windows` during the CI build, so you will _not_ find them in the git repository.

### MacOS

```sh
cd gen_macosx
make
```

The command will build both Crashpad and Sentry-Native libraries, as well as an application example (`bin/Release/sentry_example_crashpad`).

### Windows

Visual Studio solution is located at `gen_windows/Sentry-Native.sln` and contains projects for both Sentry-Native and Crashpad.

**WARNING:** There is a known issue that the Windows SDK version configured in the solution might not be present on your system. If you see `"The Windows SDK version XXX was not found"` error, you can try to "Retarget Solution" ("Project" -> "Retarget solution"). You can also regenerate build files on the target machine, for that please consult the [development section](#development-generating-build-files).

### Sample Application

The build commands will produce Sentry-Native and Crashpad libraries (e.g. `./bin/Release/libsentry_crashpad.dylib` on MacOS), and a sample crashing application. You can run it by passing your Sentry DSN key via an environment variable:

```sh
# MacOS
SENTRY_DSN=https://XXXXX@sentry.io/YYYYY ./bin/Release/sentry_example_crashpad
```

The command will result into a segmentation fault, and a new event will be sent to Sentry. If [debug symbols are present](https://docs.sentry.io/cli/dif/) in Sentry (dSYM or PDB files, they are generated automatically in the output directory), the event will also be properly symbolicated.

## Development: Generating Build Files

If you want to develop/test Sentry-Native locally, or the distribution package doesn't fit your needs, you can run generate build files right from the `premake/` directory. We use [premake5](https://premake.github.io/download.html#v5) to prepare build files.

### Additional Dependencies

- [premake5](https://premake.github.io/download.html#v5): we use `premake` to generate build files.

### MacOS

```sh
cd premake/
premake5 gmake2
```

...will create a set of `Makefile`s that you can use to build any part of the project.

Use `make help` to see all available targets.

### Windows

```sh
cd premake/
premake5 vs2017
```

The last command will create a Visual Studio 2017 solution (`Sentry-Native.sln`) that contains Crashpad and Sentry-Native library projects, and a `sentry_example_crashpad` example project.
