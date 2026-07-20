#include "sentry_metrics.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"

#include "sentry_envelope.h"
#include <string.h>

typedef struct {
    volatile long called_count;
    bool has_validation_error;
} transport_validation_data_t;

static void
validate_metrics_envelope(sentry_envelope_t *envelope, void *data)
{
    transport_validation_data_t *validation_data = data;

    // Check we have at least one envelope item
    if (sentry__envelope_get_item_count(envelope) == 0) {
        validation_data->has_validation_error = true;
        sentry_envelope_free(envelope);
        return;
    }

    // Get the first item and check its type
    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    sentry_value_t type_header = sentry__envelope_item_get_header(item, "type");
    const char *type = sentry_value_as_string(type_header);

    // Only validate and count metric envelopes, skip others (e.g., session)
    if (strcmp(type, "trace_metric") == 0) {
        sentry__atomic_fetch_and_add(&validation_data->called_count, 1);
    }

    sentry_envelope_free(envelope);
}

SENTRY_TEST(metrics_count)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Record a counter metric
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_count("test.counter", 1, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);

    sentry_close();

    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
}

SENTRY_TEST(metrics_gauge)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Record a gauge metric
    TEST_CHECK_INT_EQUAL(sentry_metrics_gauge("test.gauge", 42.5,
                             SENTRY_UNIT_PERCENT, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);

    sentry_close();

    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
}

SENTRY_TEST(metrics_distribution)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Record a distribution metric
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_distribution("test.distribution", 123.456,
            SENTRY_UNIT_MILLISECOND, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);

    sentry_close();

    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
}

SENTRY_TEST(metrics_batch)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Batch buffer is 5 items in tests
    for (int i = 0; i < 5; i++) {
        TEST_CHECK_INT_EQUAL(
            sentry_metrics_count("test.counter", 1, sentry_value_new_null()),
            SENTRY_METRICS_RESULT_SUCCESS);
    }
    // Sleep up to 5s to allow first batch to flush
    for (int i = 0;
        i < 250 && sentry__atomic_fetch(&validation_data.called_count) < 1;
        i++) {
        sleep_ms(20);
    }
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_count("test.counter", 1, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);
    sentry_close();

    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 2);
}

SENTRY_TEST(metrics_with_attributes)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Record a metric with custom attributes
    sentry_value_t attributes = sentry_value_new_object();
    sentry_value_set_by_key(
        attributes, "environment", sentry_value_new_string("production"));
    sentry_value_set_by_key(
        attributes, "service", sentry_value_new_string("api"));

    TEST_CHECK_INT_EQUAL(sentry_metrics_count("requests.total", 1, attributes),
        SENTRY_METRICS_RESULT_SUCCESS);

    sentry_close();

    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
}

static sentry_value_t
before_send_metric_discard(sentry_value_t metric, void *data)
{
    (void)data;
    sentry_value_decref(metric);
    return sentry_value_new_null();
}

SENTRY_TEST(metrics_before_send_discard)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send_metric(
        options, before_send_metric_discard, NULL);

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // This metric should be discarded by the before_send hook
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_count("test.counter", 1, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_DISCARD);

    sentry_close();

    // Transport should not be called since the metric was discarded
    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 0);
}

static sentry_value_t
before_send_metric_modify(sentry_value_t metric, void *data)
{
    (void)data;
    // Modify the metric by adding a custom attribute
    sentry_value_t attributes = sentry_value_get_by_key(metric, "attributes");
    if (sentry_value_is_null(attributes)) {
        attributes = sentry_value_new_object();
        sentry_value_set_by_key(metric, "attributes", attributes);
    }
    sentry_value_set_by_key(
        attributes, "modified", sentry_value_new_bool(true));
    return metric;
}

SENTRY_TEST(metrics_before_send_modify)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send_metric(
        options, before_send_metric_modify, NULL);

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // This metric should be modified by the before_send hook
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_count("test.counter", 1, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);

    sentry_close();

    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 1);
}

SENTRY_TEST(metrics_disabled)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_metrics(options, false);

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);

    // These should return DISABLED since metrics were explicitly disabled
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_count("test.counter", 1, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_DISABLED);
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_gauge("test.gauge", 42.5, NULL, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_DISABLED);
    TEST_CHECK_INT_EQUAL(sentry_metrics_distribution("test.distribution", 123.0,
                             NULL, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_DISABLED);

    sentry_close();

    // Transport should not be called since metrics were explicitly disabled
    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 0);
}

