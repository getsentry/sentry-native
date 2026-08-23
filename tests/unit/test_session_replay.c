#include "sentry_envelope.h"
#include "sentry_path.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"

static const char *const REPLAY_ID = "0123456789abcdef0123456789abcdef";

static sentry_value_t
new_replay_metadata(const char *replay_id)
{
    sentry_value_t metadata = sentry_value_new_object();
    sentry_value_set_by_key(
        metadata, "replayId", sentry_value_new_string(replay_id));
    sentry_value_set_by_key(
        metadata, "replayType", sentry_value_new_string("buffer"));
    sentry_value_set_by_key(metadata, "segmentId", sentry_value_new_int32(0));
    sentry_value_set_by_key(
        metadata, "durationMs", sentry_value_new_double(1000.0));
    sentry_value_set_by_key(
        metadata, "endTimestampSec", sentry_value_new_double(1787515200.0));
    sentry_value_set_by_key(metadata, "width", sentry_value_new_int32(320));
    sentry_value_set_by_key(metadata, "height", sentry_value_new_int32(180));
    sentry_value_set_by_key(metadata, "frameCount", sentry_value_new_int32(1));
    sentry_value_set_by_key(metadata, "frameRate", sentry_value_new_int32(1));
    return metadata;
}

static sentry_envelope_t *
new_crash_envelope(const char *replay_id)
{
    sentry_value_t replay = sentry_value_new_object();
    sentry_value_set_by_key(
        replay, "replay_id", sentry_value_new_string(replay_id));
    sentry_value_t contexts = sentry_value_new_object();
    sentry_value_set_by_key(contexts, "replay", replay);

    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(event, "contexts", contexts);
    sentry_value_set_by_key(event, "timestamp",
        sentry_value_new_string("2026-08-23T12:00:00.000Z"));

    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_event(envelope, event);
    return envelope;
}

static void
receive_replay_video(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);
    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "replay_video");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        REPLAY_ID);
    size_t payload_len = 0;
    TEST_CHECK(!!sentry__envelope_item_get_payload(item, &payload_len));
    TEST_CHECK(payload_len > 0);
    sentry_envelope_free(envelope);
}

SENTRY_TEST(replay_video_submission)
{
    const char *video_path
        = SENTRY_TEST_PATH_PREFIX "sentry_test_replay_video.mp4";
    sentry_path_t *path = sentry__path_from_str(video_path);
    TEST_ASSERT(!!path);
    TEST_ASSERT(sentry__path_write_buffer(path, "mp4", 3) == 0);

    sentry_value_t metadata = new_replay_metadata(REPLAY_ID);
    sentry_envelope_t *crash = new_crash_envelope(REPLAY_ID);
    TEST_CHECK_INT_EQUAL(
        sentry_submit_replay_video(video_path, metadata, crash),
        SENTRY_REPLAY_VIDEO_NOT_INITIALIZED);

    uint64_t called = 0;
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport = sentry_transport_new(receive_replay_video);
    sentry_transport_set_state(transport, &called);
    sentry_options_set_transport(options, transport);
    TEST_ASSERT(sentry_init(options) == 0);

    TEST_CHECK_INT_EQUAL(
        sentry_submit_replay_video(video_path, metadata, crash),
        SENTRY_REPLAY_VIDEO_ACCEPTED);
    TEST_CHECK_INT_EQUAL(called, 1);

    sentry_close();
    sentry_envelope_free(crash);
    sentry_value_decref(metadata);
    sentry__path_remove(path);
    sentry__path_free(path);
}

SENTRY_TEST(replay_video_submission_validation)
{
    const char *video_path
        = SENTRY_TEST_PATH_PREFIX "sentry_test_replay_video_invalid.mp4";
    sentry_path_t *path = sentry__path_from_str(video_path);
    TEST_ASSERT(!!path);
    TEST_ASSERT(sentry__path_write_buffer(path, "mp4", 3) == 0);

    sentry_value_t metadata = new_replay_metadata(REPLAY_ID);
    sentry_envelope_t *mismatched
        = new_crash_envelope("11111111111111111111111111111111");
    TEST_CHECK_INT_EQUAL(
        sentry_submit_replay_video(video_path, metadata, mismatched),
        SENTRY_REPLAY_VIDEO_INVALID_INPUT);
    sentry_envelope_free(mismatched);

    sentry_value_set_by_key(metadata, "replayId",
        sentry_value_new_string("0123456789ABCDEF0123456789ABCDEF"));
    sentry_envelope_t *crash = new_crash_envelope(REPLAY_ID);
    TEST_CHECK_INT_EQUAL(
        sentry_submit_replay_video(video_path, metadata, crash),
        SENTRY_REPLAY_VIDEO_INVALID_INPUT);

    sentry_envelope_free(crash);
    sentry_value_decref(metadata);
    sentry__path_remove(path);
    sentry__path_free(path);
}
