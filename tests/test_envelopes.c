#include "../src/sentry_envelope.h"
#include "../src/sentry_value.h"
#include "sentry_testsupport.h"
#include <sentry.h>

static bool
basic_event_request_callback(sentry_prepared_http_request_t *req,
    const sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    assert_string_equal(req->method, "POST");
    assert_string_equal(req->url, "https://sentry.invalid:443/api/42/store/");
    assert_string_equal(req->payload,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}");
    assert_false(req->payload_owned);

    sentry__prepared_http_request_free(req);

    return true;
}

SENTRY_TEST(basic_http_request_preparation_for_event)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_init(options);

    uint64_t called = 0;
    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry__envelope_add_event(envelope, event);
    sentry__envelope_for_each_request(
        envelope, basic_event_request_callback, &called);
    sentry_envelope_free(envelope);
    assert_int_equal(called, 1);

    sentry_shutdown();
}

static bool
with_attachment_request_callback(sentry_prepared_http_request_t *req,
    const sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    if (*called == 1) {
        assert_string_equal(req->method, "POST");
        assert_string_equal(
            req->url, "https://sentry.invalid:443/api/42/store/");
        assert_string_equal(req->payload,
            "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}");
        assert_false(req->payload_owned);
    } else {
        assert_string_equal(req->method, "POST");
        assert_string_equal(req->url,
            "https://sentry.invalid:443/api/42/events/"
            "c993afb6-b4ac-48a6-b61b-2558e601d65d/attachments/");
        assert_string_equal(
            req->payload, "--0220b54a-d050-42ef-954a-ac481dc924db-boundary-\r\n\
content-type:application/octet-stream\r\n\
content-disposition:form-data;name=\"attachment\";filename=\"attachment.bin\"\r\n\
\r\n\
Hello World!\r\n\
--0220b54a-d050-42ef-954a-ac481dc924db-boundary---");
        assert_true(req->payload_owned);
    }

    sentry__prepared_http_request_free(req);

    return true;
}

SENTRY_TEST(basic_http_request_preparation_for_event_with_attachment)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_init(options);

    uint64_t called = 0;
    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry__envelope_add_event(envelope, event);
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");
    sentry__envelope_for_each_request(
        envelope, with_attachment_request_callback, &called);
    sentry_envelope_free(envelope);
    assert_int_equal(called, 2);

    sentry_shutdown();
}

static bool
minidump_request_callback(sentry_prepared_http_request_t *req,
    const sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    assert_string_equal(req->method, "POST");
    assert_string_equal(
        req->url, "https://sentry.invalid:443/api/42/minidump/?sentry_key=foo");
    assert_string_equal(
        req->payload, "--0220b54a-d050-42ef-954a-ac481dc924db-boundary-\r\n\
content-type:application/octet-stream\r\n\
content-disposition:form-data;name=\"attachment\";filename=\"attachment.bin\"\r\n\
\r\n\
Hello World!\r\n\
--0220b54a-d050-42ef-954a-ac481dc924db-boundary-\r\n\
content-type:application/x-minidump\r\n\
content-disposition:form-data;name=\"uploaded_file_minidump\";filename=\"minidump.dmp\"\r\n\
\r\n\
MDMP\r\n\
--0220b54a-d050-42ef-954a-ac481dc924db-boundary---");
    assert_true(req->payload_owned);

    sentry__prepared_http_request_free(req);

    return true;
}

SENTRY_TEST(basic_http_request_preparation_for_minidump)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_init(options);

    uint64_t called = 0;
    sentry_envelope_t *envelope = sentry__envelope_new();
    char dmp[] = "MDMP";
    sentry__envelope_add_from_buffer(
        envelope, dmp, sizeof(dmp) - 1, "minidump");
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");
    sentry__envelope_for_each_request(
        envelope, minidump_request_callback, &called);
    sentry_envelope_free(envelope);
    assert_int_equal(called, 1);

    sentry_shutdown();
}