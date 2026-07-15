#ifndef SENTRY_SESSION_REPLAY_H_INCLUDED
#define SENTRY_SESSION_REPLAY_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_options.h"
#include "sentry_path.h"

/**
 * Metadata describing a captured replay clip, filled by
 * `sentry__session_replay_capture`.
 */
typedef struct sentry_session_replay_info_s {
    size_t size_bytes;
    uint32_t duration_ms;
    uint32_t width;
    uint32_t height;
    uint32_t frame_count;
    uint32_t frame_rate;
} sentry_session_replay_info_t;

/**
 * Captures a short retroactive video replay and saves it to the specified path.
 *
 * @param path The path where the replay should be saved (typically MP4).
 * @param duration_ms The requested duration in milliseconds.
 * @param pid The process ID whose output should be captured (0 = current
 * process).
 * @param end_ts_ms Unix timestamp (ms) at which the capture window ends,
 * typically the crash time (0 = now). Crash handling can run for a while
 * before the capture, while the crashed process stopped rendering at the
 * fault, so anchoring the window at "now" would miss most of the footage.
 * @param info Filled with the clip's metadata on success.
 *
 * Returns true if the replay was successfully captured and saved.
 */
bool sentry__session_replay_capture(const sentry_path_t *path,
    uint32_t duration_ms, uint32_t pid, uint64_t end_ts_ms,
    sentry_session_replay_info_t *info);

/**
 * Captures the replay for the crash described by `crash_event` and stages it
 * as one or more segments `<database>/replays/replay-<id>-<segment>.{mp4,json}`
 * so `sentry__session_replay_flush_pending` can pick them up. The configured
 * duration is split into segment windows (newest ending at the crash time) so
 * each segment's replay_video envelope stays under the ingestion size limit;
 * segments without footage or over the size ceiling are dropped. The replay id
 * is taken from the event's `contexts.replay.replay_id`; does nothing if it is
 * missing. Returns true if at least one segment was staged.
 */
bool sentry__session_replay_capture_staged(
    const sentry_options_t *options, sentry_value_t crash_event, uint32_t pid);

/**
 * Returns whether the embedder's `<database>/replays/` staging directory holds
 * a replay clip, so callers can skip crash-event work when nothing is staged.
 */
bool sentry__session_replay_has_pending(const sentry_options_t *options);

/**
 * Build and send the session-replay envelopes for the crash described by
 * `scope_source`. The replay is identified by the crash event's
 * `contexts.replay.replay_id` and staged as segment pairs
 * `<database>/replays/replay-<id>-<segment>.{json,mp4}` by
 * `sentry__session_replay_capture_staged`, or as a single un-suffixed
 * `replay-<id>.{json,mp4}` pair by an embedder recorder. One envelope is sent
 * per segment, in ascending segment order.
 *
 * Runs only on a crash. The native crash daemon calls it out-of-process with
 * the live transport, delivering the replay same-session. The crashpad,
 * breakpad and inproc backends call it from their crash handlers with a
 * run-dir disk transport, so the replay envelope is persisted next to the
 * crash envelope and delivered on the next launch. The sidecar and mp4 are
 * consumed once and removed after the flush attempt, regardless of whether
 * the envelope was built or sent, so a failed flush is not retried.
 *
 * `scope_source` is the crash event (`<run>/__sentry-event`); its scope fields
 * and trace id are copied onto the replay, and its timestamp ends the replay
 * window. If it is NULL or carries no `contexts.replay.replay_id`, nothing is
 * flushed.
 */
void sentry__session_replay_flush_pending(const sentry_options_t *options,
    sentry_transport_t *transport, sentry_value_t scope_source);

#endif