SENTRY_TEST(metrics_force_flush)
{
    transport_validation_data_t validation_data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_metrics_envelope);
    sentry_transport_set_state(transport, &validation_data);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Record multiple metrics with force flush between each
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_count("counter.1", 1, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);
    sentry_flush(5000);
    TEST_CHECK_INT_EQUAL(
        sentry_metrics_gauge("gauge.1", 42.5, NULL, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);
    sentry_flush(5000);
    TEST_CHECK_INT_EQUAL(sentry_metrics_distribution("dist.1", 100.0,
                             SENTRY_UNIT_MILLISECOND, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);
    sentry_flush(5000);

    sentry_close();

    TEST_CHECK(!validation_data.has_validation_error);
    TEST_CHECK_INT_EQUAL(validation_data.called_count, 3);
}

static sentry_value_t g_captured_metric = { 0 };

static sentry_value_t
capture_metric(sentry_value_t metric, void *data)
{
    (void)data;
    g_captured_metric = metric;
    sentry_value_incref(metric);
    return metric;
}

static void
discard_envelope(sentry_envelope_t *envelope, void *data)
{
    (void)data;
    sentry_envelope_free(envelope);
}

SENTRY_TEST(metrics_default_attributes)
{
    g_captured_metric = sentry_value_new_null();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_environment(options, "test-env");
    sentry_options_set_release(options, "1.0.0");
    sentry_options_set_before_send_metric(options, capture_metric, NULL);

    sentry_transport_t *transport = sentry_transport_new(discard_envelope);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    sentry_metrics_count("test.metric", 1, sentry_value_new_null());
    sentry_close();

    // Validate trace_id is set directly on metric
    sentry_value_t trace_id
        = sentry_value_get_by_key(g_captured_metric, "trace_id");
    TEST_CHECK(!sentry_value_is_null(trace_id));

    // Validate attributes object exists
    sentry_value_t attributes
        = sentry_value_get_by_key(g_captured_metric, "attributes");
    TEST_CHECK(!sentry_value_is_null(attributes));

    // Validate default attributes are present with typed format
    sentry_value_t env
        = sentry_value_get_by_key(attributes, "sentry.environment");
    TEST_CHECK(!sentry_value_is_null(env));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(env, "value")),
        "test-env");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(env, "type")), "string");

    sentry_value_t release
        = sentry_value_get_by_key(attributes, "sentry.release");
    TEST_CHECK(!sentry_value_is_null(release));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(release, "value")),
        "1.0.0");

    sentry_value_t sdk_name
        = sentry_value_get_by_key(attributes, "sentry.sdk.name");
    TEST_CHECK(!sentry_value_is_null(sdk_name));

    sentry_value_t sdk_version
        = sentry_value_get_by_key(attributes, "sentry.sdk.version");
    TEST_CHECK(!sentry_value_is_null(sdk_version));

    sentry_value_decref(g_captured_metric);
}

SENTRY_TEST(metrics_global_attribute_no_field_leak)
{
    g_captured_metric = sentry_value_new_null();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send_metric(options, capture_metric, NULL);

    sentry_transport_t *transport = sentry_transport_new(discard_envelope);
    sentry_options_set_transport(options, transport);

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Per-call attributes must override global attributes atomically by key.
    // If both scopes define the same attribute, the per-call attribute replaces
    // the whole global attribute instead of inheriting fields such as `unit`.
    sentry_set_attribute("shared",
        sentry_value_new_attribute(sentry_value_new_string("global"), "ms"));

    sentry_value_t attrs = sentry_value_new_object();
    sentry_value_set_by_key(attrs, "shared",
        sentry_value_new_attribute(sentry_value_new_string("local"), NULL));

    TEST_CHECK_INT_EQUAL(sentry_metrics_count("test.metric", 1, attrs),
        SENTRY_METRICS_RESULT_SUCCESS);

    sentry_close();

    sentry_value_t attributes
        = sentry_value_get_by_key(g_captured_metric, "attributes");
    sentry_value_t shared = sentry_value_get_by_key(attributes, "shared");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(shared, "value")),
        "local");
    TEST_CHECK(sentry_value_is_null(sentry_value_get_by_key(shared, "unit")));

    sentry_value_decref(g_captured_metric);
}

static sentry_value_t
string_attribute(const char *value)
{
    return sentry_value_new_attribute(sentry_value_new_string(value), NULL);
}

static const char *
captured_attribute(const char *key)
{
    sentry_value_t attributes
        = sentry_value_get_by_key(g_captured_metric, "attributes");
    return sentry_value_as_string(sentry_value_get_by_key(
        sentry_value_get_by_key(attributes, key), "value"));
}

