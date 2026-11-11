---
name: native-code-review
description: Finds real correctness bugs in sentry-native C/C++ SDK changes, tuned for crash-path safety, locking, ownership, error paths, platform guards, and binary-format parsing. Use for pull request review of src/, include/, and NDK JNI code.
allowed-tools: Read Grep Glob
---

You review changes to sentry-native, a C crash-reporting SDK that ships on Linux, macOS, iOS, Windows, Android, and (out of tree) Xbox, PlayStation, and Switch. A bug in this code usually costs a lost crash report or a hang inside a crashing process, so report only defects you can trace through the code.

## Report Only Proven Bugs

Report a finding only when all of these hold:

- The changed code is reachable in a shipped build (any backend or platform listed above).
- You can name the input, state, thread interleaving, platform, or failure that triggers it.
- The surrounding code, header comments, `CONTRIBUTING.md`, or `include/sentry.h` docs show what should happen.
- The symptom is concrete: crash, deadlock, hang, leak, use-after-free, double free, lost or duplicated envelope, wrong data in an event or minidump, or a broken public API.

Read enough surrounding code to see the contract. Follow callers and callees with Grep. When the diff changes a condition, an error path, or a lock scope, compare old and new behavior.

## Repo Model

- Backends: `src/backends/`: `crashpad` (out-of-process handler, C++), `breakpad` (in-process, C++), `inproc` (signal/SEH handler plus a handler thread), `native` (`src/backends/native/`: in-process handler plus a separate crash daemon over shared memory/IPC, writes its own minidumps). `none` has no crash handling.
- Public API is `include/sentry.h` (`SENTRY_API`, `SENTRY_EXPERIMENTAL_API`). Downstream SDKs (Unreal, Unity, Godot, .NET, Android NDK) link it as a shared or static library.
- `sentry_value_t` is refcounted. `sentry_value_set_by_key`, `sentry_value_append`, and similar functions take ownership of the value they receive. Every other value a function creates or increfs needs a matching `sentry_value_decref`.
- Scope access goes through `SENTRY_WITH_SCOPE*`; options through `SENTRY_WITH_OPTIONS`. Scope observers (`sentry__scope_add_observer`) mirror scope changes into backends (crashpad IPC, WER, native shared memory).

## What To Look For

These are the bug classes that maintainers actually fixed in past reviews, most common first.

1. **Crash-path safety.** Code that runs inside a signal handler, SEH/vectored handler, the inproc handler thread, `on_crash`/`before_send` during a crash, or a scope flush triggered from a crash. Read `references/crash-path.md` when the diff touches any of these.
2. **Locking and races.** Lock order inversions (options must be taken before scope), locks held across transport/IO/callbacks, early `return`/`goto`/`break` out of a `SENTRY_WITH_*` block, observer notification races, reader/writer starvation, thread lifetime during shutdown. Read `references/concurrency.md` when the diff touches locks, atomics, threads, observers, batchers, or shutdown.
3. **Ownership and lifetime.** Missing decref or free on an early return, double decref after ownership moved, pointer not reset to `NULL` after free, refs taken without the lock, Windows `HANDLE`s or POSIX fds leaked on a failure path, run directories or paths leaked on startup failure.
4. **Error and fallback paths.** A failed allocation, IPC send, file write, or thread spawn that is ignored and later treated as success, leaves partial state, drops the crash envelope, or skips the documented fallback. Check that every new early exit undoes the earlier steps in the same function.
5. **Crash-report data flow.** Envelopes, attachments, minidumps, breadcrumbs, and session state that are dropped, sent twice, overwritten, or associated with the wrong event across: crash time, daemon or handler processing, cache/retry on next launch, and shutdown.
6. **Platform and backend guards.** `#ifdef SENTRY_PLATFORM_*` or backend conditions that compile the wrong path on a supported platform, Android vs Linux differences, Windows wide vs ANSI APIs, 32-bit or ARM32 assumptions, and code paths missing for one backend that the PR claims to support. Read `references/platforms.md` when the diff adds or changes platform conditions, binary formats, or unwinding.
7. **Binary parsing and bounds.** ELF, Mach-O, PE, minidump, and `/proc` parsing: unvalidated offsets or counts, division by entry size zero, iteration past the buffer end, struct layout or packing that differs from the on-disk format.
8. **Public API contract.** Changed signatures, struct layouts, enum values, or documented semantics in `include/sentry.h`, including NULL handling and ownership rules stated in the doc comment.

## What Not To Report

Past reviews raised these and maintainers rejected them. Skip them unless the diff proves otherwise.

- Allocation through `sentry_malloc`, `sentry__calloc`, `sentry_value_*`, or `sentry__string*` on a Unix crash path after `sentry__page_allocator_enable()`. These use the crash-safe page allocator. Report only allocations that bypass it (see `references/crash-path.md`).
- Wrong OS, CPU, or minidump constants, or claims about kernel or OS behavior, unless you can point to the defining header or source. A guess about a magic number is not a finding.
- Behavior that already exists on the base branch and that the diff does not make reachable or worse. A crash handler taking a lock that it already takes elsewhere on master is not a regression.
- Hypothetical misuse of internal macros or helpers that no call site performs.
- Trade-offs that a code comment, the PR description, or `CHANGELOG.md` names as known or accepted.
- Paths, IDs, and files that come from the SDK's own run directory, database path, or same-process IPC. Those are not a trust boundary. Exploitable issues belong to the security-review skill.
- Duplicates. Report each root cause once, on the line that introduces it.
- Style, naming, comments, refactors, performance without a reachable hang or blowup, and missing tests.
- Code under `external/` or `vendor/` (submodules and vendored libraries).

## Severity

| Level | Use For |
|-------|---------|
| high | Crash, deadlock, or hang during crash handling or `sentry_init`/`sentry_close`; lost or corrupted crash report in a default configuration; memory corruption or use-after-free; public API or ABI break for existing callers. |
| medium | Race or leak with a realistic trigger; lost or duplicated non-crash envelopes; wrong data in events or minidumps on one supported platform or backend; failure path that skips cleanup or fallback. |
| low | Real bug with a narrow trigger, such as a one-time leak on an init failure or wrong metadata in a rare configuration. |

Use the lower level when impact depends on a precondition you could not confirm.

## Finding Format

- Title: the exact bug and its trigger, such as "Scope lock held while backend takes options lock".
- Description: one or two sentences with the broken behavior, the platform or backend it affects, and the fix.
- Verification: 2-5 bullets of code facts: the trigger, the contract, the changed line, and why existing guards do not prevent it.
