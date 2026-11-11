# Crash-Path Rules

Read this when the diff touches a signal handler, an exception filter, the inproc handler thread, crash-time scope flushes, `on_crash`, the native crash handler or daemon, or anything reachable from them.

## Where Crash Code Runs

- **inproc**: `process_ucontext()` in `src/backends/sentry_backend_inproc.c` runs on the crashed thread (signal handler on Unix, unhandled exception filter on Windows). The heavier work is handed to the inproc handler thread. The comment above `process_ucontext()` is the canonical rule list.
- **breakpad / crashpad**: the in-process callbacks in `sentry_backend_breakpad.cpp` and `sentry_backend_crashpad.cpp` (for example `flush_scope_from_handler`) run inside the crashing process after `sentry__page_allocator_enable()` and `sentry__enter_signal_handler()`. The crashpad handler process itself is out of process.
- **native**: `src/backends/native/sentry_crash_handler.c` runs in the crashing process and notifies the daemon through shared memory and IPC (`sentry_crash_ipc.c`). `sentry_crash_daemon.c` runs in a separate process. It may allocate and lock freely, but it must not trust the crashed process's memory layout without bounds checks.
- Thread sampling for app hangs (`sentry_thread_stackwalk_posix.c`) also runs code inside a signal handler on the sampled thread.

## On the Crashed Thread (Unix Signal Context)

Report when changed code on this path does any of these:

- Calls a function that is not async-signal-safe: stdio, `printf`-family formatting, the default logger, `send`/`recv` on sockets when a safe alternative exists in the file, `sem_wait`, `pthread_mutex_lock` directly, `dlopen`, `getenv` after init, C++ exceptions.
- Allocates through libc `malloc`, `new`, `std::string`, `std::vector`, `strdup`, or a third-party library. These bypass the page allocator and can deadlock if the crash interrupted `malloc`.
- Blocks without a timeout on another thread or process (waiting on a semaphore, pipe, or futex that may never be signaled).
- Puts large arrays on the stack. The handler may run on a small `sigaltstack`.
- Fails to restore `errno` in a non-fatal handler (thread sampling, chained handlers).
- Reads scope or options through a path that increfs or frees heap objects without the lock, when close or re-init on another thread can free them.
- Marks a non-signal thread as the signal-handling thread. `sentry__block_for_signal_handler()` makes locks no-ops on that thread, so its work then races with other threads.

Page-allocator details (the SKILL.md exclusion covers the rest): it exists on Unix only, frees are no-ops while it is on, and `sentry_value_*` constructors that format timestamps or strings are still unsafe on the crashed thread even though their allocation is not.

Also not a finding: `sentry__mutex_lock` while `sentry__enter_signal_handler()` is active on the same thread: the lock is skipped by design (see `src/sentry_sync.h`).

## Windows

Exception filters and vectored handlers run with fewer restrictions, but the same principles hold: no unbounded waits, no heap allocation that can deadlock on the loader or heap lock, and `HANDLE`s must be closed on every path. The page allocator does not exist on Windows.

## Re-entrancy and Chaining

- `sentry__enter_signal_handler()` returns a depth: 1 = first entry, 2 = recursive crash (skip hooks, still try to capture), 3+ = bail out to the previous handler. Check that new code respects the depth and that hooks such as `before_send` and `on_crash` do not run twice.
- When the SDK chains to a previous handler or re-raises, check the signal mask, `SA_RESETHAND`, and the saved previous action for the strategy and platform in question (Android differs from Linux).

## Crash Data Must Survive

The goal of the crash path is one complete crash report. Report a change when:

- A failure or timeout drops the event, minidump, attachments, or breadcrumbs instead of taking the documented fallback.
- The same crash can be uploaded twice (for example, run directory copy plus retry cache copy).
- A flag such as "captured" or "done" is set before the capture actually succeeded.
- The daemon or handler can exit, time out, or be shut down while a crash is still in flight.
