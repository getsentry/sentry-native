#ifndef SENTRY_SESSION_REPLAY_H_INCLUDED
#define SENTRY_SESSION_REPLAY_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_options.h"
#include "sentry_path.h"

/**
 * Captures a short retroactive video replay and saves it to the specified path.
 *
 * @param path The path where the replay should be saved (typically MP4).
 * @param duration_ms The requested duration in milliseconds.
 * @param pid The process ID whose output should be captured (0 = current
 * process).
 *
 * Returns true if the replay was successfully captured and saved.
 */
bool sentry__session_replay_capture(
    const sentry_path_t *path, uint32_t duration_ms, uint32_t pid);

/**
 * Returns the path where a session replay should be saved.
 */
sentry_path_t *sentry__session_replay_get_path(const sentry_options_t *options);

/**
 * Build and send any pending session-replay envelopes staged by an embedder in
 * `<database>/replays/` (a `replay-<id>.json` metadata sidecar next to its mp4).
 *
 * For each pending replay this parses the sidecar, reads the mp4, constructs the
 * `replay_video` envelope and hands it to `transport`, then deletes the sources
 * so it is not re-sent. `end_timestamp_sec` is the authoritative end-of-window
 * (crash) time; when <= 0 the `<database>/last_crash` marker is consulted, and
 * failing that the sidecar's own end timestamp is used.
 *
 * Called out-of-process by the daemon at crash time (same-session delivery) and
 * at `sentry_init` for the other backends (next-launch delivery, gated on a
 * crash having occurred).
 */
void sentry__session_replay_flush_pending(const sentry_options_t *options,
    sentry_transport_t *transport, double end_timestamp_sec);

#endif
