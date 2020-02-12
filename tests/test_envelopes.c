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

    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/store/");
    TEST_CHECK_STRING_EQUAL(req->payload,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}");
    TEST_CHECK(!req->payload_owned);

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
    TEST_CHECK_INT_EQUAL(called, 1);

    sentry_shutdown();
}

static bool
with_attachment_request_callback(sentry_prepared_http_request_t *req,
    const sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    if (*called == 1) {
        TEST_CHECK_STRING_EQUAL(req->method, "POST");
        TEST_CHECK_STRING_EQUAL(
            req->url, "https://sentry.invalid:443/api/42/store/");
        TEST_CHECK_STRING_EQUAL(req->payload,
            "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}");
        TEST_CHECK(!req->payload_owned);
    } else {
        TEST_CHECK_STRING_EQUAL(req->method, "POST");
        TEST_CHECK_STRING_EQUAL(req->url,
            "https://sentry.invalid:443/api/42/events/"
            "c993afb6-b4ac-48a6-b61b-2558e601d65d/attachments/");
        TEST_CHECK_STRING_EQUAL(
            req->payload, "--0220b54a-d050-42ef-954a-ac481dc924db-boundary-\r\n\
content-type:application/octet-stream\r\n\
content-disposition:form-data;name=\"attachment\";filename=\"attachment.bin\"\r\n\
\r\n\
Hello World!\r\n\
--0220b54a-d050-42ef-954a-ac481dc924db-boundary---");
        TEST_CHECK(req->payload_owned);
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
    TEST_CHECK_INT_EQUAL(called, 2);

    sentry_shutdown();
}

static bool
minidump_request_callback(sentry_prepared_http_request_t *req,
    const sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/minidump/?sentry_key=foo");
    TEST_CHECK_STRING_EQUAL(
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
    TEST_CHECK(req->payload_owned);

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
    TEST_CHECK_INT_EQUAL(called, 1);

    sentry_shutdown();
}

SENTRY_TEST(serialize_envelope)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_init(options);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry_value_set_by_key(event, "some-context", sentry_value_new_null());
    sentry__envelope_add_event(envelope, event);

    char dmp[] = "MDMP";
    sentry__envelope_add_from_buffer(
        envelope, dmp, sizeof(dmp) - 1, "minidump");
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    sentry__envelope_serialize_into_stringbuilder(envelope, &sb);
    char *str = sentry__stringbuilder_into_string(&sb);

    TEST_CHECK_STRING_EQUAL(str,
        "{\"dsn\":\"https://foo@sentry.invalid/42\",\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n\
{\"type\":\"event\",\"length\":71}\n\
{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"some-context\":null}\n\
{\"type\":\"minidump\",\"length\":4}\n\
MDMP\n\
{\"type\":\"attachment\",\"length\":12}\n\
Hello World!");

    sentry_envelope_free(envelope);
    sentry_free(str);

    sentry_shutdown();
}
