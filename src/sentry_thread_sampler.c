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
 * (target thread) is provided by the kernel: `rt_tgsigqueueinfo` is a syscall,
 * and the kernel issues full memory barriers across syscall entry/exit and
 * during signal-frame setup on the target thread. The `volatile` qualifiers
 * below prevent compiler reordering only — the cross-thread visibility comes
 * from the kernel. This is Linux/Android-specific and not portable.
 */
static pthread_mutex_t g_sampler_lock = PTHREAD_MUTEX_INITIALIZER;
static sem_t g_sampler_done;
static void **g_sampler_out_buf;
static size_t g_sampler_out_max;
static volatile size_t g_sampler_out_written;
static volatile int g_sampler_initialized = 0;

/*
 * TID the currently active sampling request is expecting. Set by the caller
 * before sending the signal (under `g_sampler_lock`) and consulted inside the
 * signal handler as a defence-in-depth check; the primary discriminator is the
 * per-request sequence number below.
 */
static volatile int g_expected_tid = 0;

/*
 * Monotonic per-request sequence. Incremented under `g_sampler_lock` on every
 * call and embedded in the queued signal's `si_value.sival_int` payload via
 * `rt_tgsigqueueinfo`. The handler compares the payload against the active
 * sequence and silently discards any signal whose sequence doesn't match — this
 * is what makes the sampler safe when consecutive requests target the same
 * TID (the typical ANR-watchdog pattern), where the TID guard alone would let
 * a stale handler from a previously-timed-out request write its frames into
 * the new request's buffer. 0 is reserved for "no active request".
 */
static volatile unsigned g_current_seq = 0;

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

    // Primary stale-signal guard: compare the per-request sequence the caller
    // embedded in the signal payload against the currently-active sequence.
    // Anything older (e.g. a delayed handler from a previously-timed-out
    // request) is dropped without posting, even if it targets the same TID
    // that the new request is sampling.
    if (!info || (unsigned)info->si_value.sival_int != g_current_seq) {
        return;
    }

    // Defence-in-depth TID guard. `rt_tgsigqueueinfo` already targets a
    // specific TID, so this should never trigger in practice; keeping it
    // means a wrong-thread delivery from a future kernel/libc bug still can't
    // corrupt the active request's buffer.
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

        // The sampler claims this signal slot for the lifetime of the
        // process; any prior disposition is silently overwritten. We don't
        // capture the old handler because we have no async-signal-safe
        // logger to report a collision and no shutdown path that would
        // restore it.
        if (sigaction(SENTRY_SAMPLER_SIGNAL, &sa, NULL) != 0) {
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

    // Pick a fresh per-request sequence under the mutex. The handler reads
    // this back through the queued signal's si_value payload to discard stale
    // deliveries. Skip 0 because 0 means "no active request".
    unsigned my_seq = g_current_seq + 1;
    if (my_seq == 0) {
        my_seq = 1;
    }
    g_current_seq = my_seq;

    // Drain any leftover posts. With the seq guard a stale handler will not
    // post, so this is belt-and-suspenders for the case where a future change
    // introduces a path that does.
    while (sem_trywait(&g_sampler_done) == 0) {
        // discard
    }

    pid_t my_pid = getpid();

    // Send via rt_tgsigqueueinfo so we can attach the request sequence in
    // si_value.sival_int. The kernel preserves these fields verbatim when
    // si_code is SI_QUEUE.
    siginfo_t si;
    memset(&si, 0, sizeof(si));
    si.si_signo = SENTRY_SAMPLER_SIGNAL;
    si.si_code = SI_QUEUE;
    si.si_value.sival_int = (int)my_seq;

    if (syscall(SYS_rt_tgsigqueueinfo, my_pid, tid, SENTRY_SAMPLER_SIGNAL, &si)
        != 0) {
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
