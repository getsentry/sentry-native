#include "sentry_session_replay.h"

#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_scope.h"
#include "sentry_string.h"
#include "sentry_utils.h"
#include "sentry_value.h"

#include "../vendor/mpack.h"

#include <stdio.h>

sentry_path_t *
sentry__session_replay_get_path(const sentry_options_t *options)
{
    return sentry__path_join_str(options->run->run_path, "session-replay.mp4");
}

// Resolve the database dir. Prefer the run path's parent: it is always the
// absolute run location. `options->database_path` is unreliable here because
// the crash daemon never overrides the default relative ".sentry-native" (set
// in sentry_options_new) with the app's absolute path, so it must only be a
// fallback. Caller owns the returned path.
static sentry_path_t *
session_replay_database_dir(const sentry_options_t *options)
{
    if (options->run && options->run->run_path) {
        return sentry__path_dir(options->run->run_path);
    }
    if (options->database_path) {
        return sentry__path_clone(options->database_path);
    }
    return NULL;
}

// Resolve `<database>/replays`. Caller owns the path.
static sentry_path_t *
session_replay_dir(const sentry_options_t *options)
{
    sentry_path_t *db_dir = session_replay_database_dir(options);
    if (!db_dir) {
        return NULL;
    }
    sentry_path_t *replays = sentry__path_join_str(db_dir, "replays");
    sentry__path_free(db_dir);
    return replays;
}

// Read the `<database>/last_crash` marker (ISO8601, written by the in-process
// crash handlers) and return the crash time in seconds, or 0 if unavailable.
static double
session_replay_marker_time_sec(const sentry_options_t *options)
{
    sentry_path_t *db_dir = session_replay_database_dir(options);
    if (!db_dir) {
        return 0.0;
    }
    sentry_path_t *marker = sentry__path_join_str(db_dir, "last_crash");
    sentry__path_free(db_dir);
    if (!marker) {
        return 0.0;
    }

    size_t len = 0;
    char *buf = sentry__path_read_to_buffer(marker, &len);
    sentry__path_free(marker);
    if (!buf || len == 0) {
        sentry_free(buf);
        return 0.0;
    }

    char iso[40];
    if (len >= sizeof(iso)) {
        len = sizeof(iso) - 1;
    }
    memcpy(iso, buf, len);
    iso[len] = '\0';
    sentry_free(buf);

    uint64_t usec = sentry__iso8601_to_usec(iso);
    return usec ? (double)usec / 1000000.0 : 0.0;
}

static int32_t
meta_int32(sentry_value_t meta, const char *key)
{
    return sentry_value_as_int32(sentry_value_get_by_key(meta, key));
}

static double
meta_double(sentry_value_t meta, const char *key)
{
    return sentry_value_as_double(sentry_value_get_by_key(meta, key));
}

// Build the replay_event payload from the recorder's metadata. The replay is
// associated with the error via the `replay` context on the event, so no
// `error_ids`/`trace_ids` are needed here (the former is deprecated).
static sentry_value_t
build_replay_event(sentry_value_t meta, const char *replay_id, double start_sec,
    double end_sec, int32_t segment_id)
{
    const char *replay_type
        = sentry_value_as_string(sentry_value_get_by_key(meta, "replayType"));

    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "type", sentry_value_new_string("replay_event"));
    sentry_value_set_by_key(event, "replay_type",
        sentry_value_new_string(
            replay_type && replay_type[0] ? replay_type : "buffer"));
    sentry_value_set_by_key(
        event, "segment_id", sentry_value_new_int32(segment_id));
    sentry_value_set_by_key(
        event, "replay_id", sentry_value_new_string(replay_id));
    sentry_value_set_by_key(
        event, "event_id", sentry_value_new_string(replay_id));
    sentry_value_set_by_key(
        event, "platform", sentry_value_new_string("native"));
    sentry_value_set_by_key(
        event, "timestamp", sentry_value_new_double(end_sec));
    sentry_value_set_by_key(
        event, "replay_start_timestamp", sentry_value_new_double(start_sec));
    sentry_value_set_by_key(event, "urls", sentry_value_new_list());
    return event;
}

