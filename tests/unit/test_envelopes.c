#include "sentry_attachment.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_testsupport.h"
#include "sentry_utils.h"
#include "sentry_value.h"
#include "transports/sentry_http_transport.h"

static char *const SERIALIZED_ENVELOPE_STR
    = "{\"dsn\":\"https://foo@sentry.invalid/42\","
      "\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"trace\":{"
      "\"public_key\":\"foo\",\"org_id\":\"\",\"sample_rate\":0,\"sample_"
      "rand\":0.01006918276309107,\"release\":\"test-release\",\"environment\":"
      "\"production\",\"sampled\":\"false\"}}\n"
      "{\"type\":\"event\",\"length\":71}\n"
      "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"some-"
      "context\":1234}\n"
      "{\"type\":\"minidump\",\"length\":4}\n"
      "MDMP\n"
      "{\"type\":\"attachment\",\"length\":12}\n"
      "Hello World!";

SENTRY_TEST(basic_http_request_preparation_for_event)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry__envelope_add_event(envelope, event);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"event\",\"length\":51}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_transaction)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t transaction = sentry_value_new_object();
    sentry_value_set_by_key(
        transaction, "event_id", sentry__value_new_uuid(&event_id));
    sentry_value_set_by_key(
        transaction, "type", sentry_value_new_string("transaction"));
    sentry__envelope_add_transaction(envelope, transaction);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\","
        "\"sent_at\":"
        "\"2021-12-16T05:53:59.343Z\"}\n"
        "{\"type\":\"transaction\",\"length\":72}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"type\":"
        "\"transaction\"}");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_user_report)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t user_report;
    SENTRY_TEST_DEPRECATED(
        user_report = sentry_value_new_user_feedback(
            &event_id, "some-name", "some-email", "some-comment"));
    sentry__envelope_add_user_report(envelope, user_report);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"user_report\",\"length\":117}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"name\":"
        "\"some-name\",\"email\":\"some-email\",\"comments\":"
        "\"some-comment\"}");
#endif
    sentry__prepared_http_request_free(req);
    sentry_value_decref(user_report);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_user_feedback)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t user_feedback = sentry_value_new_feedback(
        "some-message", "some-email", "some-name", &event_id);
    sentry__envelope_add_user_feedback(envelope, user_feedback);

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    char *line1 = req->body;
    char *line1_end = strchr(line1, '\n');
    TEST_CHECK(line1_end != NULL);
    line1_end[0] = '\0';
    TEST_CHECK_STRING_EQUAL(
        line1, "{\"event_id\":\"4c035723-8638-4c3a-923f-2ab9d08b4018\"}");

    char *line2 = line1_end + 1;
    char *line2_end = strchr(line2, '\n');
    TEST_CHECK(line2_end != NULL);
    line2_end[0] = '\0';
    TEST_CHECK_STRING_EQUAL(line2, "{\"type\":\"feedback\",\"length\":269}");

    char *line3 = line2_end + 1;
    sentry_value_t line3_json = sentry__value_from_json(line3, strlen(line3));
    TEST_CHECK(!sentry_value_is_null(line3_json));

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(line3_json, "event_id")),
        "4c035723-8638-4c3a-923f-2ab9d08b4018");
    TEST_CHECK(!sentry_value_is_null(
        sentry_value_get_by_key(line3_json, "timestamp")));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(line3_json, "platform")),
        "native");

    sentry_value_t contexts = sentry_value_get_by_key(line3_json, "contexts");
    TEST_CHECK(!sentry_value_is_null(contexts));

    sentry_value_t actual = sentry_value_get_by_key(contexts, "feedback");
    TEST_CHECK(!sentry_value_is_null(actual));

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(actual, "message")),
        "some-message");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                actual, "contact_email")),
        "some-email");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(actual, "name")),
        "some-name");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                actual, "associated_event_id")),
        "c993afb6b4ac48a6b61b2558e601d65d");
    sentry_value_decref(line3_json);
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_event_with_attachment)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

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

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"event\",\"length\":51}\n"
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
        "{\"type\":\"attachment\",\"length\":12}\n"
        "Hello World!");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

