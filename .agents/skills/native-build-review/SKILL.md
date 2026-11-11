---
name: native-build-review
description: Finds real build, packaging, CI, and release breakage in sentry-native CMake, GitHub Actions, Gradle (NDK), Craft, pkg-config, and test-runner build files. Use for pull request review of build and workflow changes.
allowed-tools: Read Grep Glob
---

You review build, CI, and release changes in sentry-native, a C SDK that is built with CMake on Linux, macOS, iOS, Windows, Android (Gradle + CMake under `ndk/`), and out-of-tree console toolchains, and consumed by downstream SDKs via `add_subdirectory`, `find_package(sentry)`, pkg-config, and the Maven NDK artifact.

Report only breakage you can trace: a configuration that fails to configure or compile, a target that links wrongly, an install or package that consumers cannot use, a CI job that silently stops testing what it claims, or a release step that will reject or mis-publish artifacts.

## How the Build Fits Together

- Root `CMakeLists.txt` owns the options (`SENTRY_BACKEND`, `SENTRY_TRANSPORT`, `SENTRY_BUILD_SHARED_LIBS`, `SENTRY_INTEGRATION_*`, and others). Platform defaults are set before `project()` in some cases, so ordering matters. Sources are listed in `src/CMakeLists.txt`.
- Install and export files: `sentry-config.cmake.in`, `sentry.rc.in`, `src/exports.map` (the Linux symbol version script), and `sentry.pc.in` on branches that ship pkg-config.
- Tests are driven by pytest. `tests/cmake.py` configures and builds a fresh CMake tree per test, with compiler flags that `tests/build_config.py` derives from the CI environment (`ERROR_ON_WARNINGS`, `RUN_ANALYZER`, `TEST_X86`, `ANDROID_*`, and others). `tests/unit/CMakeLists.txt` builds the unit test binary.
- `.github/workflows/ci.yml` is one large matrix across compilers, sanitizers, analyzers, emulators, Wine, and Android. Third-party actions are pinned to full commit SHAs with a version comment.
- `.craft.yml` drives releases: artifact names in `requireNames` and each target's `includeNames` must match what the workflows upload.
- `ndk/` builds the Android AAR with Gradle. `ndk/build.gradle.kts` and `ndk/lib/build.gradle.kts` choose the NDK version and the native backend.

## What To Look For

These classes caused real fixes in past reviews.

1. **CMake option and platform logic.** A new option or `if()` branch that is not guarded to the platforms that support it (`WIN32`, `APPLE`, `ANDROID`, Xbox), accepts invalid values without `message(FATAL_ERROR)`, uses a cached default that overrides an explicit user value, or sets flags after `project()` when they must be set before. Check every backend and transport combination the branch can see.
2. **Linking and install.** Missing link dependencies (for example `ptrace`, `dl`, `pthread`, system libunwind), wrong `PRIVATE`/`PUBLIC` scope that leaves consumers without a needed define or library, pkg-config or CMake package files that hardcode `lib`/`include` instead of `CMAKE_INSTALL_*`, omit static link dependencies, or ignore `SENTRY_ENABLE_INSTALL`. Library names and sonames that differ between platforms (Linux soname vs macOS install name vs Windows DLL).
3. **CI matrix drift.** A workflow change that drops a needed setup step for one matrix entry (QEMU, LLVM for coverage, Android emulator, Wine), picks a different toolchain than intended (PATH lookup instead of the pinned path), writes `GITHUB_ENV` or `GITHUB_PATH` in a way that breaks on Windows shells, pins a tool version that is incompatible with the runner, or leaves a debug or stress-test change in place. Check `tests/build_config.py`, `tests/cmake.py`, and `tests/conftest.py` when workflow environment variables change, since they read them.
4. **Version pins.** A SHA that does not match its version comment, or sibling actions from the same project pinned to different versions (for example CodeQL `init` and `analyze`), or a dependency bump that removes an input still passed (Gradle action inputs, pytest plugin versions in `tests/requirements.txt`).
5. **Release config.** Artifact names, glob or regex patterns, and paths in `.craft.yml` that no longer match the archives the release workflow builds; GCS `latest/` handling; Maven NDK artifact naming.
6. **Gradle / NDK.** An NDK version or backend override that applies to all builds when it was meant for one job, glob patterns Gradle will never match, configuration applied twice, or local debug properties committed in `ndk/gradle.properties`.
7. **Quoting and generators.** Flags quoted as one argument so MSBuild or Ninja receives them wrong, generator or toolset strings that break cross-builds, and paths with spaces.

## What Not To Report

Past reviews raised these and maintainers rejected them:

- Claims about runner images, OS package names, or tool behavior that depend on a version you did not confirm in the workflow (for example `runs-on`, container image, or pinned tool version). Read the job definition first.
- A missing dependency or ordering problem that another step in the same file already handles. Read the whole target or job, not only the hunk.
- `PRIVATE` definitions that are intentionally private because downstream integration sources are added to the `sentry` target directly. Report scope only when a consumer outside the target needs the definition.
- Checks that already skip missing artifacts or unsupported configurations by design.
- Problem-matcher regex tuning, log formatting, and job naming unless a real failure stops being reported.
- Style, ordering of list entries, and refactor suggestions.
- Anything under `external/` or `vendor/`.

## Severity

| Level | Use For |
|-------|---------|
| high | Release or publish fails or ships wrong artifacts; a supported platform or backend fails to configure, compile, or link; installed package files are unusable for consumers. |
| medium | A CI job silently stops testing what its name says, or tests the wrong toolchain; a non-default option combination breaks; a pin or bump breaks a workflow step. |
| low | Narrow breakage in a rarely used option or local developer path. |

## Finding Format

- Title: the exact breakage and where it shows up, such as "Windows screenshot option enabled on non-WIN32 builds".
- Description: one or two sentences with the failing configuration or job and the fix.
- Verification: 2-5 bullets citing the changed line and the file or job that proves the failure.
