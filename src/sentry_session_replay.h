#ifndef SENTRY_SESSION_REPLAY_H_INCLUDED
#define SENTRY_SESSION_REPLAY_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_options.h"
#include "sentry_path.h"

/**
 * Captures a short retroactive video clip and saves it to the specified path.
 *
 * @param path The path where the clip should be saved (typically MP4).
 * @param duration_ms The requested duration in milliseconds.
 * @param pid The process ID whose output should be captured (0 = current
 * process).
 *
 * Returns true if the clip was successfully captured and saved.
 */
bool sentry__session_replay_capture(
    const sentry_path_t *path, uint32_t duration_ms, uint32_t pid);

/**
 * Returns the path where a session replay clip should be saved.
 */
sentry_path_t *sentry__session_replay_get_path(const sentry_options_t *options);

#endif
