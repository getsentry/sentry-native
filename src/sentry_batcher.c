#include "sentry_batcher.h"
#include "sentry_cpu_relax.h"
#include "sentry_options.h"

#ifdef SENTRY_UNITTEST
#    ifdef SENTRY_PLATFORM_WINDOWS
#        include <windows.h>
#        define sleep_ms(MILLISECONDS) Sleep(MILLISECONDS)
#    else
#        include <unistd.h>
#        define sleep_ms(MILLISECONDS) usleep(MILLISECONDS * 1000)
#    endif
#endif

// Use a sleep spinner around a monotonic timer so we don't syscall sleep from
// a signal handler. While this is strictly needed only there, there is no
// reason not to use the same implementation across platforms.
static void
crash_safe_sleep_ms(uint64_t delay_ms)
{
    const uint64_t start = sentry__monotonic_time();
    const uint64_t end = start + delay_ms;
    while (sentry__monotonic_time() < end) {
        for (int i = 0; i < 64; i++) {
            sentry__cpu_relax();
        }
    }
}

// checks whether the currently active buffer should be flushed.
// otherwise we could miss the trigger of adding the last log if we're actively
// flushing the other buffer already.
// we can safely check the state of the active buffer, as the only thread that
// can change which buffer is active is the one calling this check function
// inside sentry__batcher_flush() below
static bool
check_for_flush_condition(sentry_batcher_t *batcher)
{
    // In sentry__batcher_flush, after finishing a flush:
    long current_active = sentry__atomic_fetch(&batcher->active_idx);
    sentry_batcher_buffer_t *current_buf = &batcher->buffers[current_active];

    // Check if current active buffer is also full
    // We could even lower the threshold for high-contention scenarios
    return sentry__atomic_fetch(&current_buf->index)
        >= SENTRY_BATCHER_QUEUE_LENGTH;
}

bool
sentry__batcher_flush(sentry_batcher_t *batcher, bool crash_safe)
{
    if (crash_safe) {
        // In crash-safe mode, spin lock with timeout and backoff
        int attempts = 0;
        while (!sentry__atomic_compare_swap(&batcher->flushing, 0, 1)) {
            const int max_attempts = 200;
            if (++attempts > max_attempts) {
                SENTRY_WARN(
                    "sentry__batcher_flush: timeout waiting for flushing "
                    "lock in crash-safe mode");
                return false;
            }

            // backoff max-wait with max_attempts = 200 based sleep slots:
            // 9ms + 450ms + 1010ms = 1500ish ms
            const uint32_t sleep_time = (attempts < 10) ? 1
                : (attempts < 100)                      ? 5
                                                        : 10;
            crash_safe_sleep_ms(sleep_time);
        }
    } else {
        // Normal mode: try once and return if already flushing
        const long already_flushing
            = sentry__atomic_store(&batcher->flushing, 1);
        if (already_flushing) {
            return false;
        }
    }
    do {
        // prep both buffers
        long old_buf_idx = sentry__atomic_fetch(&batcher->active_idx);
        long new_buf_idx = 1 - old_buf_idx;
        sentry_batcher_buffer_t *old_buf = &batcher->buffers[old_buf_idx];
        sentry_batcher_buffer_t *new_buf = &batcher->buffers[new_buf_idx];

        // reset new buffer...
        sentry__atomic_store(&new_buf->index, 0);
        sentry__atomic_store(&new_buf->adding, 0);
        sentry__atomic_store(&new_buf->sealed, 0);

        // ...and make it active (after this we're good to go producer side)
        sentry__atomic_store(&batcher->active_idx, new_buf_idx);

        // seal old buffer
        sentry__atomic_store(&old_buf->sealed, 1);

        // Wait for all in-flight producers of the old buffer
        while (sentry__atomic_fetch(&old_buf->adding) > 0) {
            sentry__cpu_relax();
        }

        long n = sentry__atomic_store(&old_buf->index, 0);
        if (n > SENTRY_BATCHER_QUEUE_LENGTH) {
            n = SENTRY_BATCHER_QUEUE_LENGTH;
        }

        if (n > 0) {
            // now we can do the actual batching of the old buffer

            sentry_value_t logs = sentry_value_new_object();
            sentry_value_t log_items = sentry_value_new_list();
            int i;
            for (i = 0; i < n; i++) {
                sentry_value_append(log_items, old_buf->items[i]);
            }
            sentry_value_set_by_key(logs, "items", log_items);

            sentry_envelope_t *envelope
                = sentry__envelope_new_with_dsn(batcher->dsn);
            batcher->batch_func(envelope, logs);

            if (crash_safe) {
                // Write directly to disk to avoid transport queuing during
                // crash
                sentry__run_write_envelope(batcher->run, envelope);
                sentry_envelope_free(envelope);
            } else if (!batcher->user_consent
                || sentry__atomic_fetch(batcher->user_consent)
                    == SENTRY_USER_CONSENT_GIVEN) {
                // Normal operation: use transport for HTTP transmission
                sentry__transport_send_envelope(batcher->transport, envelope);
            } else {
                sentry_envelope_free(envelope);
            }
            sentry_value_decref(logs);
        }
    } while (check_for_flush_condition(batcher));

    sentry__atomic_store(&batcher->flushing, 0);
    return true;
}

