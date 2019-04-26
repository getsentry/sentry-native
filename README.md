# SentryPad

SentryPad is a wrapper around two most popular crash-reporting frameworks: [Breakpad](https://chromium.googlesource.com/breakpad/breakpad/) and [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md). It provides an abstraction layer that unlocks certain Sentry features, such as setting tags and adding breadcrumbs to your crashes. SentryPad is also responsible for sending the crashes directly to Sentry in an efficient manner.

Platforms we aim to support: MacOS X, Linux (32/64 bit), Windows (32/64 bit), Android, iOS.

## Building Example Application

### Breakpad Linux:

```sh
make example-breakpad-linux
LD_LIBRARY_PATH=. ./example
```

### Crashpad macOS:

```sh
make example-crashpad-mac
./example
```
