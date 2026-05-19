#include "sentry_boot.h"

#include <stddef.h>

#if (defined(__linux__) || defined(__ANDROID__))                               \
    && defined(SENTRY_WITH_UNWINDER_LIBUNWIND)
#    define SENTRY_THREAD_SAMPLER_SUPPORTED 1
#else
#    define SENTRY_THREAD_SAMPLER_SUPPORTED 0
#endif

#if SENTRY_THREAD_SAMPLER_SUPPORTED

#    include <errno.h>
#    include <pthread.h>
#    include <semaphore.h>
#    include <signal.h>
#    include <stdint.h>
#    include <string.h>
#    include <sys/syscall.h>
#    include <sys/types.h>
#    include <time.h>
#    include <unistd.h>

#    define UNW_LOCAL_ONLY
#    include <libunwind.h>

/*
 * Real-time signal used for asynchronous stack sampling.
 *
 * `SIGRTMIN + 5` is chosen because:
 *   - real-time signals (>= SIGRTMIN) are queued, not coalesced, and are not
 *     used by libc itself, so they will not collide with internal C library
 *     machinery (e.g. NPTL uses SIGRTMIN .. SIGRTMIN+2 on glibc, and Bionic
 *     reserves a similar low range for its own thread plumbing);
 *   - +5 matches the offset that async-profiler uses for the same purpose,
 *     which avoids stepping on common application-side users of low real-time
 *     signal slots.
 *
 * NOTE: the actual value of SIGRTMIN is only known at runtime on glibc/Bionic
 * (it is a function call expanding to libc internals), so we cannot use it in
 * a `case` label and must compute it at handler-install time.
 */
#    define SENTRY_SAMPLER_SIGNAL (SIGRTMIN + 5)

/*
 * Memory ordering between the caller (sampler thread) and the signal handler
 * (target thread) is provided by the kernel: `tgkill` is a syscall, and the
 * kernel issues full memory barriers across syscall entry/exit and during
 * signal-frame setup on the target thread. The `volatile` qualifiers below
 * prevent compiler reordering only — the cross-thread visibility comes from
 * the kernel. This is Linux/Android-specific and not portable.
 */
static pthread_mutex_t g_sampler_lock = PTHREAD_MUTEX_INITIALIZER;
static sem_t g_sampler_done;
static void **g_sampler_out_buf;
static size_t g_sampler_out_max;
static volatile size_t g_sampler_out_written;
static volatile int g_sampler_initialized = 0;

/*
 * TID the currently active sampling request is expecting. Set by the caller
 * before `tgkill` (under `g_sampler_lock`) and consulted inside the signal
 * handler to discard stale signals delivered after a previous sampling request
 * timed out. Real-time signals are queued, not coalesced, so a target thread
 * that was blocked when we sent it the original signal may eventually run our
 * handler at an arbitrarily later time. The TID guard rejects deliveries that
 * land while a different (or no) request is in flight.
 *
 * NOTE: This does NOT discriminate between successive requests targeting the
 * same TID. A caller that re-samples the same thread within the timeout
 * window of a previous timed-out request may receive stale frames. The
 * intended consumer (ANR / frozen-frame capture) samples once per event, so
 * this is not a concern in practice; callers must not exceed the 1s sampling
 * cadence per TID.
 */
static volatile int g_expected_tid = 0;

/*
 * Previous disposition of SENTRY_SAMPLER_SIGNAL, captured at install time so we
 * can forward signals not originating from our sampler (e.g. a host
 * application or another library that also uses this slot). Written once
 * under `g_sampler_lock` before the handler can fire; the `sigaction` syscall
 * provides the memory barrier that publishes it to the handler context.
 */
static struct sigaction g_prev_action;

/*
 * Forward a SENTRY_SAMPLER_SIGNAL delivery to whatever handler was installed
 * before us. Async-signal-safe by construction: just a function-pointer
 * dispatch into code the host already deemed safe for this signal.
 */
static void
sentry__sampler_chain_previous(int sig, siginfo_t *info, void *ucontext_v)
{
    void *fn = (g_prev_action.sa_flags & SA_SIGINFO)
        ? (void *)g_prev_action.sa_sigaction
        : (void *)g_prev_action.sa_handler;
    if (fn == NULL || fn == (void *)SIG_DFL || fn == (void *)SIG_IGN) {
        // No-op for SIG_DFL too: the default disposition for a real-time
        // signal is process termination, which we never want to trigger from
        // a stale or unrelated delivery.
        return;
    }
    if (g_prev_action.sa_flags & SA_SIGINFO) {
        g_prev_action.sa_sigaction(sig, info, ucontext_v);
    } else {
        g_prev_action.sa_handler(sig);
    }
}

