#ifndef SENTRY_REMOTE_UNWIND_H_INCLUDED
#define SENTRY_REMOTE_UNWIND_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define SENTRY_REMOTE_UNWIND_MAX_SYMBOL 256

typedef struct {
    uint64_t ip;
    char symbol[SENTRY_REMOTE_UNWIND_MAX_SYMBOL];
    uint64_t symbol_offset;
} sentry_remote_frame_t;

/**
 * Remotely unwind a thread's stack using libunwind ptrace accessors.
 * Handles ptrace attach/detach internally.
 *
 * @param tid Thread ID to unwind
 * @param frames Output array of frames
 * @param max_frames Maximum number of frames to capture
 * @return Number of frames captured, or 0 on failure
 */
size_t sentry__remote_unwind_thread(
    pid_t tid, sentry_remote_frame_t *frames, size_t max_frames);

#endif
