# Android native E2E fixture

Prepare the APK separately from pytest, using a sentry-java checkout with the
native crash service (currently `feat/native-crash-service`). Set `ANDROID_HOME`,
`ANDROID_NDK` (the NDK directory), and `ANDROID_ARCH` for the build:

```sh
python scripts/setup-android-e2e.py ../sentry-java
pytest -s -v tests/test_e2e_sentry.py::test_e2e_android_native
```

Run these commands from the sentry-native root. Testing needs a running emulator
or device, `ANDROID_API`, and the `SENTRY_E2E_*` credentials documented in the test
module. The crash service needs API 30+; tombstone merging needs API 31+.

The setup script builds this checkout's native NDK package and the Java SDK
into an isolated Maven repository, overriding the Java SDK's pinned NDK dependency.
It then builds the fixture APK under `build/android-e2e`. Use `--build-dir` to
choose another output directory and set `SENTRY_E2E_ANDROID_APK` to its APK path
when running pytest.
