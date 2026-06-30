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
 * Build and send the session-replay envelope(s) the embedder staged in
 * `<database>/replays/` (a `replay-<id>.json` sidecar next to its mp4). Native-
 * daemon-only: called out-of-process by the crash daemon, so it runs only on a
 * crash and delivers same-session. Each consumed sidecar and its mp4 are
 * removed once the envelope has been captured (malformed sidecars are removed
 * too), so the flush is idempotent. The SDK owns this cleanup -- the embedder
 * doesn't have to clear the `replays/` folder itself.
 *
 * `scope_source` is the crash event (`<run>/__sentry-event`); its scope fields
 * and trace id are copied onto the replay, and its timestamp ends the replay
 * window. Null skips enrichment.
 */
void sentry__session_replay_flush_pending(const sentry_options_t *options,
    sentry_transport_t *transport, sentry_value_t scope_source);

#endif