// Build the rrweb recording list (meta event + video event) describing the
// clip.
static sentry_value_t
build_replay_recording(sentry_value_t meta, double start_sec,
    int32_t segment_id, double size_bytes, double duration_ms)
{
    const int32_t width = meta_int32(meta, "width");
    const int32_t height = meta_int32(meta, "height");
    const double ts_ms = start_sec * 1000.0;

    sentry_value_t meta_data = sentry_value_new_object();
    sentry_value_set_by_key(meta_data, "href", sentry_value_new_string(""));
    sentry_value_set_by_key(meta_data, "width", sentry_value_new_int32(width));
    sentry_value_set_by_key(
        meta_data, "height", sentry_value_new_int32(height));
    sentry_value_t meta_event = sentry_value_new_object();
    sentry_value_set_by_key(meta_event, "type", sentry_value_new_int32(4));
    sentry_value_set_by_key(
        meta_event, "timestamp", sentry_value_new_double(ts_ms));
    sentry_value_set_by_key(meta_event, "data", meta_data);

    sentry_value_t payload = sentry_value_new_object();
    sentry_value_set_by_key(
        payload, "segmentId", sentry_value_new_int32(segment_id));
    sentry_value_set_by_key(
        payload, "size", sentry_value_new_double(size_bytes));
    sentry_value_set_by_key(
        payload, "duration", sentry_value_new_double(duration_ms));
    sentry_value_set_by_key(
        payload, "encoding", sentry_value_new_string("h264"));
    sentry_value_set_by_key(
        payload, "container", sentry_value_new_string("mp4"));
    sentry_value_set_by_key(payload, "height", sentry_value_new_int32(height));
    sentry_value_set_by_key(payload, "width", sentry_value_new_int32(width));
    sentry_value_set_by_key(payload, "left", sentry_value_new_int32(0));
    sentry_value_set_by_key(payload, "top", sentry_value_new_int32(0));
    sentry_value_set_by_key(payload, "frameCount",
        sentry_value_new_int32(meta_int32(meta, "frameCount")));
    sentry_value_set_by_key(payload, "frameRate",
        sentry_value_new_int32(meta_int32(meta, "frameRate")));
    sentry_value_set_by_key(
        payload, "frameRateType", sentry_value_new_string("variable"));
    sentry_value_t video_data = sentry_value_new_object();
    sentry_value_set_by_key(
        video_data, "tag", sentry_value_new_string("video"));
    sentry_value_set_by_key(video_data, "payload", payload);
    sentry_value_t video_event = sentry_value_new_object();
    sentry_value_set_by_key(video_event, "type", sentry_value_new_int32(5));
    sentry_value_set_by_key(
        video_event, "timestamp", sentry_value_new_double(ts_ms));
    sentry_value_set_by_key(video_event, "data", video_data);

    sentry_value_t recording = sentry_value_new_list();
    sentry_value_append(recording, meta_event);
    sentry_value_append(recording, video_event);
    return recording;
}