SENTRY_TEST(scope_capture_metric_attributes)
{
    g_captured_metric = sentry_value_new_null();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send_metric(options, capture_metric, NULL);
    sentry_options_set_transport(
        options, sentry_transport_new(discard_envelope));

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    sentry_set_attribute("from_global", string_attribute("global"));
    sentry_set_attribute("scope_over_global", string_attribute("global"));
    sentry_set_attribute("metric_over_all", string_attribute("global"));

    sentry_scope_t *scope = sentry_scope_new();
    sentry_scope_set_attribute(scope, "from_scope", string_attribute("scope"));
    sentry_scope_set_attribute(
        scope, "scope_over_global", string_attribute("scope"));
    sentry_scope_set_attribute(
        scope, "metric_over_all", string_attribute("scope"));

    sentry_value_t attrs = sentry_value_new_object();
    sentry_value_set_by_key(attrs, "from_metric", string_attribute("metric"));
    sentry_value_set_by_key(
        attrs, "metric_over_all", string_attribute("metric"));

    TEST_CHECK_INT_EQUAL(sentry_scope_capture_metric(scope, SENTRY_METRIC_COUNT,
                             "honks", sentry_value_new_int64(1), NULL, attrs),
        SENTRY_METRICS_RESULT_SUCCESS);

    // Each source contributes the attributes only it has...
    TEST_CHECK_STRING_EQUAL(captured_attribute("from_global"), "global");
    TEST_CHECK_STRING_EQUAL(captured_attribute("from_scope"), "scope");
    TEST_CHECK_STRING_EQUAL(captured_attribute("from_metric"), "metric");

    // ... and the most specific source wins the ones they share.
    TEST_CHECK_STRING_EQUAL(captured_attribute("scope_over_global"), "scope");
    TEST_CHECK_STRING_EQUAL(captured_attribute("metric_over_all"), "metric");

    sentry_scope_free(scope);
    sentry_value_decref(g_captured_metric);
    sentry_close();
}

SENTRY_TEST(scope_capture_metric_user_owned)
{
    g_captured_metric = sentry_value_new_null();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send_metric(options, capture_metric, NULL);
    sentry_options_set_transport(
        options, sentry_transport_new(discard_envelope));

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // A user-owned scope survives being captured with, so it can be reused.
    sentry_scope_t *scope = sentry_scope_new();
    sentry_scope_set_attribute(scope, "run", string_attribute("first"));

    TEST_CHECK_INT_EQUAL(
        sentry_scope_capture_metric(scope, SENTRY_METRIC_COUNT, "honks",
            sentry_value_new_int64(1), NULL, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);
    TEST_CHECK_STRING_EQUAL(captured_attribute("run"), "first");
    sentry_value_decref(g_captured_metric);

    // The scope was applied but not freed, so mutating and reusing it is safe
    // (a use-after-free here would trip the sanitizers).
    sentry_scope_set_attribute(scope, "run", string_attribute("second"));

    TEST_CHECK_INT_EQUAL(
        sentry_scope_capture_metric(scope, SENTRY_METRIC_COUNT, "honks",
            sentry_value_new_int64(1), NULL, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);
    TEST_CHECK_STRING_EQUAL(captured_attribute("run"), "second");

    sentry_scope_free(scope);
    sentry_value_decref(g_captured_metric);
    sentry_close();
}

SENTRY_TEST(scope_capture_metric_one_shot)
{
    g_captured_metric = sentry_value_new_null();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send_metric(options, capture_metric, NULL);
    sentry_options_set_transport(
        options, sentry_transport_new(discard_envelope));

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // A one-shot scope is applied, then freed by the capture
    // (a leak or a double-free would trip the sanitizers).
    sentry_scope_t *scope = sentry_local_scope_new();
    sentry_scope_set_attribute(scope, "from_scope", string_attribute("scope"));

    TEST_CHECK_INT_EQUAL(
        sentry_scope_capture_metric(scope, SENTRY_METRIC_COUNT, "honks",
            sentry_value_new_int64(1), NULL, sentry_value_new_null()),
        SENTRY_METRICS_RESULT_SUCCESS);
    TEST_CHECK_STRING_EQUAL(captured_attribute("from_scope"), "scope");

    sentry_value_decref(g_captured_metric);
    sentry_close();
}

SENTRY_TEST(metrics_reinit)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_init(options);
    sentry__metrics_wait_for_thread_startup();

    // Fill the buffer to trigger a flush on the batcher thread
    for (int i = 0; i < 5; i++) {
        sentry_metrics_count("metric", 1, sentry_value_new_null());
    }

    // Re-init immediately while the batcher thread is likely mid-flush.
    // This will deadlock if sentry__batcher_flush holds g_options_lock.
    SENTRY_TEST_OPTIONS_NEW(options2);
    sentry_options_set_dsn(options2, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_metrics(options2, true);

    sentry_init(options2);
    sentry_close();
}

SENTRY_THREAD_FN
metric_producer_thread(void *data)
{
    volatile long *produce = data;
    while (sentry__atomic_fetch(produce)) {
        sentry_metrics_count("metric", 1, sentry_value_new_null());
        sentry__thread_yield();
    }
    return 0;
}

// multiple threads produce metrics during SDK re-init
SENTRY_TEST(metrics_reinit_stress)
{
    volatile long produce = 1;

    sentry_threadid_t threads[8];
    for (int t = 0; t < 8; t++) {
        sentry__thread_init(&threads[t]);
    }

    for (int i = 0; i < 5; i++) {
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
        sentry_init(options);

        if (i == 0) {
            sentry__metrics_wait_for_thread_startup();
            for (int t = 0; t < 8; t++) {
                sentry__thread_spawn(
                    &threads[t], metric_producer_thread, (void *)&produce);
            }
        }

        sentry_close();
    }

    sentry__atomic_store(&produce, 0);
    for (int t = 0; t < 8; t++) {
        sentry__thread_join(threads[t]);
    }
}
