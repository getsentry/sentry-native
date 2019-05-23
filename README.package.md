# SentryPad Distribution Package

**Note**: This README files is part of SentryPad distribution package. For general information about the project, see [this README](https://github.com/getsentry/sentrypad/blob/master/README.md).

This package contains source code and build files for SentryPad and Crashpad. Build files are pregenerated for several platforms: Linux, Windows, and MacOS at the moment.

Build files for a given platform can be found in `./sentrypad/gen_PLATFORM` directory, where `PLATFORM` is one of `linux`, `windows`, and `macosx`.

## Build Prerequisites

### Linux, MacOS

* [GNU make](https://www.gnu.org/software/make/)

### Windows

* Microsoft Visual Studio 2017 (or later)

### Optional Dependencies

* [premake5](https://premake.github.io/download.html#v5): can be used to regenerate build files, if necessary

## Building Crashpad

### On MacOS

```sh
cd crashpad/gen_macosx
make
```

The commands will compile Crashpad libraries and binaries. The output can then be found in `crashpad/gen_macosx/bin/Release`.

### On Windows

Visual Studio solution is located at `crashpad/gen_windows/Crashpad.sln`
