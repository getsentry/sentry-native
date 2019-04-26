# SentryPad

SentryPad is a wrapper around two most popular crash-reporting frameworks: [Breakpad](https://chromium.googlesource.com/breakpad/breakpad/) and [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md). It provides an abstraction layer that unlocks certain Sentry features, such as setting tags and adding breadcrumbs to your crashes. SentryPad is also responsible for sending the crashes directly to Sentry in an efficient manner.

Platforms we aim to support: MacOS X, Linux (32/64 bit), Windows (32/64 bit), Android, iOS.

## Prerequisites

* [premake5](https://premake.github.io/download.html#v5)
* make (Linux, MacOS)
* Visual Studio 2017 (Windows)

## Building Example Applications

### Crashpad on Linux/MacOS

```sh
cd premake/
premake5 gmake2
make example_crashpad
./bin/Release/example_crashpad
```

### Breakpad on Windows

```sh
cd premake/
premake5 vs2017
```

The last command will create a Visual Studio 2017 solution that contains `example_breakpad` project.
