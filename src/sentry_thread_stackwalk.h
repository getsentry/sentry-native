#ifndef SENTRY_THREAD_STACKWALK_H_INCLUDED
#define SENTRY_THREAD_STACKWALK_H_INCLUDED

#include "sentry_boot.h"

#include <stddef.h>
#include <stdint.h>

// A platform thread stackwalker is compiled in only for the targets below.
// On any other target `sentry__thread_stackwalk` has no definition.
#if defined(SENTRY_PLATFORM_MACOS) || defined(SENTRY_PLATFORM_WINDOWS)         \
    || defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    define SENTRY_HAS_THREAD_STACKWALK 1
#else
#    define SENTRY_HAS_THREAD_STACKWALK 0
#endif

// Windows x64 (MSVC) unwinds suspended threads without taking any lock. Other
// Windows targets use dbghelp, which can block on a lock that the suspended
// thread holds.
#if defined(SENTRY_PLATFORM_WINDOWS) && defined(_MSC_VER) && defined(_M_X64)
#    define SENTRY_THREAD_STACKWALK_RTL_UNWIND 1
#else
#    define SENTRY_THREAD_STACKWALK_RTL_UNWIND 0
#endif

#if SENTRY_HAS_THREAD_STACKWALK
// Captures the call stack of another thread by suspending it, walking its
// stack, and writing the instruction pointers into `ips_out` (IPs only; no
// symbolication while suspended). Returns the number of frames captured.
size_t sentry__thread_stackwalk(
    uint64_t target_tid, void **ips_out, size_t max_frames);

#    define SENTRY_THREAD_NAME_MAX 64

typedef struct {
    uint64_t tid;
    char name[SENTRY_THREAD_NAME_MAX];
} sentry_thread_info_t;

// Lists the threads of the current process: at most `max` entries are written
// to `out`. Thread ids use the same id space as `sentry__thread_stackwalk`.
// Names are UTF-8 and empty when unknown. Returns the number of entries.
size_t sentry__thread_list(sentry_thread_info_t *out, size_t max);
#endif

#endif
