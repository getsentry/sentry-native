<p align="center">
  <a href="https://sentry.io" target="_blank" align="center">
    <img src="https://sentry-brand.storage.googleapis.com/sentry-logo-black.png" width="280">
  </a>
  <br />
</p>

# Official Sentry SDK for C/C++ <!-- omit in toc -->

_Sentry Native_ is a wrapper around two most popular crash-reporting frameworks: [Breakpad](https://chromium.googlesource.com/breakpad/breakpad/) and [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md). It provides an abstraction layer that unlocks certain Sentry features, such as setting tags and adding breadcrumbs to your crashes. Sentry Native is also responsible for sending the crashes directly to Sentry in an efficient manner.

Platforms we are going to support eventually: MacOS X, Linux (32/64 bit), Windows (32/64 bit), Android, iOS.

## Table of Contents <!-- omit in toc -->

- [Downloads](#downloads)
  - [What is inside](#what-is-inside)
- [Build Prerequisites](#build-prerequisites)
  - [Linux](#linux)
  - [MacOS](#macos)
  - [Windows](#windows)
- [Building Sentry Native](#building-sentry-native)
  - [Linux](#linux-1)
  - [MacOS](#macos-1)
  - [Windows](#windows-1)
- [Sample Application](#sample-application)
- [Development: Generating Build Files](#development-generating-build-files)
  - [Additional Dependencies](#additional-dependencies)
  - [MacOS / Linux](#macos--linux)
  - [Windows](#windows-2)

## Downloads

Packages with source code can be downloaded from the [release](https://github.com/getsentry/sentry-native/releases) page.

### What is inside

At the moment, we package and distribute the following components:

- Sentry Native source code
- Crashpad source code
- Build files for MacOS ("make" files) and Windows (Visual Studio solution)

If any of the provided build files do not work for you, please refer to [Development: Generating Build Files](#development-generating-build-files)

## Build Prerequisites

In order to build the provided source files, you will need the following software:

### Linux

- [GNU make](https://www.gnu.org/software/make/)
- [clang](https://clang.llvm.org/) 3.9+
- uuid-dev (on Ubuntu)

### MacOS

- [Xcode](https://developer.apple.com/xcode/) (can be installed from Mac App Store)
- [GNU make](https://www.gnu.org/software/make/)
- [clang](https://clang.llvm.org/) 3.9+

### Windows

- Microsoft Visual Studio 2017 (or later)

## Building Sentry Native

**NOTE:** The following commands will work for the **packaged** version of
Sentry Native (see [Downloads](#downloads)). We generate `gen_*` folders for
Linux, macOS and Windows during the CI build, so you will _not_ find them in the
GIT repository.

### Linux

```sh
cd gen_linux
make -j`nproc`
```

### MacOS

```sh
cd gen_macosx
make -j`getconf _NPROCESSORS_ONLN`
```

The command will produce the following outputs to `bin/Release`:

- The Sentry Native SDK dynamic libraries
- Example applications for each variant
- Test targets

### Windows

Visual Studio solution is located at `gen_windows/Sentry-Native.sln` and contains projects for both Sentry Native and Crashpad.

**WARNING:** There is a known issue that the Windows SDK version configured in the solution might not be present on your system. If you see `"The Windows SDK version XXX was not found"` error, you can try to "Retarget Solution" ("Project" -> "Retarget solution"). You can also regenerate build files on the target machine, for that please consult the [development section](#development-generating-build-files).

## Sample Application

The build commands produce the Sentry Native libraries (e.g.
`./bin/Release/libsentry.dylib` on MacOS), and sample crashing applications. You
can run them by passing your Sentry DSN key via an environment variable:

```sh
# MacOS
SENTRY_DSN=https://XXXXX@sentry.io/YYYYY ./bin/Release/example
```

The command will send an event to Sentry. If [debug symbols are present](https://docs.sentry.io/workflow/debug-files/) in Sentry, the event will also be properly symbolicated.

## Development: Generating Build Files

If you want to develop or test Sentry Native locally, or the distribution package
doesn't fit your needs, you can run `make configure` from the root directory. On
Windows, please check `premake/README.md` for more instructions.

### Additional Dependencies

- [premake5](https://premake.github.io/download.html#v5): we use `premake` to
  generate build files. When using `make configure`, this is downloaded automatically.

### MacOS / Linux

```sh
make configure
```

This will create `./premake/Makefile`, that you can use to build any part of the
project and run tests.

Use `make help` to see all available targets.

### Windows

```sh
cd premake/
premake5 vs2017
```

The last command will create a Visual Studio 2017 solution (`Sentry-Native.sln`)
that contains all variants of the Sentry Native library projects, and example projects.