#define ENQUEUE_MAX_RETRIES 2

bool
sentry__batcher_enqueue(sentry_batcher_t *batcher, sentry_value_t item)
{
    for (int attempt = 0; attempt <= ENQUEUE_MAX_RETRIES; attempt++) {
        // retrieve the active buffer
        const long active_idx = sentry__atomic_fetch(&batcher->active_idx);
        sentry_batcher_buffer_t *active = &batcher->buffers[active_idx];

        // if the buffer is already sealed we retry or drop and exit early.
        if (sentry__atomic_fetch(&active->sealed) != 0) {
            if (attempt == ENQUEUE_MAX_RETRIES) {
                return false;
            }
            continue;
        }

        // `adding` is our boundary for this buffer since it keeps the flusher
        // blocked. We have to recheck that the flusher hasn't already switched
        // the active buffer or sealed the one this thread is on. If either is
        // true we have to unblock the flusher and retry or drop the item.
        sentry__atomic_fetch_and_add(&active->adding, 1);
        const long active_idx_check
            = sentry__atomic_fetch(&batcher->active_idx);
        const long sealed_check = sentry__atomic_fetch(&active->sealed);
        if (active_idx != active_idx_check) {
            sentry__atomic_fetch_and_add(&active->adding, -1);
            if (attempt == ENQUEUE_MAX_RETRIES) {
                return false;
            }
            continue;
        }
        if (sealed_check) {
            sentry__atomic_fetch_and_add(&active->adding, -1);
            if (attempt == ENQUEUE_MAX_RETRIES) {
                return false;
            }
            continue;
        }

        // Now we can finally request a slot and check if the item fits in this
        // buffer.
        const long item_idx = sentry__atomic_fetch_and_add(&active->index, 1);
        if (item_idx < SENTRY_BATCHER_QUEUE_LENGTH) {
            // got a slot, write item to the buffer and unblock flusher
            active->items[item_idx] = item;
            sentry__atomic_fetch_and_add(&active->adding, -1);

            // Check if active buffer is now full and trigger flush. We could
            // introduce additional watermarks here to trigger the flush earlier
            // under high contention.
            // TODO replace with a level-triggered flag
            if (item_idx == SENTRY_BATCHER_QUEUE_LENGTH - 1) {
                sentry__cond_wake(&batcher->request_flush);
            }
            return true;
        }
        // ping the batching thread to flush, since we could miss a cond_wake
        // on adding the last item
        sentry__cond_wake(&batcher->request_flush);
        // Buffer is already full, roll back our increments and retry or drop.
        sentry__atomic_fetch_and_add(&active->adding, -1);
        if (attempt == ENQUEUE_MAX_RETRIES) {
            // TODO report this (e.g. client reports)
            return false;
        }
    }
    return false;
}

SENTRY_THREAD_FN
batcher_thread_func(void *data)
{
    sentry_batcher_t *batcher = data;
    SENTRY_DEBUG("Starting batching thread");
    sentry_mutex_t task_lock;
    sentry__mutex_init(&task_lock);
    sentry__mutex_lock(&task_lock);

    // Transition from STARTING to RUNNING using compare-and-swap
    // CAS ensures atomic state verification: only succeeds if state is STARTING
    // If CAS fails, shutdown already set state to STOPPED, so exit immediately
    // Uses sequential consistency to ensure all thread initialization is
    // visible
    if (!sentry__atomic_compare_swap(&batcher->thread_state,
            (long)SENTRY_BATCHER_THREAD_STARTING,
            (long)SENTRY_BATCHER_THREAD_RUNNING)) {
        SENTRY_DEBUG(
            "batcher thread detected shutdown during startup, exiting");
        sentry__mutex_unlock(&task_lock);
        sentry__mutex_free(&task_lock);
        return 0;
    }

    // Main loop: run while state is RUNNING
    while (sentry__atomic_fetch(&batcher->thread_state)
        == SENTRY_BATCHER_THREAD_RUNNING) {
        // Sleep for 5 seconds or until request_flush hits
        const int triggered_by = sentry__cond_wait_timeout(
            &batcher->request_flush, &task_lock, 5000);

        // Check if we should still be running
        if (sentry__atomic_fetch(&batcher->thread_state)
            != SENTRY_BATCHER_THREAD_RUNNING) {
            break;
        }

        switch (triggered_by) {
        case 0:
#ifdef SENTRY_PLATFORM_WINDOWS
            if (GetLastError() == ERROR_TIMEOUT) {
                SENTRY_TRACE("Batcher flushed by timeout");
                break;
            }
#endif
            SENTRY_TRACE("Batcher flushed by filled buffer");
            break;
#ifdef SENTRY_PLATFORM_UNIX
        case ETIMEDOUT:
            SENTRY_TRACE("Batcher flushed by timeout");
            break;
#endif
#ifdef SENTRY_PLATFORM_WINDOWS
        case 1:
            SENTRY_TRACE("Batcher flushed by filled buffer");
            break;
#endif
        default:
            SENTRY_WARN("Batcher flush trigger returned unexpected value");
            continue;
        }

        // Try to flush
        sentry__batcher_flush(batcher, false);
    }

    sentry__mutex_unlock(&task_lock);
    sentry__mutex_free(&task_lock);
    SENTRY_DEBUG("batching thread exiting");
    return 0;
}