SENTRY_TEST(basic_http_request_preparation_for_minidump)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    sentry_envelope_t *envelope = sentry__envelope_new();
    char dmp[] = "MDMP";
    sentry__envelope_add_from_buffer(
        envelope, dmp, sizeof(dmp) - 1, "minidump");
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(envelope, dsn, NULL, NULL);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/envelope/");
#ifndef SENTRY_TRANSPORT_COMPRESSION
    TEST_CHECK_STRING_EQUAL(req->body,
        "{}\n"
        "{\"type\":\"minidump\",\"length\":4}\n"
        "MDMP\n"
        "{\"type\":\"attachment\",\"length\":12}\n"
        "Hello World!");
#endif
    sentry__prepared_http_request_free(req);
    sentry_envelope_free(envelope);

    sentry__dsn_decref(dsn);
}

sentry_envelope_t *
create_test_envelope()
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_release(options, "test-release");
    sentry_init(options);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry_value_set_by_key(
        event, "some-context", sentry_value_new_int32(1234));
    sentry__envelope_add_event(envelope, event);

    char dmp[] = "MDMP";
    sentry__envelope_add_from_buffer(
        envelope, dmp, sizeof(dmp) - 1, "minidump");
    char msg[] = "Hello World!";
    sentry__envelope_add_from_buffer(
        envelope, msg, sizeof(msg) - 1, "attachment");
    return envelope;
}

SENTRY_TEST(serialize_envelope)
{
    sentry_envelope_t *envelope = create_test_envelope();

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    sentry__envelope_serialize_into_stringbuilder(envelope, &sb);
    char *str = sentry__stringbuilder_into_string(&sb);
    TEST_ASSERT(!!str);

    TEST_CHECK_STRING_EQUAL(str, SERIALIZED_ENVELOPE_STR);

    sentry_envelope_free(envelope);
    sentry_free(str);

    sentry_close();
}

SENTRY_TEST(basic_write_envelope_to_file)
{
    sentry_envelope_t *envelope = create_test_envelope();
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_envelope";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    int rv = sentry_envelope_write_to_file(envelope, test_file_str);
    TEST_CHECK_INT_EQUAL(rv, 0);
    TEST_ASSERT(sentry__path_is_file(test_file_path));

    size_t test_file_size;
    char *test_file_content
        = sentry__path_read_to_buffer(test_file_path, &test_file_size);
    TEST_CHECK_INT_EQUAL(test_file_size, strlen(SERIALIZED_ENVELOPE_STR));
    TEST_CHECK_STRING_EQUAL(test_file_content, SERIALIZED_ENVELOPE_STR);

    sentry_free(test_file_content);
    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(read_write_envelope_to_file_null)
{
    sentry_envelope_t *empty_envelope = sentry__envelope_new();

    TEST_CHECK(!sentry_envelope_read_from_file(NULL));
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(NULL, "irrelevant/path"), 1);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(empty_envelope, NULL), 1);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file_n(NULL, "irrelevant/path", 0), 1);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file_n(empty_envelope, NULL, 0), 1);

    sentry_envelope_free(empty_envelope);
}

SENTRY_TEST(read_write_envelope_to_invalid_path)
{
    sentry_envelope_t *envelope = create_test_envelope();
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX
        "./directory_that_does_not_exist/sentry_test_envelope";
    TEST_CHECK(!sentry_envelope_read_from_file(test_file_str));
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    int rv = sentry_envelope_write_to_file(envelope, test_file_str);
    TEST_CHECK_INT_EQUAL(rv, 1);

    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(write_raw_envelope_to_file)
{
    sentry_envelope_t *envelope = create_test_envelope();
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_envelope";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(envelope, test_file_str), 0);

    sentry_envelope_t *raw_envelope
        = sentry__envelope_from_path(test_file_path);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(raw_envelope, test_file_str), 0);

    size_t test_file_size;
    char *test_file_content
        = sentry__path_read_to_buffer(test_file_path, &test_file_size);
    TEST_CHECK_INT_EQUAL(test_file_size, strlen(SERIALIZED_ENVELOPE_STR));
    TEST_CHECK_STRING_EQUAL(test_file_content, SERIALIZED_ENVELOPE_STR);
    TEST_CHECK_INT_EQUAL(sentry__path_remove(test_file_path), 0);

    sentry_free(test_file_content);
    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_envelope_free(raw_envelope);
    sentry_close();
}

