// Remote stack unwinding via libunwind ptrace accessors.
// This file must NOT define UNW_LOCAL_ONLY so that unw_init_remote()
// and other generic (_U prefix) symbols are used.
//
// Only compiled into sentry-crash daemon (SENTRY_WITH_REMOTE_UNWIND).
// The sentry library compiles this file too (shared sources) but the
// function body is empty without the define.

#include "sentry_remote_unwind.h"

#ifdef SENTRY_WITH_REMOTE_UNWIND

#    include "sentry_logger.h"

#    include <errno.h>
#    include <string.h>
#    include <sys/ptrace.h>
#    include <sys/wait.h>

#    include <libunwind-ptrace.h>
#    include <libunwind.h>

size_t
sentry__remote_unwind_thread(
    pid_t tid, sentry_remote_frame_t *frames, size_t max_frames)
{
    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) != 0) {
        SENTRY_WARNF("remote_unwind: ptrace attach failed for %d: %s", tid,
            strerror(errno));
        return 0;
    }

    int status;
    if (waitpid(tid, &status, __WALL) < 0) {
        SENTRY_WARNF(
            "remote_unwind: waitpid failed for %d: %s", tid, strerror(errno));
        ptrace(PTRACE_DETACH, tid, NULL, NULL);
        return 0;
    }

    size_t n = 0;

    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);
    if (!as) {
        SENTRY_WARN("remote_unwind: unw_create_addr_space failed");
        goto detach;
    }

    void *upt = _UPT_create(tid);
    if (!upt) {
        SENTRY_WARN("remote_unwind: _UPT_create failed");
        unw_destroy_addr_space(as);
        goto detach;
    }

    unw_cursor_t cursor;
    int ret = unw_init_remote(&cursor, as, upt);
    if (ret < 0) {
        SENTRY_WARNF("remote_unwind: unw_init_remote failed: %d", ret);
        _UPT_destroy(upt);
        unw_destroy_addr_space(as);
        goto detach;
    }

    while (n < max_frames) {
        unw_word_t ip = 0;
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0 || ip == 0) {
            break;
        }

        frames[n].ip = (uint64_t)ip;
        frames[n].symbol[0] = '\0';
        frames[n].symbol_offset = 0;

        unw_word_t off = 0;
        if (unw_get_proc_name(
                &cursor, frames[n].symbol, sizeof(frames[n].symbol), &off)
            == 0) {
            frames[n].symbol_offset = (uint64_t)off;
        }

        n++;

        if (unw_step(&cursor) <= 0) {
            break;
        }
    }

    _UPT_destroy(upt);
    unw_destroy_addr_space(as);

detach:
    ptrace(PTRACE_DETACH, tid, NULL, NULL);

    SENTRY_DEBUGF("Remote unwound %zu frames for thread %d", n, tid);
    return n;
}

#else /* !SENTRY_WITH_REMOTE_UNWIND */

size_t
sentry__remote_unwind_thread(
    pid_t tid, sentry_remote_frame_t *frames, size_t max_frames)
{
    (void)tid;
    (void)frames;
    (void)max_frames;
    return 0;
}

#endif /* SENTRY_WITH_REMOTE_UNWIND */
