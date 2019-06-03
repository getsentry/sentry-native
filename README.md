# SentryPad

SentryPad is a wrapper around two most popular crash-reporting frameworks: [Breakpad](https://chromium.googlesource.com/breakpad/breakpad/) and [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md). It provides an abstraction layer that unlocks certain Sentry features, such as setting tags and adding breadcrumbs to your crashes. SentryPad is also responsible for sending the crashes directly to Sentry in an efficient manner.

Platforms we are going to support eventually: MacOS X, Linux (32/64 bit), Windows (32/64 bit), Android, iOS.

- [SentryPad](#sentrypad)
  - [Downloads](#downloads)
    - [What is inside](#what-is-inside)
  - [Build Prerequisites](#build-prerequisites)
    - [MacOS](#macos)
    - [Windows](#windows)
  - [Building Crashpad + Sentrypad](#building-crashpad--sentrypad)
    - [MacOS](#macos-1)
    - [Windows](#windows-1)
  - [Development: Generating Build Files](#development-generating-build-files)
    - [Additional Dependencies](#additional-dependencies)
    - [MacOS](#macos-2)
    - [Windows](#windows-2)

## Downloads

Packages with source code can be downloaded from the [release](https://github.com/getsentry/sentrypad/releases) page.

### What is inside

At the moment, we package and distribute the following components:

- Sentrypad source code
- Crashpad source code
- Build files for MacOS ("make" files) and Windows (Visual Studio solution)

If any of the provided build files do not work for you, please refer to [Development: Generating Build Files](#development-generating-build-files)

## Build Prerequisites

In order to build the provided source files, you will need the following software:

### MacOS

- [GNU make](https://www.gnu.org/software/make/)
- [clang](https://clang.llvm.org/)

### Windows

- Microsoft Visual Studio 2017 (or later)

## Building Crashpad + Sentrypad

**WARNING** The following commands will work for the **packaged** version of Sentrypad (see [Downloads](#downloads)). We generate `gen_macosx` and `gen_windows` create during the CI phase, so you will _not_ find them in the repository.

### MacOS

```sh
cd gen_macosx
make
```

The command will build both Crashpad and Sentrypad libraries, as well as

### Windows

Visual Studio solution is located at `gen_windows/Crashpad.sln`

```sh
cd premake/
premake5 vs2017
```

## Development: Generating Build Files

### Additional Dependencies

- [premake5](https://premake.github.io/download.html#v5): we use `premake` to generate build files.

### MacOS

```sh
cd premake/
premake5 gmake2
```

### Windows

```sh
cd premake/
premake5 vs2017
```

The last command will create a Visual Studio 2017 solution (`Sentrypad.sln`) that contains `example_crashpad` project.