SENTRY_TEST(raw_envelope_event_id)
{
    sentry_envelope_t *envelope = create_test_envelope();
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_envelope";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(
        sentry_envelope_write_to_file(envelope, test_file_str), 0);

    sentry_envelope_t *raw_envelope
        = sentry__envelope_from_path(test_file_path);
    TEST_CHECK(!!raw_envelope);

    sentry_uuid_t event_id = sentry__envelope_get_event_id(raw_envelope);
    char event_id_str[37];
    sentry_uuid_as_string(&event_id, event_id_str);
    TEST_CHECK_STRING_EQUAL(
        event_id_str, "c993afb6-b4ac-48a6-b61b-2558e601d65d");

    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_envelope_free(raw_envelope);

    // missing event_id
    const char header_no_event_id[]
        = "{\"dsn\":\"https://foo@sentry.invalid/42\"}\n{}";
    test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(
        sentry__path_write_buffer(
            test_file_path, header_no_event_id, sizeof(header_no_event_id) - 1),
        0);
    raw_envelope = sentry__envelope_from_path(test_file_path);
    TEST_CHECK(!!raw_envelope);
    event_id = sentry__envelope_get_event_id(raw_envelope);
    TEST_CHECK(sentry_uuid_is_nil(&event_id));
    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_envelope_free(raw_envelope);

    // missing newline
    const char header_no_newline[]
        = "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}";
    test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(sentry__path_write_buffer(test_file_path,
                             header_no_newline, sizeof(header_no_newline) - 1),
        0);
    raw_envelope = sentry__envelope_from_path(test_file_path);
    TEST_CHECK(!!raw_envelope);
    event_id = sentry__envelope_get_event_id(raw_envelope);
    sentry_uuid_as_string(&event_id, event_id_str);
    TEST_CHECK_STRING_EQUAL(
        event_id_str, "c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_envelope_free(raw_envelope);

    sentry_close();
}

SENTRY_TEST(read_envelope_from_file)
{
    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_envelope";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(
        sentry__path_write_buffer(test_file_path, SERIALIZED_ENVELOPE_STR,
            strlen(SERIALIZED_ENVELOPE_STR)),
        0);

    sentry_envelope_t *envelope = sentry_envelope_read_from_file(test_file_str);
    TEST_CHECK(!!envelope);

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_envelope_get_header(envelope, "dsn")),
        "https://foo@sentry.invalid/42");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "c993afb6-b4ac-48a6-b61b-2558e601d65d");

    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    char event_id_str[37];
    sentry_uuid_as_string(&event_id, event_id_str);
    TEST_CHECK_STRING_EQUAL(
        event_id_str, "c993afb6-b4ac-48a6-b61b-2558e601d65d");

    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 3);

    sentry_value_t event = sentry_envelope_get_event(envelope);
    TEST_CHECK(!sentry_value_is_null(event));
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(event, "some-context")),
        1234);

    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!item);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "event");
    size_t ev_len = 0;
    TEST_CHECK_STRING_EQUAL(sentry__envelope_item_get_payload(item, &ev_len),
        "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\","
        "\"some-context\":1234}");

    const sentry_envelope_item_t *minidump
        = sentry__envelope_get_item(envelope, 1);
    TEST_CHECK(!!minidump);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(minidump, "type")),
        "minidump");
    size_t dmp_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(minidump, &dmp_len), "MDMP");
    TEST_CHECK_INT_EQUAL(dmp_len, 4);

    const sentry_envelope_item_t *attachment
        = sentry__envelope_get_item(envelope, 2);
    TEST_CHECK(!!attachment);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "type")),
        "attachment");
    size_t attachment_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(attachment, &attachment_len),
        "Hello World!");
    TEST_CHECK_INT_EQUAL(attachment_len, 12);

    sentry__path_free(test_file_path);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(deserialize_envelope)
{
    const char *buf
        = "{\"event_id\":\"9ec79c33ec9942ab8353589fcb2e04dc\",\"dsn\":\"https:/"
          "/e12d836b15bb49d7bbf99e64295d995b:@sentry.io/42\"}\n"
          "{\"type\":\"attachment\",\"length\":10,\"content_type\":\"text/"
          "plain\",\"filename\":\"hello.txt\"}\n"
          "\xef\xbb\xbfHello\r\n\n"
          "{\"type\":\"event\",\"length\":41,\"content_type\":\"application/"
          "json\",\"filename\":\"application.log\"}\n"
          "{\"message\":\"hello world\",\"level\":\"error\"}\n";

    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, strlen(buf));
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 2);

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_envelope_get_header(envelope, "dsn")),
        "https://e12d836b15bb49d7bbf99e64295d995b:@sentry.io/42");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "9ec79c33ec9942ab8353589fcb2e04dc");
    TEST_CHECK_JSON_VALUE(sentry_envelope_get_event(envelope),
        "{\"message\":\"hello world\",\"level\":\"error\"}");

    const sentry_envelope_item_t *attachment
        = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!attachment);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "type")),
        "attachment");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "filename")),
        "hello.txt");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "content_type")),
        "text/plain");
    size_t attachment_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(attachment, &attachment_len),
        "\xef\xbb\xbfHello\r\n");
    TEST_CHECK_INT_EQUAL(attachment_len, 10);

    sentry_envelope_free(envelope);
}

