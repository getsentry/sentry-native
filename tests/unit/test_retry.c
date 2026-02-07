#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_retry.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_uuid.h"
#include "sentry_value.h"

#define NUM_ENVELOPES 3

static uint64_t g_timestamps[NUM_ENVELOPES];
static int g_call_count;

static sentry_send_result_t
recording_retry_func(void *envelope, void *state)
{
    (void)envelope;
    (void)state;
    if (g_call_count < NUM_ENVELOPES) {
        g_timestamps[g_call_count] = sentry__monotonic_time();
    }
    g_call_count++;
    return SENTRY_SEND_SUCCESS;
}

SENTRY_TEST(retry_throttle)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, 5);
    sentry_init(options);

    if (!sentry__transport_can_retry(options->transport)) {
        sentry_close();
        SKIP_TEST();
    }

    sentry__transport_set_retry_func(options->transport, recording_retry_func);

    sentry_path_t *retry_path
        = sentry__path_join_str(options->database_path, "retry");
    TEST_ASSERT(!!retry_path);
    sentry__path_remove_all(retry_path);

    sentry_retry_t *retry = sentry__retry_new(options->database_path, 5, false);
    TEST_ASSERT(!!retry);

    g_call_count = 0;
    memset(g_timestamps, 0, sizeof(g_timestamps));

    for (int i = 0; i < NUM_ENVELOPES; i++) {
        sentry_envelope_t *envelope = sentry__envelope_new();
        TEST_ASSERT(!!envelope);
        sentry_uuid_t event_id = sentry_uuid_new_v4();
        sentry_value_t event = sentry__value_new_event_with_id(&event_id);
        sentry__envelope_add_event(envelope, event);
        TEST_CHECK(sentry__retry_write_envelope(retry, envelope));
        sentry_envelope_free(envelope);
    }

    sentry__retry_process_envelopes(options);

    sentry_bgworker_t *bgw = sentry__transport_get_bgworker(options->transport);
    TEST_ASSERT(!!bgw);
    for (int i = 0; i < 20 && g_call_count < NUM_ENVELOPES; i++) {
        sentry__bgworker_flush(bgw, 500);
    }

    TEST_CHECK_INT_EQUAL(g_call_count, NUM_ENVELOPES);

    for (int i = 1; i < g_call_count && i < NUM_ENVELOPES; i++) {
        uint64_t delta = g_timestamps[i] - g_timestamps[i - 1];
        TEST_CHECK(delta >= 100);
        TEST_MSG("gap[%d]: expected >= 100ms, got %llu ms", i,
            (unsigned long long)delta);
    }

    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry_close();
}