// Build the replay_video envelope from a parsed metadata sidecar + its mp4.
// `end_sec` is the authoritative end-of-window (the crash time); when <= 0 the
// sidecar's own end timestamp is used. Returns NULL on failure.
static sentry_envelope_t *
build_replay_envelope(const sentry_options_t *options, sentry_value_t meta,
    const sentry_path_t *mp4_path, double end_sec)
{
    if (sentry_value_is_null(meta) || !mp4_path) {
        return NULL;
    }

    const char *replay_id
        = sentry_value_as_string(sentry_value_get_by_key(meta, "replayId"));
    if (!replay_id || !replay_id[0]) {
        return NULL;
    }

    size_t video_len = 0;
    char *video = sentry__path_read_to_buffer(mp4_path, &video_len);
    if (!video || video_len == 0) {
        sentry_free(video);
        return NULL;
    }

    const double duration_ms = meta_double(meta, "durationMs");
    if (end_sec <= 0.0) {
        end_sec = meta_double(meta, "endTimestampSec");
    }
    const double start_sec = end_sec - duration_ms / 1000.0;
    const int32_t segment_id = meta_int32(meta, "segmentId");

    sentry_value_t event
        = build_replay_event(meta, replay_id, start_sec, end_sec, segment_id);
    sentry_value_t recording = build_replay_recording(
        meta, start_sec, segment_id, (double)video_len, duration_ms);

    sentry_envelope_t *envelope = NULL;

    size_t event_len = 0;
    char *event_json = sentry__value_to_json(event, &event_len);
    size_t rrweb_len = 0;
    char *rrweb_json = sentry__value_to_json(recording, &rrweb_len);

    if (event_json && rrweb_json) {
        // replay_recording = `{"segment_id":N}\n` + the rrweb array.
        char hdr[48];
        int hdr_len
            = snprintf(hdr, sizeof(hdr), "{\"segment_id\":%d}\n", segment_id);

        sentry_stringbuilder_t rb;
        sentry__stringbuilder_init(&rb);
        sentry__stringbuilder_append_buf(&rb, hdr, (size_t)hdr_len);
        sentry__stringbuilder_append_buf(&rb, rrweb_json, rrweb_len);
        size_t recording_len = sentry__stringbuilder_len(&rb);
        char *recording_buf = sentry__stringbuilder_into_string(&rb);

        // replay_video item body: msgpack map of three raw blobs.
        mpack_writer_t writer;
        char *body = NULL;
        size_t body_len = 0;
        mpack_writer_init_growable(&writer, &body, &body_len);
        mpack_start_map(&writer, 3);
        mpack_write_cstr(&writer, "replay_event");
        mpack_write_bin(&writer, event_json, (uint32_t)event_len);
        mpack_write_cstr(&writer, "replay_recording");
        mpack_write_bin(&writer, recording_buf, (uint32_t)recording_len);
        mpack_write_cstr(&writer, "replay_video");
        mpack_write_bin(&writer, video, (uint32_t)video_len);
        mpack_finish_map(&writer);
        bool body_ok = mpack_writer_destroy(&writer) == mpack_ok;

        sentry_free(recording_buf);

        if (body_ok && body) {
            envelope = sentry__envelope_new();
            if (envelope) {
                // Relay keys the replay on the envelope event_id.
                sentry__envelope_set_header(
                    envelope, "event_id", sentry_value_new_string(replay_id));
                const char *dsn = sentry_options_get_dsn(options);
                if (dsn && dsn[0]) {
                    sentry__envelope_set_header(
                        envelope, "dsn", sentry_value_new_string(dsn));
                }
                sentry__envelope_add_from_buffer(
                    envelope, body, body_len, "replay_video");
            }
        }
        sentry_free(body);
    }

    sentry_free(rrweb_json);
    sentry_free(event_json);
    sentry_value_decref(event);
    sentry_value_decref(recording);
    sentry_free(video);
    return envelope;
}

void
sentry__session_replay_flush_pending(const sentry_options_t *options,
    sentry_transport_t *transport, double end_timestamp_sec)
{
    if (!options || !transport) {
        return;
    }

    sentry_path_t *dir = session_replay_dir(options);
    if (!dir) {
        return;
    }
    if (!sentry__path_is_dir(dir)) {
        sentry__path_free(dir);
        return;
    }

    // Resolve the end-of-window (crash) time once. The caller may pass it
    // explicitly (e.g. crashpad's completed-report time for the WER path);
    // otherwise fall back to the on-disk crash marker.
    double end_sec = end_timestamp_sec;
    if (end_sec <= 0.0) {
        end_sec = session_replay_marker_time_sec(options);
    }

    sentry_pathiter_t *iter = sentry__path_iter_directory(dir);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        if (!sentry__path_ends_with(file, ".json")) {
            continue;
        }

        size_t json_len = 0;
        char *json = sentry__path_read_to_buffer(file, &json_len);
        if (!json) {
            continue;
        }
        sentry_value_t meta = sentry__value_from_json(json, json_len);
        sentry_free(json);

        const char *video_filename = sentry_value_as_string(
            sentry_value_get_by_key(meta, "videoFilename"));
        sentry_path_t *mp4_path = (video_filename && video_filename[0])
            ? sentry__path_join_str(dir, video_filename)
            : NULL;

        sentry_envelope_t *envelope
            = build_replay_envelope(options, meta, mp4_path, end_sec);
        if (envelope) {
            // Hands ownership to the transport (queued for async send); the
            // transport re-persists any unsent envelope to the run folder on
            // shutdown, so removing the sources below is safe.
            sentry__capture_envelope(transport, envelope, options);
        }

        // Delete sources right after capture so the same replay is not re-sent
        // on the next launch (where `last_crash` would still be set).
        if (mp4_path) {
            sentry__path_remove(mp4_path);
            sentry__path_free(mp4_path);
        }
        sentry__path_remove(file);
        sentry_value_decref(meta);
    }
    sentry__pathiter_free(iter);
    sentry__path_free(dir);
}