static void
test_deserialize_envelope_empty_attachments(const char *buf, size_t buf_len)
{
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 2);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "9ec79c33ec9942ab8353589fcb2e04dc");
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    for (int i = 0; i < 2; ++i) {
        const sentry_envelope_item_t *attachment
            = sentry__envelope_get_item(envelope, i);
        TEST_CHECK(!!attachment);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(
                sentry__envelope_item_get_header(attachment, "type")),
            "attachment");
        TEST_CHECK_INT_EQUAL(
            sentry_value_as_int32(
                sentry__envelope_item_get_header(attachment, "length")),
            0);
        size_t attachment_len = 0;
        TEST_CHECK(
            !sentry__envelope_item_get_payload(attachment, &attachment_len));
        TEST_CHECK_INT_EQUAL(attachment_len, 0);
    }

    sentry_envelope_free(envelope);
}

SENTRY_TEST(deserialize_envelope_empty_attachments)
{
    const char *buf = "{\"event_id\":\"9ec79c33ec9942ab8353589fcb2e04dc\"}\n"
                      "{\"type\":\"attachment\",\"length\":0}\n"
                      "\n"
                      "{\"type\":\"attachment\",\"length\":0}\n"
                      "\n";
    size_t buf_len = strlen(buf);

    // trailing newline
    test_deserialize_envelope_empty_attachments(buf, buf_len);

    // eof
    test_deserialize_envelope_empty_attachments(buf, buf_len - 1);
}

static void
test_deserialize_envelope_implicit_length(const char *buf, size_t buf_len)
{
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_envelope_get_header(
                                envelope, "event_id")),
        "9ec79c33ec9942ab8353589fcb2e04dc");
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    const sentry_envelope_item_t *attachment
        = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!attachment);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(
            sentry__envelope_item_get_header(attachment, "type")),
        "attachment");
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(sentry__envelope_item_get_header(
                             attachment, "length")),
        0);
    size_t attachment1_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(attachment, &attachment1_len),
        "helloworld");
    TEST_CHECK_INT_EQUAL(attachment1_len, 10);

    sentry_envelope_free(envelope);
}

SENTRY_TEST(deserialize_envelope_implicit_length)
{
    const char *buf = "{\"event_id\":\"9ec79c33ec9942ab8353589fcb2e04dc\"}\n"
                      "{\"type\":\"attachment\"}\n"
                      "helloworld\n";
    size_t buf_len = strlen(buf);

    // trailing newline
    test_deserialize_envelope_implicit_length(buf, buf_len);

    // eof
    test_deserialize_envelope_implicit_length(buf, buf_len - 1);
}

SENTRY_TEST(deserialize_envelope_no_headers)
{
    const char *session = "{\"started\": "
                          "\"2020-02-07T14:16:00Z\",\"attrs\":{\"release\":"
                          "\"sentry-test@1.0.0\"}"
                          "}";
    char buf[512];
    snprintf(buf, sizeof(buf),
        "{}\n"
        "{\"type\":\"session\"}\n"
        "%s",
        session);

    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, strlen(buf));
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "event_id")));
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK(!!item);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "session");
    TEST_CHECK(
        sentry_value_is_null(sentry__envelope_item_get_header(item, "length")));
    size_t session_len = 0;
    TEST_CHECK_STRING_EQUAL(
        sentry__envelope_item_get_payload(item, &session_len), session);
    TEST_CHECK_INT_EQUAL(session_len, strlen(session));

    sentry_envelope_free(envelope);
}

