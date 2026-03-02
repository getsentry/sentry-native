#include "sentry_client_report.h"
#include "sentry_envelope.h"
#include "sentry_testsupport.h"
#include "sentry_value.h"

SENTRY_TEST(client_report_discard)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    TEST_CHECK(!sentry__client_report_has_pending());

    sentry__client_report_discard(
        SENTRY_DISCARD_REASON_SAMPLE_RATE, SENTRY_DATA_CATEGORY_ERROR, 2);
    sentry__client_report_discard(
        SENTRY_DISCARD_REASON_BEFORE_SEND, SENTRY_DATA_CATEGORY_TRANSACTION, 1);

    TEST_CHECK(sentry__client_report_has_pending());

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_CHECK(!!envelope);

    sentry_envelope_item_t *item
        = sentry__client_report_into_envelope(envelope);
    TEST_CHECK(!!item);

    TEST_CHECK(!sentry__client_report_has_pending());

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "client_report");

    size_t payload_len = 0;
    const char *payload = sentry__envelope_item_get_payload(item, &payload_len);
    TEST_CHECK(!!payload);
    TEST_CHECK(payload_len > 0);

    sentry_value_t report = sentry__value_from_json(payload, payload_len);
    TEST_CHECK(!sentry_value_is_null(report));

    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(report, "timestamp")));

    sentry_value_t discarded
        = sentry_value_get_by_key(report, "discarded_events");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(discarded), 2);

    sentry_value_t entry0 = sentry_value_get_by_index(discarded, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "reason")),
        "sample_rate");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "category")),
        "error");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry0, "quantity")), 2);

    sentry_value_t entry1 = sentry_value_get_by_index(discarded, 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "reason")),
        "before_send");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "category")),
        "transaction");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry1, "quantity")), 1);

    sentry_value_decref(report);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(client_report_discard_envelope)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    TEST_CHECK(!sentry__client_report_has_pending());

    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "event");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "session");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "transaction");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "client_report");

    sentry__client_report_discard_envelope(
        envelope, SENTRY_DISCARD_REASON_NETWORK_ERROR);

    TEST_CHECK(sentry__client_report_has_pending());

    sentry_envelope_t *carrier = sentry__envelope_new();
    sentry_envelope_item_t *item = sentry__client_report_into_envelope(carrier);
    TEST_CHECK(!!item);

    size_t payload_len = 0;
    const char *payload = sentry__envelope_item_get_payload(item, &payload_len);
    sentry_value_t report = sentry__value_from_json(payload, payload_len);

    sentry_value_t discarded
        = sentry_value_get_by_key(report, "discarded_events");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(discarded), 3);

    sentry_value_t entry0 = sentry_value_get_by_index(discarded, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "category")),
        "error");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry0, "quantity")), 1);

    sentry_value_t entry1 = sentry_value_get_by_index(discarded, 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "category")),
        "session");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry1, "quantity")), 1);

    sentry_value_t entry2 = sentry_value_get_by_index(discarded, 2);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry2, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry2, "category")),
        "transaction");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry2, "quantity")), 1);

    sentry_value_decref(report);
    sentry_envelope_free(carrier);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(client_report_none)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    TEST_CHECK(!sentry__client_report_has_pending());

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_CHECK(!!envelope);

    sentry_envelope_item_t *item
        = sentry__client_report_into_envelope(envelope);
    TEST_CHECK(!item);

    sentry_envelope_free(envelope);
    sentry_close();
}
