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
 * so it is not re-sent.
 *
 * Session replay is a native-daemon-only feature: this is called out-of-process
 * by the crash daemon, so it runs only when a crash occurred (delivery is
 * same-session and gating is inherent — no next-launch path).
 *
 * `scope_source` is the crashed session's event read from `<run>/__sentry-event`,
 * already enriched in-process via `sentry__scope_apply_to_event`. Its
 * tags/contexts/release/environment/user/sdk are copied onto the replay_event,
 * `contexts.trace.trace_id` is lifted into `trace_ids`, and its `timestamp` marks
 * the end of the replay window. Pass a null value to skip enrichment.
 */
void sentry__session_replay_flush_pending(const sentry_options_t *options,
    sentry_transport_t *transport, sentry_value_t scope_source);

#endif