static void
test_deserialize_envelope_empty(const char *buf, size_t buf_len)
{
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    TEST_CHECK(!!envelope);
    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 0);

    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "dsn")));
    TEST_CHECK(
        sentry_value_is_null(sentry_envelope_get_header(envelope, "event_id")));
    TEST_CHECK(sentry_value_is_null(sentry_envelope_get_event(envelope)));

    sentry_envelope_free(envelope);
}

SENTRY_TEST(deserialize_envelope_empty)
{
    const char *buf = "{}\n";
    size_t buf_len = strlen(buf);

    // trailing newline
    test_deserialize_envelope_empty(buf, buf_len);

    // eof
    test_deserialize_envelope_empty(buf, buf_len - 1);
}

SENTRY_TEST(tus_upload_url)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    char *url = sentry__dsn_get_upload_url(dsn);
    TEST_CHECK_STRING_EQUAL(url, "https://sentry.invalid:443/api/42/upload/");
    sentry_free(url);
    sentry__dsn_decref(dsn);

    TEST_CHECK(!sentry__dsn_get_upload_url(NULL));
}

SENTRY_TEST(tus_request_preparation)
{
    SENTRY_TEST_DSN_NEW_DEFAULT(dsn);

    const char *test_file_str = SENTRY_TEST_PATH_PREFIX "sentry_test_tus_file";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    TEST_CHECK_INT_EQUAL(
        sentry__path_write_buffer(test_file_path, "test-data", 9), 0);

    sentry_prepared_http_request_t *req
        = sentry__prepare_tus_request(test_file_path, 9, dsn, NULL);
    TEST_CHECK(!!req);
    TEST_CHECK_STRING_EQUAL(req->method, "POST");
    TEST_CHECK_STRING_EQUAL(
        req->url, "https://sentry.invalid:443/api/42/upload/");
    TEST_CHECK(!!req->body_path);
    TEST_CHECK_INT_EQUAL(req->body_len, 9);
    TEST_CHECK(!req->body);

    bool has_tus_resumable = false;
    bool has_upload_length = false;
    bool has_content_type = false;
    for (size_t i = 0; i < req->headers_len; i++) {
        if (strcmp(req->headers[i].key, "tus-resumable") == 0) {
            TEST_CHECK_STRING_EQUAL(req->headers[i].value, "1.0.0");
            has_tus_resumable = true;
        }
        if (strcmp(req->headers[i].key, "upload-length") == 0) {
            TEST_CHECK_STRING_EQUAL(req->headers[i].value, "9");
            has_upload_length = true;
        }
        if (strcmp(req->headers[i].key, "content-type") == 0) {
            TEST_CHECK_STRING_EQUAL(
                req->headers[i].value, "application/offset+octet-stream");
            has_content_type = true;
        }
    }
    TEST_CHECK(has_tus_resumable);
    TEST_CHECK(has_upload_length);
    TEST_CHECK(has_content_type);

    sentry__prepared_http_request_free(req);
    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry__dsn_decref(dsn);
}

SENTRY_TEST(attachment_ref_creation)
{
    const char *test_file_str
        = SENTRY_TEST_PATH_PREFIX "sentry_test_attachment_ref";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);

    // Small file: should be added to envelope
    {
        sentry_envelope_t *envelope = sentry__envelope_new();
        char small_data[] = "small";
        TEST_CHECK_INT_EQUAL(sentry__path_write_buffer(test_file_path,
                                 small_data, sizeof(small_data) - 1),
            0);

        sentry_attachment_t *attachment
            = sentry__attachment_from_path(sentry__path_clone(test_file_path));
        sentry_envelope_item_t *item
            = sentry__envelope_add_attachment(envelope, attachment);

        TEST_CHECK(!!item);
        TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);
        size_t payload_len = 0;
        TEST_CHECK_STRING_EQUAL(
            sentry__envelope_item_get_payload(item, &payload_len), "small");

        sentry__attachment_free(attachment);
        sentry_envelope_free(envelope);
    }

    // Large file (>= threshold): should be skipped by envelope
    {
        sentry_envelope_t *envelope = sentry__envelope_new();
        size_t large_size = 100 * 1024 * 1024;
        FILE *f = fopen(test_file_str, "wb");
        TEST_CHECK(!!f);
        fseek(f, (long)(large_size - 1), SEEK_SET);
        fputc(0, f);
        fclose(f);

        sentry_attachment_t *attachment
            = sentry__attachment_from_path(sentry__path_clone(test_file_path));
        sentry_envelope_item_t *item
            = sentry__envelope_add_attachment(envelope, attachment);

        TEST_CHECK(!item);
        TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 0);

        sentry__attachment_free(attachment);
        sentry_envelope_free(envelope);
    }

    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
}

