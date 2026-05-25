#ifndef SENTRY_UNWINDER_H_INCLUDED
#define SENTRY_UNWINDER_H_INCLUDED

typedef struct {
    uintptr_t lo, hi;
} mem_range_t;

#if defined(SENTRY_PLATFORM_LINUX)

#    include <stddef.h>
#    include <stdint.h>
#    include <sys/types.h>

#    define SENTRY_REMOTE_UNWIND_MAX_SYMBOL 256

typedef struct {
    uint64_t ip;
    char symbol[SENTRY_REMOTE_UNWIND_MAX_SYMBOL];
    uint64_t symbol_offset;
} sentry_remote_frame_t;

size_t sentry__unwind_stack_from_thread(
    pid_t tid, sentry_remote_frame_t *frames, size_t max_frames);

#endif

#endif // SENTRY_UNWINDER_H_INCLUDED
