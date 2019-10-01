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
- [Distributions](#distributions)
  - [What is Inside](#what-is-inside)
  - [Platform Support](#platform-support)
- [Building and Installation](#building-and-installation)
  - [Build Prerequisites](#build-prerequisites)
  - [Building the SDK](#building-the-sdk)
- [Known Issues](#known-issues)
- [Sample Application](#sample-application)
- [Development](#development)
  - [Additional Dependencies](#additional-dependencies)
  - [Development Setup](#development-setup)
  - [Running Tests](#running-tests)

## Downloads

The SDK can be downloaded from the [Releases] page, which also lists the
changelog of every version.

## Distributions

The release archive contains three distributions of the SDK:

![SDK distributions](https://user-images.githubusercontent.com/1433023/65526140-dc3ccc00-def0-11e9-8271-6876afe400cc.png)

\* _Adding stack traces and capturing application crashes requires you to add an
unwind library and hook into signal handlers of your process. The Standalone
distribution currently does not contain integrations that perform this
automatically._

**Note**: On Windows and macOS, we strongly encourage the Crashpad distribution.
Due to limitations of Crashpad on Linux, we recommend to use Breakpad for Linux.

### What is Inside

The SDK bundle contains the following folders:

- `breakpad`, `crashpad`: Contain headers and sources of the Breakpad and
  Crashpad libraries. These are required to build the SDK in its respective
  distributions. To build the standalone distribution of the SDK, these folders
  can be discarded.
- `include`: Contains the Sentry header file. Set the include path to this
  directory or copy the header file to your source tree so that it is available
  during the build.
- `gen_*`: Contains generated projects and build files for the supported
  platforms. See the next section for more information.
- `premake`: Contains build files that allow to generate new projects and
  customize build parameters. When building for one of the supported platforms,
  prefer to use build files from the `gen_*` folders, instead.
- `src`: Sources of the Sentry SDK required for building.

### Platform Support

The Sentry SDK comes with pregenerated build files for the following platforms:

- **Linux and macOS**

  `gen_linux` and `gen_macos` contain Makefiles that can be used to produce
  dynamic libraries. Run `make help` to see an overview of the available
  configurations and target. There are debug and release configurations, that
  can be toggled when building:

  ```bash
  make config=release sentry
  ```

- **Windows**

  `gen_windows` contains a Microsoft Visual Studio 2017 solution. Open the
  solution and add your projects or copy the projects to an existing solution.
  Each project supports a debug and release configuration and includes all
  sources required for building.

- **Android (early alpha)**

  `gen_android` contains `*.mk` files that can be used by `ndk-build` to build the project using
  Android NDK.

## Building and Installation

### Build Prerequisites

To build the SDK from the provided source files, ensure that your system meets
the following requirements:

**Linux**:

- [GNU make]
- [clang] 3.9 or later
- `uuid-dev` (on Ubuntu)

**macOS**:

- [Xcode] (can be installed from Mac App Store)
- [GNU make]
- [clang] 3.9 or later

**Windows**:

- Microsoft Visual Studio 2017 or later

**Android**:

- [Android NDK] (Native Development Kit)

  *Note*: Android cross-compilation is currently only tested on MacOS.

### Building the SDK

**NOTE:** The following commands will work for the **packaged** version of the
Sentry SDK (see [Downloads]). The `gen_*` folders are only included in the
release archives. There are multiple available targets to build (defaults to all
targets):

- `sentry`: Builds the Native SDK built as a dynamic library.
- `sentry_breakpad`: Builds the Native SDK with Google Breakpad as a dynamic
  library.
- `sentry_crashpad`: Builds the Native SDK with Google Crashpad as a dynamic
  library. This requires to build and ship the `crashpad_handler` executable
  with the application.

**Linux**:

```sh
cd gen_linux
make
```

**macOS**:

```sh
cd gen_macosx
make
```

**Windows**:

The Visual Studio solution is located at `gen_windows/Sentry-Native.sln` and
contains projects for both Sentry Native and Crashpad.

*WARNING:* There is a known issue that the Windows SDK version configured in
the solution might not be present on your system. If you see `"The Windows SDK version XXX was not found"` error, you can try to "Retarget Solution" ("Project"
-> "Retarget solution"). You can also regenerate build files on the target
machine, for that please consult the [Development].

**Android**:

```sh
make android-build PREMAKE_DIR=gen_android
```

*NOTE*: `nkd-build` command from Android NDK should be available, i.e. Android NDK directory should be added to your `PATH`.

The command will build the project binaries for all NDK platforms (arm64-v8a, armeabi-v7a, x86, and x86_64 at the time of writing) and place the outputs in `gen_android/libs/`.

## Known Issues

- Sentry with Crashpad cannot upload minidumps on Linux. This is due to a
  limitation in the upstream Crashpad project.
- Sentry with Breakpad cannot upload minidumps. The uploader was temporarily
  removed and will be restored in a future version.
- Attachments are currently in _Preview_ and may not be available to your
  organization. Please see [Event Attachments] for more information.

[event attachments]: https://docs.sentry.io/platforms/native/#event-attachments-preview

## Sample Application

The build commands produce the Sentry Native libraries (e.g.
`./bin/Release/libsentry.dylib` on MacOS), and sample crashing applications. You
can run them by passing your Sentry DSN key via an environment variable:

```sh
# MacOS
SENTRY_DSN=https://XXXXX@sentry.io/YYYYY ./bin/Release/example
```

The example sends a message event to Sentry.

## Development

If you want to develop or test Sentry Native locally, or the distribution package
doesn't fit your needs, you can run `make configure` from the root directory. On
Windows, please check `premake/README.md` for more instructions.

### Additional Dependencies

- [premake5](https://premake.github.io/download.html#v5): We use `premake` to
  generate build files. When using `make configure`, this is downloaded
  automatically.

### Development Setup

**MacOS / Linux**:

```sh
make configure
cd premake
make
```

This will create `./premake/Makefile`, that you can use to build any part of the
project and run tests.

Use `make help` to see all available targets.

**Windows**:

```sh
cd premake/
premake5 vs2017
```

The last command will create a Visual Studio 2017 solution (`Sentry-Native.sln`)
that contains all variants of the Sentry Native library projects, and example projects.

**Android**:

```sh
make android-configure
```

...creates `./premake/*.mk` build files. You can then run `make android-build`
to build all the targets.

### Running Tests

The SDK ships with a test suite based on `catch.hpp`. Tests are built as
separate test targets, prefixed with `test_*` each. You can build and run the
test targets to execute tests.

On macOS and Linux, the _top-level Makefile_ contains a convenience command to
run tests:

```sh
make test
```

[releases]: https://github.com/getsentry/sentry-native/releases
[breakpad]: https://chromium.googlesource.com/breakpad/breakpad/
[crashpad]: https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md
[xcode]: https://developer.apple.com/xcode/
[gnu make]: https://www.gnu.org/software/make/
[clang]: https://clang.llvm.org/
[debug files]: https://docs.sentry.io/workflow/debug-files/
[downloads]: #downloads
[development]: #development
[android ndk]: https://developer.android.com/ndk/downloads/
