# Concurrency Rules

Read this when the diff touches mutexes, rwlocks, atomics, condition variables, threads, scope observers, batchers, the telemetry thread pool, transports, or `sentry_init`/`sentry_close`.

## Lock Rules (from CONTRIBUTING.md)

- Mutexes are recursive. Recursion is allowed, not encouraged.
- If code nests them, directly or through callees, the **options lock is always acquired before the scope lock**. A backend or helper that calls `SENTRY_WITH_OPTIONS` from inside `SENTRY_WITH_SCOPE*` inverts the order used by `sentry_init` and can deadlock against a concurrent re-init.
- Never `return`, `goto`, or `break` out of a `SENTRY_WITH_*` block. The macros are `for` loops whose increment step releases the lock or reference. A `break` is a finding only when a call site actually does it.
- `SENTRY_WITH_OPTIONS` only holds a refcount for its block. `SENTRY_WITH_SCOPE*` holds the scope lock for the whole block, so its body must stay short.

## Patterns That Caused Real Bugs

- A lock held across transport sends, disk writes, retries, user callbacks, or backend IPC. Other threads that set tags or breadcrumbs then block on IO, and a callback that waits on another thread can deadlock.
- Two locks taken in different orders on two paths, for example a scope write lock and an observers lock, or batch-update APIs that hold one while mutation APIs take the other first.
- Observer notification (`sentry__scope_add_observer` and friends) that reads observer lists or scope data without the lock, runs after a failed mutation, skips observers on one mutation path, or frees data an observer still points to.
- Reader/writer locks (`sentry_sync.h`) where writers starve, wake-ups are lost, or nested reads deadlock when a writer is waiting. Pre-Vista Windows fallbacks have their own condition variable emulation.
- Shutdown that returns before worker threads finish, joins a thread from itself, or frees a shared pool while tasks still run. Check thread handle and condition variable lifetimes on every exit path.
- Batchers and the telemetry pool: data drained from a buffer is dropped when a submit fails, async completion races a crash-time flush, ordering changes on a sync fallback, or a spinlock is held across heavy work.
- Atomics: a flag published with relaxed ordering that another thread uses to gate access to non-atomic data, or a check-then-act sequence on two separate atomics.

## Not Findings

- Lock skipping on the signal-handling thread during a crash. `sentry__block_for_signal_handler()` does this on purpose.
- A claimed race between a crash handler and scope or options cleanup, without checking the teardown order in `sentry_close` (`src/sentry_core.c`): the backend is shut down before global state is freed. Report it only when the handler can still run while the freed object is released.
- Data races in test-only code that cannot hide a shipped bug.
