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
 * handler at an arbitrarily later time — potentially while another sampling
 * request targeting a different thread is in flight. Without this guard, the
 * stale handler would write the wrong thread's stack into the new request's
 * buffer.
 */
static volatile int g_expected_tid = 0;

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
    (void)sig;
    (void)info;

    // Stale-signal guard: if our TID doesn't match the request currently in
    // flight, return silently without posting. Writing into the active
    // request's buffer here would corrupt the result. Not posting is safe —
    // the active request's tgkill will produce its own (correctly-targeted)
    // handler invocation, and `sem_trywait` drains any leftover posts at the
    // start of each request.
    const int my_tid = (int)syscall(SYS_gettid);
    if (my_tid != g_expected_tid) {
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

        // Save any previous disposition. We intentionally overwrite it; this
        // signal slot is owned by the sampler for the lifetime of the
        // process. If the slot was already in use by the host application we
        // would still want to know — but since we have no logger available
        // here that is async-signal-safe to call later, just proceed.
        struct sigaction oldact;
        memset(&oldact, 0, sizeof(oldact));
        if (sigaction(SENTRY_SAMPLER_SIGNAL, &sa, &oldact) != 0) {
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