SENTRY_TEST(attachment_ref_from_path)
{
    const char *test_file_str
        = SENTRY_TEST_PATH_PREFIX "sentry_test_attachment_ref_from_path";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);

    // Small file: should be added to envelope
    {
        sentry_envelope_t *envelope = sentry__envelope_new();
        char small_data[] = "small";
        TEST_CHECK_INT_EQUAL(sentry__path_write_buffer(test_file_path,
                                 small_data, sizeof(small_data) - 1),
            0);

        sentry_envelope_item_t *item = sentry__envelope_add_from_path(
            envelope, test_file_path, "attachment");

        TEST_CHECK(!!item);
        TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);
        size_t payload_len = 0;
        TEST_CHECK_STRING_EQUAL(
            sentry__envelope_item_get_payload(item, &payload_len), "small");

        sentry_envelope_free(envelope);
    }

    // Large file (>= threshold): should return NULL
    {
        sentry_envelope_t *envelope = sentry__envelope_new();
        size_t large_size = 100 * 1024 * 1024;
        FILE *f = fopen(test_file_str, "wb");
        TEST_CHECK(!!f);
        fseek(f, (long)(large_size - 1), SEEK_SET);
        fputc(0, f);
        fclose(f);

        sentry_envelope_item_t *item = sentry__envelope_add_from_path(
            envelope, test_file_path, "attachment");

        TEST_CHECK(!item);
        TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 0);

        sentry_envelope_free(envelope);
    }

    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
}

SENTRY_TEST(attachment_ref_copy)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_init(options);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");

    const char *test_file_str
        = SENTRY_TEST_PATH_PREFIX "sentry_test_large_attachment";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);
    size_t large_size = 100 * 1024 * 1024;
    FILE *f = fopen(test_file_str, "wb");
    TEST_CHECK(!!f);
    fseek(f, (long)(large_size - 1), SEEK_SET);
    fputc(0, f);
    fclose(f);

    sentry_attachment_t *attachment
        = sentry__attachment_from_path(sentry__path_clone(test_file_path));
    sentry_attachment_set_content_type(attachment, "application/x-dmp");

    // cache_large_attachments copies the file (not under run_path)
    sentry_path_t *db_path = NULL;
    SENTRY_WITH_OPTIONS (opts) {
        db_path = sentry__path_clone(opts->database_path);
        sentry__cache_large_attachments(
            opts->run->cache_path, &event_id, attachment, NULL);
    }

    // original file still exists (copied, not moved)
    TEST_CHECK(sentry__path_is_file(test_file_path));

    // attachment file exists in cache dir
    sentry_path_t *cache_dir = sentry__path_join_str(db_path, "cache");
    sentry_path_t *event_dir = sentry__path_join_str(
        cache_dir, "c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry__path_free(cache_dir);
    sentry_path_t *att_file
        = sentry__path_join_str(event_dir, "sentry_test_large_attachment");
    TEST_CHECK(sentry__path_is_file(att_file));

    // __sentry-attachments.json exists with correct metadata
    sentry_path_t *refs_path
        = sentry__path_join_str(event_dir, "__sentry-attachments.json");
    TEST_CHECK(sentry__path_is_file(refs_path));

    size_t refs_len = 0;
    char *refs_buf = sentry__path_read_to_buffer(refs_path, &refs_len);
    TEST_CHECK(!!refs_buf);
    if (refs_buf) {
        sentry_value_t refs = sentry__value_from_json(refs_buf, refs_len);
        sentry_free(refs_buf);
        TEST_CHECK(sentry_value_get_type(refs) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK(sentry_value_get_length(refs) == 1);
        sentry_value_t ref_entry = sentry_value_get_by_index(refs, 0);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    ref_entry, "filename")),
            "sentry_test_large_attachment");
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    ref_entry, "content_type")),
            "application/x-dmp");
        sentry_value_decref(refs);
    }

    sentry__path_free(refs_path);
    sentry__path_free(att_file);
    sentry__path_free(event_dir);
    sentry__path_free(db_path);
    sentry__attachment_free(attachment);
    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
    sentry_close();
}

