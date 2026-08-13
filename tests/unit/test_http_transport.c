#include "sentry_alloc.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_testsupport.h"
#include "transports/sentry_http_transport.h"

#include <string.h>
#ifdef SENTRY_PLATFORM_WINDOWS
#    include <wchar.h>
#endif

SENTRY_TEST(http_response_set_header_sets_known_headers)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    sentry_http_response_set_header(&resp, "retry-after", "60");
    sentry_http_response_set_header(
        &resp, "x-sentry-rate-limits", "60:error:key");
    sentry_http_response_set_header(&resp, "location", "/uploads/1");

    TEST_CHECK_STRING_EQUAL(resp.retry_after, "60");
    TEST_CHECK_STRING_EQUAL(resp.x_sentry_rate_limits, "60:error:key");
    TEST_CHECK_STRING_EQUAL(resp.location, "/uploads/1");

    sentry_free(resp.retry_after);
    sentry_free(resp.x_sentry_rate_limits);
    sentry_free(resp.location);
}

SENTRY_TEST(http_response_set_header_is_case_insensitive)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    sentry_http_response_set_header(&resp, "Retry-After", "30");
    sentry_http_response_set_header(
        &resp, "X-SENTRY-RATE-LIMITS", "30:error:key");
    sentry_http_response_set_header(&resp, "Location", "/uploads/2");

    TEST_CHECK_STRING_EQUAL(resp.retry_after, "30");
    TEST_CHECK_STRING_EQUAL(resp.x_sentry_rate_limits, "30:error:key");
    TEST_CHECK_STRING_EQUAL(resp.location, "/uploads/2");

    sentry_free(resp.retry_after);
    sentry_free(resp.x_sentry_rate_limits);
    sentry_free(resp.location);
}

SENTRY_TEST(http_response_set_header_ignores_unknown_headers)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    sentry_http_response_set_header(&resp, "content-type", "text/plain");
    sentry_http_response_set_header(&resp, "x-request-id", "abc123");

    TEST_CHECK(resp.retry_after == NULL);
    TEST_CHECK(resp.x_sentry_rate_limits == NULL);
    TEST_CHECK(resp.location == NULL);
}

SENTRY_TEST(http_response_set_header_overwrites_previous_value)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    sentry_http_response_set_header(&resp, "retry-after", "10");
    sentry_http_response_set_header(&resp, "retry-after", "20");

    TEST_CHECK_STRING_EQUAL(resp.retry_after, "20");

    sentry_free(resp.retry_after);
}

SENTRY_TEST(http_response_set_header_null_safety)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    // Must not crash on a NULL response or a NULL key.
    sentry_http_response_set_header(NULL, "retry-after", "10");
    sentry_http_response_set_header(&resp, NULL, "10");

    TEST_CHECK(resp.retry_after == NULL);
}

SENTRY_TEST(http_response_set_status_code)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    sentry_http_response_set_status_code(&resp, 429);
    TEST_CHECK_INT_EQUAL(resp.status_code, 429);

    // Must not crash on a NULL response.
    sentry_http_response_set_status_code(NULL, 500);
}

static sentry_prepared_http_request_t *
make_test_request(void)
{
    sentry_prepared_http_request_t *req
        = SENTRY_MAKE(sentry_prepared_http_request_t);
    memset(req, 0, sizeof(*req));
    req->method = "POST";
    req->url = sentry__string_clone("https://sentry.invalid/api/42/envelope/");
    req->headers_len = 2;
    req->headers = sentry_malloc(
        sizeof(sentry_prepared_http_header_t) * req->headers_len);
    req->headers[0].key = "content-type";
    req->headers[0].value
        = sentry__string_clone("application/x-sentry-envelope");
    req->headers[1].key = "content-length";
    req->headers[1].value = sentry__string_clone("11");
    req->body = sentry__string_clone("hello world");
    req->body_len = 11;
    req->body_owned = true;
    return req;
}

SENTRY_TEST(http_request_accessors_in_memory_body)
{
    sentry_prepared_http_request_t *req = make_test_request();

    TEST_CHECK_STRING_EQUAL(sentry_http_request_get_method(req), "POST");
    TEST_CHECK_STRING_EQUAL(sentry_http_request_get_url(req),
        "https://sentry.invalid/api/42/envelope/");
    TEST_CHECK_INT_EQUAL((int)sentry_http_request_get_header_count(req), 2);

    const char *key = NULL;
    const char *value = NULL;
    TEST_CHECK(sentry_http_request_get_header(req, 0, &key, &value) == 1);
    TEST_CHECK_STRING_EQUAL(key, "content-type");
    TEST_CHECK_STRING_EQUAL(value, "application/x-sentry-envelope");

    TEST_CHECK(sentry_http_request_get_header(req, 1, &key, &value) == 1);
    TEST_CHECK_STRING_EQUAL(key, "content-length");
    TEST_CHECK_STRING_EQUAL(value, "11");

    // Out of range must fail rather than reading past the array.
    TEST_CHECK(sentry_http_request_get_header(req, 2, &key, &value) == 0);

    size_t len = 0;
    const char *body = sentry_http_request_get_body(req, &len);
    TEST_CHECK(body != NULL);
    TEST_CHECK_INT_EQUAL((int)len, 11);
    TEST_CHECK(memcmp(body, "hello world", 11) == 0);

    // A request with an in-memory body has no file-backed body.
    size_t file_len = 123;
    TEST_CHECK(sentry_http_request_get_body_file_path(req, &file_len) == NULL);
    TEST_CHECK_INT_EQUAL((int)file_len, 0);

    sentry__prepared_http_request_free(req);
}

