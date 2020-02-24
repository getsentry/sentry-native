#include "sentry_envelope.h"
#include "sentry_testsupport.h"
#include "sentry_value.h"
#include <sentry.h>

typedef struct {
    uint64_t called_envelope;
    uint64_t called_request;
    sentry_stringbuilder_t serialized_envelope;
} sentry_attachments_testdata_t;

static bool
envelope_request_callback(sentry_prepared_http_request_t *req,
    const sentry_envelope_t *envelope, void *_data)
{
    sentry_attachments_testdata_t *data = _data;
    data->called_request += 1;

    bool found_header = false;
    for (size_t i = 0; i < req->headers_len; i++) {
        if (strcmp(req->headers[i].key, "content-type") == 0) {
            found_header = true;
            TEST_CHECK_STRING_EQUAL(
                req->headers[i].value, "application/x-sentry-envelope");
        }
    }
    TEST_CHECK(found_header);

    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/store/");
    TEST_CHECK(
        strstr(req->payload,
            "{\"dsn\":\"https://foo@sentry.invalid/"
            "42\",\"event_id\":\"4c035723-8638-4c3a-923f-2ab9d08b4018\"}\n"
            "{\"type\":\"event\",")
        != NULL);
    TEST_CHECK(!req->payload_owned);

    sentry__prepared_http_request_free(req);

    return true;
}

static void
send_envelope(sentry_envelope_t *envelope, void *_data)
{
    sentry_attachments_testdata_t *data = _data;
    data->called_envelope += 1;

    sentry__envelope_for_each_request(
        envelope, envelope_request_callback, _data);

    sentry__envelope_serialize_into_stringbuilder(
        envelope, &data->serialized_envelope);
}

#ifdef __ANDROID__
#    define PREFIX "/data/local/tmp/"
#else
#    define PREFIX ""
#endif

SENTRY_TEST(enumerating_database)
{
    sentry_path_t *path = sentry__path_from_str(PREFIX ".test-db");
    sentry__path_remove_all(path);

    sentry_attachments_testdata_t testdata;
    testdata.called_envelope = 0;
    testdata.called_request = 0;
    sentry__stringbuilder_init(&testdata.serialized_envelope);

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_database_path(options, PREFIX ".test-db");
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(
        options, sentry_new_function_transport(send_envelope, &testdata));
    sentry_init(options);

    // force the disk transport so we flush the event to disk, and shutdown.
    // but free the function transport before so leak sanitizer does not
    // complain, as the enforce_disk_transport leaks intentionally ;-)
    sentry_transport_free(options->transport);
    sentry__enforce_disk_transport();
    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "Hello World!"));
    sentry_shutdown();

    // start up again, which should enqueue the event we flushed to disk
    options = sentry_options_new();
    sentry_options_set_database_path(options, PREFIX ".test-db");
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(
        options, sentry_new_function_transport(send_envelope, &testdata));
    sentry_init(options);

    TEST_CHECK_INT_EQUAL(testdata.called_envelope, 1);
    TEST_CHECK_INT_EQUAL(testdata.called_request, 1);
    char *serialized
        = sentry_stringbuilder_take_string(&testdata.serialized_envelope);
    TEST_CHECK(
        strstr(serialized,
            "{\"dsn\":\"https://foo@sentry.invalid/"
            "42\",\"event_id\":\"4c035723-8638-4c3a-923f-2ab9d08b4018\"}\n"
            "{\"type\":\"event\",")
        != NULL);

    sentry_free(serialized);

    sentry_shutdown();
    sentry__path_remove_all(path);
    sentry__path_free(path);
}
