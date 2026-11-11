# Platform, Backend, and Binary-Format Rules

Read this when the diff adds or changes `#if`/`#ifdef` conditions, platform-specific files, unwinders, module finders, minidump writers, or ELF/Mach-O/PE parsing.

## Platform Macros

`include/sentry.h` defines `SENTRY_PLATFORM_*`. Several macros overlap:

- `SENTRY_PLATFORM_UNIX` covers Linux, Android, macOS, iOS, AIX, and PlayStation (`SENTRY_PLATFORM_PS`). Switch (`SENTRY_PLATFORM_NX`) is neither Unix nor Windows.
- `SENTRY_PLATFORM_LINUX` is also defined on Android. Code meant only for desktop Linux needs `!defined(SENTRY_PLATFORM_ANDROID)` too. Code under `SENTRY_PLATFORM_UNIX` that uses Linux or POSIX-only APIs also compiles for PlayStation.
- `SENTRY_PLATFORM_DARWIN` covers macOS and iOS. `SENTRY_PLATFORM_WINDOWS` also covers Xbox and GDK (`SENTRY_PLATFORM_XBOX`, `SENTRY_PLATFORM_WINGDK`).
- Xbox, PlayStation, and Switch (`SENTRY_PLATFORM_NX`) are built out of tree from private sources. Public CI does not compile those branches, so check them by reading.

Report when a new condition:

- Compiles a path on a platform that lacks the API it uses (for example `dlfcn.h`, `/proc`, `ptrace`, `mach_*`, `Wer*`, or desktop-only Win32 APIs on Xbox/UWP).
- Leaves a supported platform or backend with no implementation, a stub that silently returns success, or an unused-variable warning that breaks `-Werror` (most CI jobs set `ERROR_ON_WARNINGS`).
- Uses ANSI Win32 or CRT functions (`getenv`, `fopen`, `*A` APIs) for paths or user strings that must handle non-ASCII characters. Paths are wide strings on Windows (`src/path/sentry_path_windows.c`).
- Assumes 64-bit pointers, a specific `long` size, little endian, or a frame layout that does not hold on 32-bit x86, ARM32, or RISC-V.

## Backends

A change to shared code (`src/sentry_core.c`, `sentry_scope.c`, `sentry_database.c`, transports) can change behavior for every backend. Check the callers in each backend that uses it. Typical gaps: a new attachment kind or hint handled by `native` but not by `crashpad` or `breakpad`, or a flush and cache path that differs between `inproc` and the out-of-process backends.

## Binary Formats

- Minidump structs must match the Breakpad/Crashpad on-disk layout exactly, including packing (`#pragma pack(4)` in Breakpad for several structs). Report extra or missing padding only when you can compare against the reference definition. If you cannot see the reference, do not report.
- CPU type, context flag, and stream type constants: report only with a cited definition from Breakpad, Crashpad, or OS headers. Past reviews flagged correct RISC-V constants as wrong.
- ELF and Mach-O parsing of another process's memory or of files on disk: every offset, size, count, and index (`sh_entsize`, `strtab_idx`, note sizes and alignment, `module_count`) must be validated against the mapped buffer before use. Report unchecked division, iteration past the end, alignment arithmetic that loops forever, and reads at file offset vs virtual address confusion.
- Module and image addresses: check that load base, slide, and `vmaddr` are combined the same way as the rest of the module finder for that platform.

## Unwinding

- Remote unwinding (`ptrace`, libunwind remote, libunwindstack) must handle a crashed thread that is not thread 0, must not resume a thread it did not stop, and must bound every `waitpid` or retry loop.
- Frame records need correct trust metadata and registers. A frame list that starts at a zero IP or drops caller frames is a data bug, not a style issue.
