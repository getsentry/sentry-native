# Changelog

## Unreleased

**Fixes**:

- Fix a memory unsafety issue when calling `sentry_value_remove_by_key`.
- Improvements to internal logging.
- Better handling of timeouts.
- Better 32-bit build support.

**Thank you**:

Fixes in this release have been contributed by:

- [@eakoli](https://github.com/eakoli)

## 0.3.2

**Features**:

- Implement a new logger hook. ([#267](https://github.com/getsentry/sentry-native/pull/267))

  This adds the new `sentry_options_set_logger` function, which can be used to customize the sentry-internal logging, for example to integrate into an app’s own logging system, or to stream logs to a file.

- New CMake options: `SENTRY_LINK_PTHREAD`, `SENTRY_BUILD_RUNTIMESTATIC` and `SENTRY_EXPORT_SYMBOLS` along with other CMake improvements.

**Fixes**:

- Avoid memory unsafety when loading session from disk. ([#270](https://github.com/getsentry/sentry-native/pull/270))
- Avoid Errors in Crashpad Backend without prior scope changes. ([#272](https://github.com/getsentry/sentry-native/pull/272))
- Fix absolute paths on Windows, and allow using forward-slashes as directory separators. ([#266](https://github.com/getsentry/sentry-native/pull/266), [#289](https://github.com/getsentry/sentry-native/pull/289))
- Various fixes uncovered by static analysis tools, notably excessive allocations by the page-allocator used inside signal handlers.
- Build fixes for MinGW and other compilers.

**Thank you**:

Features, fixes and improvements in this release have been contributed by:

- [@Mixaill](https://github.com/Mixaill)
- [@blinkov](https://github.com/blinkov)
- [@eakoli](https://github.com/eakoli)

## 0.3.1

- Add support for on-device symbolication, which is enabled by default on
  Android. Use `sentry_options_set_symbolize_stacktraces` to customize.
- Enable gzip compressed crashpad minidumps on windows.
- Correctly 0-pad short `build-id`s.
- Fix build for 32bit Apple targets.

## 0.3.0

- Always send the newer `x-sentry-envelope` format, which makes this
  incompatible with older on-premise installations.
- Better document and handle non-ASCII paths. Users on windows should use the
  `w` version of the appropriate APIs.
- Avoid segfaults due to failed sentry initialization.
- Avoid creating invalid sessions without a `release`.
- Make `sentry_transport_t` opaque, and instead expose APIs to configure it.
  More functionality related to creating custom transports will be exposed in
  future versions.

### Breaking changes

- The `sentry_backend_free` function was removed.
- The `sentry_backend_t` type was removed.
- The `sentry_transport_t` type is now opaque. Use the following new API to
  create a custom transport.

### New API

- `sentry_transport_new`
- `sentry_transport_set_state`
- `sentry_transport_set_free_func`
- `sentry_transport_set_startup_func`
- `sentry_transport_set_shutdown_func`

See `sentry.h` for more documentation.

### Deprecations

- `sentry_new_function_transport` has been deprecated in favor of the new
  transport builder functions.

## 0.2.6

- Avoid crash with invalid crashpad handler path.

## 0.2.5

- Send sessions to the correct sentry endpoint and make sure they work.
- Smaller cleanups.

## 0.2.4

- Avoid unsafe reads in the linux module finder.
- Update to latest crashpad snapshot.
- Yet more CMake improvements (thanks @madebr and @Amphaal).

## 0.2.3

### Important upgrade notice

All `0.2.x` versions prior to this one were affected by a bug that could
potentially lead to serious data-loss on Windows platforms. We encourage
everyone to update as quickly as possible.
See [#220](https://github.com/getsentry/sentry-native/issues/220) for details.

### Deprecations

- `sentry_transport_t` will be replaced by an opaque struct with setter methods
  in a future release.
- `sentry_backend_free` and `sentry_backend_t` are deprecated and will be
  removed in a future release.

### Other changes

- Further improvements to the cmake build system (huge thanks to @madebr
  [#207](https://github.com/getsentry/sentry-native/pull/207))
- Improved support for older Windows versions, as low as Windows XP SP3 (thanks
  to @Mixaill [#203](https://github.com/getsentry/sentry-native/pull/203),
  @cammm [#202](https://github.com/getsentry/sentry-native/pull/202) and
  @jblazquez [#212](https://github.com/getsentry/sentry-native/pull/212))
- Improved documentation
- Cleaned up sentry database handling
- Added new `sentry_handle_exception` function to explicitly capture a crash
  (thanks @cammm [#201](https://github.com/getsentry/sentry-native/pull/201))
- Added new `sentry_clear_modulecache` function to clear the list of loaded
  modules. Use this function when dynamically loading libraries at runtime.

## 0.2.2

- Implement experimental Session handling
- Implement more fine grained Rate Limiting for HTTP requests
- Implement `sample_rate` option
- In-process and Breakpad backend will not lose events queued for HTTP
  submission on crash
- `sentry_shutdown` will better clean up after itself
- Add Experimental MinGW build support (thanks @Amphaal
  [#189](https://github.com/getsentry/sentry-native/pull/189))
- Various other fixes and improvements

## 0.2.1

- Added Breakpad support on Linux
- Implemented fallback `debug-id` on Linux and Android for modules that are
  built without a `build-id`
- Fixes issues and added CI for more platforms/compilers, including 32-bit Linux
  and 32-bit VS2017
- Further improvements to the CMake configuration (thanks @madebr
  [#168](https://github.com/getsentry/sentry-native/pull/168))
- Added a new `SENTRY_TRANSPORT` CMake option to customize the default HTTP transport

## 0.2.0

- Complete rewrite in C
- Build system was switched to CMake
- Add attachment support
- Better support for custom transports
- The crashpad backend will automatically look for a `crashpad_handler`
  executable next to the running program if no `handler_path` is set.

### Breaking Changes

- The `sentry_uuid_t` struct is now always a `char bytes[16]` instead of a
  platform specific type.
- `sentry_remove_context`: The second parameter was removed.
- `sentry_options_set_transport`:
  This function now takes a pointer to the new `sentry_transport_t` type.
  Migrating from the old API can be done by wrapping with
  `sentry_new_function_transport`, like this:
  ```c
  sentry_options_set_transport(
        options, sentry_new_function_transport(send_envelope_func, &closure_data));
  ```

### Other API Additions

- `size_t sentry_value_refcount(sentry_value_t value)`
- `void sentry_envelope_free(sentry_envelope_t *envelope)`
- `void sentry_backend_free(sentry_backend_t *backend)`

## 0.1.4

- Add an option to enable the system crash reporter
- Fix compilation warnings

## 0.1.3

- Stack unwinding on Android
- Fix UUID generation on Android
- Fix concurrently captured events leaking data in some cases
- Fix crashes when the database path contains both slashes and backslashes
- More robust error handling when creating the database folder
- Fix wrong initialization of CA info for the curl backend
- Disable the system crash handler on macOS for faster crashes

## 0.1.2

- Fix SafeSEH builds on Win32
- Fix a potential error when shutting down after unloading libsentry on macOS

## 0.1.1

- Update Crashpad
- Fix compilation on Windows with VS 2019
- Fix a bug in the JSON serializer causing invalid escapes
- Fix a bug in the Crashpad backend causing invalid events
- Reduce data event data sent along with minidumps
- Experimental support for Android NDK

## 0.1.0

- Support for capturing messages
- Add an API to capture arbitrary contexts (`sentry_set_context`)
- Fix scope information being lost in some cases
- Experimental on-device unwinding support
- Experimental on-device symbolication support

## 0.0.4

- Breakpad builds on all platforms
- Add builds for Windows (x86)
- Add builds for Linux

## 0.0.3

- Fix debug information generation on macOS

## 0.0.2

- Crashpad builds on macOS
- Crashpad builds on Windows (x64)

## 0.0.1

Initial Release