SENTRY_TEST(http_request_accessors_file_backed_body)
{
    sentry_prepared_http_request_t *req
        = SENTRY_MAKE(sentry_prepared_http_request_t);
    memset(req, 0, sizeof(*req));
    req->method = "PATCH";
    req->url = sentry__string_clone("https://sentry.invalid/upload/abc");
    req->headers_len = 0;
    req->body_path = sentry__path_from_str("/tmp/does-not-need-to-exist");
    req->body_len = 100 * 1024 * 1024;

    size_t len = 0;
    TEST_CHECK(sentry_http_request_get_body(req, &len) == NULL);
    TEST_CHECK_INT_EQUAL((int)len, 0);

    len = 0;
    const char *path = sentry_http_request_get_body_file_path(req, &len);
    TEST_CHECK_STRING_EQUAL(path, "/tmp/does-not-need-to-exist");
    TEST_CHECK_INT_EQUAL((int)len, 100 * 1024 * 1024);

#ifdef SENTRY_PLATFORM_WINDOWS
    // The wide variant must stay in sync with the narrow one, since that is
    // what a Windows client needs to hand to the `W` Win32 file APIs.
    len = 0;
    const wchar_t *path_w = sentry_http_request_get_body_file_pathw(req, &len);
    TEST_CHECK(path_w != NULL);
    TEST_CHECK(wcscmp(path_w, L"/tmp/does-not-need-to-exist") == 0);
    TEST_CHECK_INT_EQUAL((int)len, 100 * 1024 * 1024);

    len = 123;
    TEST_CHECK(sentry_http_request_get_body_file_pathw(NULL, &len) == NULL);
    TEST_CHECK_INT_EQUAL((int)len, 0);
#endif

    sentry__prepared_http_request_free(req);
}

SENTRY_TEST(http_request_accessors_bodyless_request)
{
    // Mirrors the TUS creation `POST`, which has neither an in-memory nor a
    // file-backed body.
    sentry_prepared_http_request_t *req
        = SENTRY_MAKE(sentry_prepared_http_request_t);
    memset(req, 0, sizeof(*req));
    req->method = "POST";
    req->url = sentry__string_clone("https://sentry.invalid/upload/");
    req->headers_len = 0;

    size_t len = 123;
    TEST_CHECK(sentry_http_request_get_body(req, &len) == NULL);
    TEST_CHECK_INT_EQUAL((int)len, 0);

    len = 123;
    TEST_CHECK(sentry_http_request_get_body_file_path(req, &len) == NULL);
    TEST_CHECK_INT_EQUAL((int)len, 0);

    sentry__prepared_http_request_free(req);
}

SENTRY_TEST(http_request_accessors_null_safety)
{
    TEST_CHECK(sentry_http_request_get_method(NULL) == NULL);
    TEST_CHECK(sentry_http_request_get_url(NULL) == NULL);
    TEST_CHECK_INT_EQUAL((int)sentry_http_request_get_header_count(NULL), 0);
    TEST_CHECK(sentry_http_request_get_header(NULL, 0, NULL, NULL) == 0);
    size_t len = 0;
    TEST_CHECK(sentry_http_request_get_body(NULL, &len) == NULL);
    TEST_CHECK(sentry_http_request_get_body_file_path(NULL, &len) == NULL);
}

static void *
counting_factory(void *factory_data)
{
    int *call_count = factory_data;
    (*call_count)++;
    return sentry_malloc(sizeof(int));
}

static void *
failing_factory(void *factory_data)
{
    (void)factory_data;
    return NULL;
}

// Returns a non-`NULL` client without heap-allocating, for tests that
// exercise a code path with no client-free hook and must not leak.
static void *
non_owning_factory(void *factory_data)
{
    (void)factory_data;
    return (void *)1;
}

static int
noop_send_func(
    void *client, sentry_http_request_t *req, sentry_http_response_t *resp)
{
    (void)client;
    (void)req;
    (void)resp;
    return 1;
}

static bool g_client_freed = false;

static void
counting_client_free(void *client)
{
    g_client_freed = true;
    sentry_free(client);
}

SENTRY_TEST(http_transport_new_creates_transport_via_factory)
{
    int call_count = 0;
    g_client_freed = false;

    sentry_transport_t *transport = sentry_http_transport_new(
        counting_factory, &call_count, noop_send_func, counting_client_free);

    TEST_CHECK(!!transport);
    TEST_CHECK_INT_EQUAL(call_count, 1);
    TEST_CHECK(!g_client_freed);

    sentry_transport_free(transport);
    TEST_CHECK(g_client_freed);
}

SENTRY_TEST(http_transport_new_null_factory_fails)
{
    int call_count = 0;
    sentry_transport_t *transport = sentry_http_transport_new(
        NULL, &call_count, noop_send_func, counting_client_free);
    TEST_CHECK(!transport);
}

SENTRY_TEST(http_transport_new_null_send_func_fails)
{
    int call_count = 0;
    sentry_transport_t *transport = sentry_http_transport_new(
        counting_factory, &call_count, NULL, counting_client_free);
    TEST_CHECK(!transport);
    // The factory must not have been called if `send_func` was already
    // known to be invalid.
    TEST_CHECK_INT_EQUAL(call_count, 0);
}

SENTRY_TEST(http_transport_new_factory_failure_returns_null)
{
    sentry_transport_t *transport = sentry_http_transport_new(
        failing_factory, NULL, noop_send_func, counting_client_free);
    TEST_CHECK(!transport);
}

SENTRY_TEST(http_transport_new_null_client_free_func_is_safe)
{
    sentry_transport_t *transport = sentry_http_transport_new(
        non_owning_factory, NULL, noop_send_func, NULL);
    TEST_CHECK(!!transport);
    // Must not crash freeing a transport with no client-free hook.
    sentry_transport_free(transport);
}