SENTRY_TEST(attachment_ref_move)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_init(options);

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");

    // create large file inside the run directory (SDK-owned)
    sentry_path_t *run_path = NULL;
    sentry_path_t *db_path = NULL;
    SENTRY_WITH_OPTIONS (opts) {
        run_path = sentry__path_clone(opts->run->run_path);
        db_path = sentry__path_clone(opts->database_path);
    }
    TEST_CHECK(!!run_path);
    sentry_path_t *src_path
        = sentry__path_join_str(run_path, "test_minidump.dmp");

    size_t large_size = 100 * 1024 * 1024;
#ifdef SENTRY_PLATFORM_WINDOWS
    FILE *f = _wfopen(src_path->path_w, L"wb");
#else
    FILE *f = fopen(src_path->path, "wb");
#endif
    TEST_CHECK(!!f);
    fseek(f, (long)(large_size - 1), SEEK_SET);
    fputc(0, f);
    fclose(f);

    sentry_attachment_t *attachment
        = sentry__attachment_from_path(sentry__path_clone(src_path));

    // cache with run_path → file is renamed (moved)
    SENTRY_WITH_OPTIONS (opts) {
        sentry__cache_large_attachments(
            opts->run->cache_path, &event_id, attachment, run_path);
    }

    // run-dir file is gone (renamed)
    TEST_CHECK(!sentry__path_is_file(src_path));

    // attachment file moved to cache dir
    sentry_path_t *cache_dir = sentry__path_join_str(db_path, "cache");
    sentry_path_t *event_dir = sentry__path_join_str(
        cache_dir, "c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry__path_free(cache_dir);
    sentry_path_t *att_file
        = sentry__path_join_str(event_dir, "test_minidump.dmp");
    TEST_CHECK(sentry__path_is_file(att_file));

    // __sentry-attachments.json exists
    sentry_path_t *refs_path
        = sentry__path_join_str(event_dir, "__sentry-attachments.json");
    TEST_CHECK(sentry__path_is_file(refs_path));

    sentry__path_free(refs_path);
    sentry__path_free(att_file);
    sentry__path_free(event_dir);
    sentry__path_free(db_path);
    sentry__attachment_free(attachment);
    sentry__path_free(src_path);
    sentry__path_free(run_path);
    sentry_close();
}

static void
send_restore_envelope(sentry_envelope_t *envelope, void *data)
{
    int *called = data;
    *called += 1;
    sentry_envelope_free(envelope);
}

