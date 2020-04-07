# Changelog

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
