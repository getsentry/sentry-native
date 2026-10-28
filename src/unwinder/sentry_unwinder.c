#include "sentry_unwinder.h"
#include "sentry_boot.h"

#if defined(SENTRY_WITH_UNWINDER_LIBUNWIND_REMOTE)                             \
    || defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK_REMOTE)
#    include "sentry_logger.h"

#    include <errno.h>
#    include <string.h>
#    include <sys/ptrace.h>
#    include <sys/wait.h>
#    include <unistd.h>
#endif

#define DEFINE_UNWINDER(Func)                                                  \
    size_t sentry__unwind_stack_##Func(void *addr,                             \
        const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)

#define TRY_UNWINDER(Func)                                                     \
    do {                                                                       \
        size_t rv = sentry__unwind_stack_##Func(addr, uctx, ptrs, max_frames); \
        if (rv > 0) {                                                          \
            return rv;                                                         \
        }                                                                      \
    } while (0)

DEFINE_UNWINDER(libunwindstack);
DEFINE_UNWINDER(libbacktrace);
DEFINE_UNWINDER(dbghelp);
DEFINE_UNWINDER(libunwind);
DEFINE_UNWINDER(libunwind_mac);
DEFINE_UNWINDER(psunwind);

#if defined(SENTRY_PLATFORM_LINUX)
#    define DEFINE_THREAD_UNWINDER(Func)                                       \
        size_t sentry__unwind_stack_from_thread_##Func(pid_t tid,              \
            sentry_remote_frame_t *frames, size_t max_frames,                  \
            sentry_remote_registers_t *registers, bool symbolize)

#    define TRY_THREAD_UNWINDER(Func)                                          \
        do {                                                                   \
            size_t rv = sentry__unwind_stack_from_thread_##Func(               \
                tid, frames, max_frames, registers, symbolize);                \
            if (rv > 0) {                                                      \
                return rv;                                                     \
            }                                                                  \
        } while (0)

DEFINE_THREAD_UNWINDER(libunwind_remote);
DEFINE_THREAD_UNWINDER(libunwindstack_remote);
#endif

static size_t
unwind_stack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
#ifdef SENTRY_WITH_UNWINDER_LIBUNWINDSTACK
    TRY_UNWINDER(libunwindstack);
#endif
#ifdef SENTRY_WITH_UNWINDER_LIBBACKTRACE
    TRY_UNWINDER(libbacktrace);
#endif
#ifdef SENTRY_WITH_UNWINDER_DBGHELP
    TRY_UNWINDER(dbghelp);
#endif
#ifdef SENTRY_WITH_UNWINDER_LIBUNWIND
    TRY_UNWINDER(libunwind);
#endif
#ifdef SENTRY_WITH_UNWINDER_LIBUNWIND_MAC
    TRY_UNWINDER(libunwind_mac);
#endif
#ifdef SENTRY_WITH_UNWINDER_PS
    TRY_UNWINDER(psunwind);
#endif
    return 0;
}

size_t
sentry_unwind_stack(void *addr, void **stacktrace_out, size_t max_len)
{
    return unwind_stack(addr, NULL, stacktrace_out, max_len);
}

size_t
sentry_unwind_stack_from_ucontext(
    const sentry_ucontext_t *uctx, void **stacktrace_out, size_t max_len)
{
    return unwind_stack(NULL, uctx, stacktrace_out, max_len);
}

#if defined(SENTRY_PLATFORM_LINUX)
size_t
sentry__unwind_stack_from_thread(pid_t tid, sentry_remote_frame_t *frames,
    size_t max_frames, sentry_remote_registers_t *registers, bool symbolize)
{
    (void)tid;
    (void)frames;
    (void)max_frames;
    (void)registers;
    (void)symbolize;
#    ifdef SENTRY_WITH_UNWINDER_LIBUNWIND_REMOTE
    TRY_THREAD_UNWINDER(libunwind_remote);
#    endif
#    ifdef SENTRY_WITH_UNWINDER_LIBUNWINDSTACK_REMOTE
    TRY_THREAD_UNWINDER(libunwindstack_remote);
#    endif
    return 0;
}

#    if defined(SENTRY_WITH_UNWINDER_LIBUNWIND_REMOTE)                         \
        || defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK_REMOTE)
/**
 * Attach to a thread of another process and wait until it stops.
 * Returns false, leaving the thread detached, if either fails.
 */
bool
sentry__ptrace_attach_thread(pid_t tid)
{
    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) != 0) {
        SENTRY_WARNF("remote_unwind: ptrace attach failed for %d: %s", tid,
            strerror(errno));
        return false;
    }

    // Poll with WNOHANG to avoid hanging on threads in D state. PTRACE_ATTACH
    // sends SIGSTOP but the thread won't stop until it leaves uninterruptible
    // sleep, which may never happen.
    int status = 0;
    int retries = 20;
    for (;;) {
        pid_t waited = waitpid(tid, &status, __WALL | WNOHANG);
        if (waited == tid) {
            break;
        }
        if (waited < 0) {
            SENTRY_WARNF("remote_unwind: waitpid failed for %d: %s", tid,
                strerror(errno));
            ptrace(PTRACE_DETACH, tid, NULL, NULL);
            return false;
        }
        if (--retries <= 0) {
            SENTRY_WARNF("remote_unwind: waitpid timed out for %d", tid);
            ptrace(PTRACE_DETACH, tid, NULL, NULL);
            return false;
        }
        usleep(100000);
    }

    if (!WIFSTOPPED(status)) {
        SENTRY_WARNF(
            "remote_unwind: thread %d did not stop after attach: status=%d",
            tid, status);
        ptrace(PTRACE_DETACH, tid, NULL, NULL);
        return false;
    }

    return true;
}
#    endif
#endif
