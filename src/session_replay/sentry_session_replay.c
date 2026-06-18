#include "sentry_session_replay.h"

#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_scope.h"
#include "sentry_string.h"
#include "sentry_value.h"

#include "../vendor/mpack.h"

#include <stdio.h>

sentry_path_t *
sentry__session_replay_get_path(const sentry_options_t *options)
{
    return sentry__path_join_str(options->run->run_path, "session-replay.mp4");
}

void
sentry_capture_session_replay(
    const char *video_path, sentry_value_t replay_event, sentry_value_t recording)
{
    if (!video_path) {
        sentry_value_decref(replay_event);
        sentry_value_decref(recording);
        return;
    }

    // Read the video the embedder recorded.
    sentry_path_t *vpath = sentry__path_from_str(video_path);
    size_t video_len = 0;
    char *video = vpath ? sentry__path_read_to_buffer(vpath, &video_len) : NULL;
    sentry__path_free(vpath);
    if (!video || video_len == 0) {
        sentry_free(video);
        sentry_value_decref(replay_event);
        sentry_value_decref(recording);
        return;
    }

    SENTRY_WITH_OPTIONS (options) {
        // The embedder owns the replay id; it lives on the event it built.
        char *replay_id = NULL;
        const char *rid = sentry_value_as_string(
            sentry_value_get_by_key(replay_event, "replay_id"));
        if (rid && rid[0]) {
            replay_id = sentry__string_clone(rid);
        }
        if (!replay_id) {
            break; // nothing to key the envelope on
        }

        // Enrich the embedder-built event from the live scope: tags, contexts,
        // user, release/environment, os/device/sdk, and contexts.trace.
        SENTRY_WITH_SCOPE (scope) {
            sentry__scope_apply_to_event(
                scope, options, replay_event, SENTRY_SCOPE_NONE);
            // TODO(phase 2): map scope->breadcrumbs (and logs) into rrweb
            // breadcrumb events and append them to `recording` here.
        }

        // trace_ids <- contexts.trace.trace_id (added by scope apply)
        sentry_value_t trace_id = sentry_value_get_by_key(
            sentry_value_get_by_key(
                sentry_value_get_by_key(replay_event, "contexts"), "trace"),
            "trace_id");
        sentry_value_t trace_ids = sentry_value_new_list();
        if (!sentry_value_is_null(trace_id)) {
            sentry_value_incref(trace_id);
            sentry_value_append(trace_ids, trace_id);
        }
        sentry_value_set_by_key(replay_event, "trace_ids", trace_ids);

        // Serialize the event and the embedder-built rrweb recording list.
        size_t event_len = 0;
        char *event_json = sentry__value_to_json(replay_event, &event_len);
        size_t rrweb_len = 0;
        char *rrweb_json = sentry__value_to_json(recording, &rrweb_len);

        if (event_json && rrweb_json) {
            // replay_recording = `{"segment_id":N}\n` + the rrweb array.
            const int32_t segment_id = sentry_value_as_int32(
                sentry_value_get_by_key(replay_event, "segment_id"));
            char hdr[48];
            int hdr_len
                = snprintf(hdr, sizeof(hdr), "{\"segment_id\":%d}\n", segment_id);

            sentry_stringbuilder_t rb;
            sentry__stringbuilder_init(&rb);
            sentry__stringbuilder_append_buf(&rb, hdr, (size_t)hdr_len);
            sentry__stringbuilder_append_buf(&rb, rrweb_json, rrweb_len);
            size_t recording_len = sentry__stringbuilder_len(&rb);
            char *recording_buf = sentry__stringbuilder_into_string(&rb);

            // replay_video item body: msgpack map of three raw blobs, framed with
            // the vendored mpack writer.
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
                sentry_envelope_t *envelope = sentry__envelope_new();
                if (envelope) {
                    // Relay keys the replay on the envelope event_id.
                    sentry__envelope_set_header(envelope, "event_id",
                        sentry_value_new_string(replay_id));
                    const char *dsn = sentry_options_get_dsn(options);
                    if (dsn && dsn[0]) {
                        sentry__envelope_set_header(
                            envelope, "dsn", sentry_value_new_string(dsn));
                    }
                    sentry__envelope_add_from_buffer(
                        envelope, body, body_len, "replay_video");
                    if (options->run) {
                        sentry__run_write_envelope(options->run, envelope);
                    }
                    sentry_envelope_free(envelope);
                }
            }
            sentry_free(body);
        }
        sentry_free(rrweb_json);
        sentry_free(event_json);
        sentry_free(replay_id);
    }

    sentry_free(video);
    sentry_value_decref(replay_event);
    sentry_value_decref(recording);
}