SENTRY_TEST(attachment_ref_restore)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    const char *db_str = SENTRY_TEST_PATH_PREFIX ".sentry-native";

    sentry_uuid_t event_id
        = sentry_uuid_from_string("c993afb6-b4ac-48a6-b61b-2558e601d65d");

    // set up old run dir with a stripped envelope (no attachment-ref items)
    sentry_path_t *db_path = sentry__path_from_str(db_str);
    sentry_path_t *old_run_path = sentry__path_join_str(db_path, "old.run");
    TEST_ASSERT(sentry__path_create_dir_all(old_run_path) == 0);

    // build attachment + __sentry-attachments.json in db/cache/<uuid>/
    sentry_path_t *cache_dir = sentry__path_join_str(db_path, "cache");
    sentry_path_t *event_att_dir = sentry__path_join_str(
        cache_dir, "c993afb6-b4ac-48a6-b61b-2558e601d65d");
    sentry__path_free(cache_dir);
    TEST_ASSERT(sentry__path_create_dir_all(event_att_dir) == 0);

    sentry_path_t *att_file
        = sentry__path_join_str(event_att_dir, "test_minidump.dmp");
    size_t large_size = 100 * 1024 * 1024;
    FILE *f = fopen(att_file->path, "wb");
    TEST_ASSERT(!!f);
    fseek(f, (long)(large_size - 1), SEEK_SET);
    fputc(0, f);
    fclose(f);

    // write __sentry-attachments.json
    sentry_value_t refs = sentry_value_new_list();
    sentry_value_t ref_obj = sentry_value_new_object();
    sentry_value_set_by_key(
        ref_obj, "filename", sentry_value_new_string("test_minidump.dmp"));
    sentry_value_set_by_key(
        ref_obj, "attachment_type", sentry_value_new_string("event.minidump"));
    sentry_value_set_by_key(ref_obj, "attachment_length",
        sentry_value_new_uint64((uint64_t)large_size));
    sentry_value_append(refs, ref_obj);
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    sentry__jsonwriter_write_value(jw, refs);
    size_t refs_buf_len = 0;
    char *refs_buf = sentry__jsonwriter_into_string(jw, &refs_buf_len);
    sentry_value_decref(refs);
    sentry_path_t *refs_path
        = sentry__path_join_str(event_att_dir, "__sentry-attachments.json");
    TEST_ASSERT(
        sentry__path_write_buffer(refs_path, refs_buf, refs_buf_len) == 0);
    sentry_free(refs_buf);

    // build envelope with only the event (attachment-refs stripped at persist)
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "event_id", sentry__value_new_uuid(&event_id));
    sentry__envelope_add_event(envelope, event);
    TEST_ASSERT(sentry__envelope_get_item_count(envelope) == 1);

    // write stripped envelope to old run dir
    sentry_path_t *envelope_path = sentry__path_join_str(
        old_run_path, "c993afb6-b4ac-48a6-b61b-2558e601d65d.envelope");
    TEST_ASSERT(sentry_envelope_write_to_path(envelope, envelope_path) == 0);
    sentry_envelope_free(envelope);

    // init sentry with function transport — process_old_runs runs during init
    int called = 0;
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport = sentry_transport_new(send_restore_envelope);
    sentry_transport_set_state(transport, &called);
    sentry_options_set_transport(options, transport);
    sentry_init(options);

    // transport callback should have been called with the raw envelope
    TEST_CHECK(called >= 1);

    // attachment + __sentry-attachments.json remain in cache for transport to
    // resolve via TUS
    TEST_CHECK(sentry__path_is_file(att_file));
    TEST_CHECK(sentry__path_is_file(refs_path));

    sentry__path_free(envelope_path);
    sentry__path_free(refs_path);
    sentry__path_free(att_file);
    sentry__path_free(event_att_dir);
    sentry__path_free(old_run_path);
    sentry__path_free(db_path);
    sentry_close();
}

SENTRY_TEST(deserialize_envelope_invalid)
{
    TEST_CHECK(!sentry_envelope_deserialize("", 0));
    TEST_CHECK(!sentry_envelope_deserialize("{}", 0));
    TEST_CHECK(!sentry_envelope_deserialize("\n", 1));
    TEST_CHECK(!sentry_envelope_deserialize("{}\n{}", 5));
    TEST_CHECK(!sentry_envelope_deserialize("{}\ninvalid\n", 11));
    TEST_CHECK(!sentry_envelope_deserialize("invalid", 7));
    TEST_CHECK(!sentry_envelope_deserialize("{}\n{\"length\":-1}\n", 17));
    char buf[128];
    snprintf(buf, sizeof(buf), "{}\n{\"length\":%zu}\n", SIZE_MAX);
    TEST_CHECK(!sentry_envelope_deserialize(buf, strlen(buf)));
}

static bool
tus_mock_send(void *client, sentry_prepared_http_request_t *req,
    sentry_http_response_t *resp)
{
    (void)client;
    (void)req;
    resp->status_code = 201;
    resp->location = sentry__string_clone("https://sentry.invalid/upload/abc");
    return true;
}

SENTRY_TEST(tus_file_attachment_preserves_original)
{
    const char *test_file_str
        = SENTRY_TEST_PATH_PREFIX "sentry_test_tus_preserve";
    sentry_path_t *test_file_path = sentry__path_from_str(test_file_str);

    size_t large_size = 100 * 1024 * 1024;
    FILE *f = fopen(test_file_str, "wb");
    TEST_CHECK(!!f);
    fseek(f, (long)(large_size - 1), SEEK_SET);
    fputc(0, f);
    fclose(f);

    sentry_transport_t *transport
        = sentry__http_transport_new(NULL, tus_mock_send);
    TEST_CHECK(!!transport);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(options, transport);
    sentry_options_add_attachment(options, test_file_str);
    sentry_init(options);

    sentry_capture_event(
        sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "test"));

    sentry_close();

    TEST_CHECK(sentry__path_is_file(test_file_path));

    sentry__path_remove(test_file_path);
    sentry__path_free(test_file_path);
}