void
sentry__batcher_startup(
    sentry_batcher_t *batcher, const sentry_options_t *options)
{
    batcher->dsn = sentry__dsn_incref(options->dsn);
    batcher->transport = options->transport;
    batcher->run = options->run;
    batcher->user_consent
        = options->require_user_consent ? (long *)&options->user_consent : NULL;

    // Mark thread as starting before actually spawning so thread can transition
    // to RUNNING. This prevents shutdown from thinking the thread was never
    // started if it races with the thread's initialization.
    sentry__atomic_store(
        &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_STARTING);

    sentry__cond_init(&batcher->request_flush);

    sentry__thread_init(&batcher->batching_thread);
    int spawn_result = sentry__thread_spawn(
        &batcher->batching_thread, batcher_thread_func, batcher);

    if (spawn_result == 1) {
        SENTRY_ERROR("Failed to start batching thread");
        // Failed to spawn, reset to STOPPED
        // Note: condition variable doesn't need explicit cleanup for static
        // storage (pthread_cond_t on POSIX and CONDITION_VARIABLE on Windows)
        sentry__atomic_store(
            &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_STOPPED);
        sentry__dsn_decref(batcher->dsn);
        batcher->dsn = NULL;
    }
}

bool
sentry__batcher_shutdown_begin(sentry_batcher_t *batcher)
{
    // Atomically transition to STOPPED and get the previous state
    // This handles the race where thread might be in STARTING state:
    // - If thread's CAS hasn't run yet: CAS will fail, thread exits cleanly
    // - If thread already transitioned to RUNNING: normal shutdown path
    const long old_state = sentry__atomic_store(
        &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_STOPPED);

    // If thread was never started, nothing to do
    if (old_state == SENTRY_BATCHER_THREAD_STOPPED) {
        SENTRY_DEBUG("batcher thread was not started, skipping shutdown");
        return false;
    }

    // Thread was started (either STARTING or RUNNING), signal it to stop
    sentry__cond_wake(&batcher->request_flush);
    return true;
}

void
sentry__batcher_shutdown_wait(sentry_batcher_t *batcher, uint64_t timeout)
{
    (void)timeout;

    // Always join the thread to avoid leaks
    sentry__thread_join(batcher->batching_thread);

    // Perform final flush to ensure any remaining items are sent
    sentry__batcher_flush(batcher, false);

    sentry__dsn_decref(batcher->dsn);
    batcher->dsn = NULL;

    sentry__thread_free(&batcher->batching_thread);
}

void
sentry__batcher_flush_crash_safe(sentry_batcher_t *batcher)
{
    // Check if batcher is initialized
    const long state = sentry__atomic_fetch(&batcher->thread_state);
    if (state == SENTRY_BATCHER_THREAD_STOPPED) {
        return;
    }

    // Signal the thread to stop but don't wait, since the crash-safe flush
    // will spin-lock on flushing anyway.
    sentry__atomic_store(
        &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_STOPPED);

    // Perform crash-safe flush directly to disk to avoid transport queuing
    // This is safe because we're in a crash scenario and the main thread
    // is likely dead or dying anyway
    sentry__batcher_flush(batcher, true);
}

void
sentry__batcher_force_flush_begin(sentry_batcher_t *batcher)
{
    sentry__cond_wake(&batcher->request_flush);
}

void
sentry__batcher_force_flush_wait(sentry_batcher_t *batcher)
{
    do {
        // wait for in-progress flush to complete
        while (sentry__atomic_fetch(&batcher->flushing)) {
            sentry__cpu_relax();
        }
        // retry if the batcher thread (woken by _begin) wins the race
    } while (!sentry__batcher_flush(batcher, false));
}

#ifdef SENTRY_UNITTEST
/**
 * Wait for the batching thread to be ready.
 * This is a test-only helper to avoid race conditions in tests.
 */
void
sentry__batcher_wait_for_thread_startup(sentry_batcher_t *batcher)
{
    const int max_wait_ms = 1000;
    const int check_interval_ms = 10;
    const int max_attempts = max_wait_ms / check_interval_ms;

    for (int i = 0; i < max_attempts; i++) {
        const long state = sentry__atomic_fetch(&batcher->thread_state);
        if (state == SENTRY_BATCHER_THREAD_RUNNING) {
            SENTRY_DEBUGF(
                "batcher thread ready after %d ms", i * check_interval_ms);
            return;
        }
        sleep_ms(check_interval_ms);
    }

    SENTRY_WARNF(
        "batcher thread failed to start within %d ms timeout", max_wait_ms);
}
#endif