/*
 * Signal handler running on the *target* thread's stack. Must be strictly
 * async-signal-safe: no malloc, no logging, no mutex acquisition.
 *
 * We unwind from the saved ucontext using libunwind's
 * `UNW_INIT_SIGNAL_FRAME` mode (same pattern as
 * `sentry__unwind_stack_libunwind` for the crash path), write IPs into the
 * caller-provided buffer, then signal completion via `sem_post`, which POSIX
 * mandates be async-signal-safe.
 */
static void
sentry__sampler_signal_handler(int sig, siginfo_t *info, void *ucontext_v)
{
    // Stale-signal guard: if our TID doesn't match the request currently in
    // flight, this delivery is either (a) a queued stale signal from a
    // previous timed-out sample targeting some other thread (impossible here,
    // since tgkill is thread-targeted) or (b) a signal a host application or
    // unrelated library sent on this slot. Case (a) cannot occur — a stale
    // queued signal can only land on the thread it was targeted at — so any
    // mismatch is by definition not ours. Forward to the previously installed
    // handler so the host's use of this signal keeps working.
    const int my_tid = (int)syscall(SYS_gettid);
    if (my_tid != g_expected_tid) {
        sentry__sampler_chain_previous(sig, info, ucontext_v);
        return;
    }

    size_t written = 0;
    if (g_sampler_out_buf && g_sampler_out_max > 0 && ucontext_v) {
        unw_cursor_t cursor;
        if (unw_init_local2(&cursor, (unw_context_t *)ucontext_v,
                UNW_INIT_SIGNAL_FRAME)
            == 0) {
            unw_word_t prev_ip = 0;
            unw_word_t prev_sp = 0;
            int have_prev = 0;
            for (;;) {
                unw_word_t ip = 0;
                if (unw_get_reg(&cursor, UNW_REG_IP, &ip) != 0) {
                    break;
                }
                unw_word_t sp = 0;
                (void)unw_get_reg(&cursor, UNW_REG_SP, &sp);

                // Stop on lack of progress (mirrors the crash unwinder).
                if (have_prev && ip == prev_ip && sp == prev_sp) {
                    break;
                }

                g_sampler_out_buf[written++] = (void *)(uintptr_t)ip;
                if (written >= g_sampler_out_max) {
                    break;
                }

                prev_ip = ip;
                prev_sp = sp;
                have_prev = 1;

                if (unw_step(&cursor) <= 0) {
                    break;
                }
            }
        }
    }
    g_sampler_out_written = written;
    // sem_post is in the POSIX async-signal-safe list.
    sem_post(&g_sampler_done);
}

#endif // SENTRY_THREAD_SAMPLER_SUPPORTED

size_t
sentry_unwind_thread_stack(int tid, void **stacktrace_out, size_t max_len)
{
#if !SENTRY_THREAD_SAMPLER_SUPPORTED
    (void)tid;
    (void)stacktrace_out;
    (void)max_len;
    return 0;
#else
    if (!stacktrace_out || max_len == 0 || tid <= 0) {
        return 0;
    }

    pthread_mutex_lock(&g_sampler_lock);

    if (!g_sampler_initialized) {
        if (sem_init(&g_sampler_done, 0, 0) != 0) {
            pthread_mutex_unlock(&g_sampler_lock);
            return 0;
        }

        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = sentry__sampler_signal_handler;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sigemptyset(&sa.sa_mask);

        // Capture the previous disposition so the handler can chain to it for
        // any delivery that isn't our sample (see
        // `sentry__sampler_chain_previous`). The handler is permanent for the
        // process lifetime — `sentry_close()` does not restore the previous
        // action, because doing so races with queued-but-undelivered samples
        // and could reinstate `SIG_DFL` in time to terminate the process.
        if (sigaction(SENTRY_SAMPLER_SIGNAL, &sa, &g_prev_action) != 0) {
            sem_destroy(&g_sampler_done);
            pthread_mutex_unlock(&g_sampler_lock);
            return 0;
        }
        g_sampler_initialized = 1;
    }

    g_sampler_out_buf = stacktrace_out;
    g_sampler_out_max = max_len;
    g_sampler_out_written = 0;
    g_expected_tid = tid;

    // Drain any spurious posts from a previous timed-out sample, so that the
    // wait below cannot return prematurely on a stale token.
    while (sem_trywait(&g_sampler_done) == 0) {
        // discard
    }

    pid_t my_pid = getpid();
    if (syscall(SYS_tgkill, my_pid, tid, SENTRY_SAMPLER_SIGNAL) != 0) {
        g_sampler_out_buf = NULL;
        g_expected_tid = 0;
        pthread_mutex_unlock(&g_sampler_lock);
        return 0;
    }

    // Bounded wait — 1 second max.
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 1;

    int rc;
    do {
        rc = sem_timedwait(&g_sampler_done, &timeout);
    } while (rc == -1 && errno == EINTR);

    size_t result = (rc == 0) ? g_sampler_out_written : 0;
    g_sampler_out_buf = NULL;
    g_sampler_out_max = 0;
    g_expected_tid = 0;

    pthread_mutex_unlock(&g_sampler_lock);
    return result;
#endif
}
