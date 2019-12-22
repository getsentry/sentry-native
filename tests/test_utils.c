#include "../src/sentry_utils.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(url_parsing_complete)
{
    sentry_url_t url;
    assert_int_equal(sentry_url_parse(&url,
                         "http://username:password@example.com/foo/bar?x=y#z"),
        0);
    assert_string_equal(url.scheme, "http");
    assert_string_equal(url.host, "example.com");
    assert_int_equal(url.port, 80);
    assert_string_equal(url.username, "username");
    assert_string_equal(url.password, "password");
    assert_string_equal(url.path, "/foo/bar");
    assert_string_equal(url.query, "x=y");
    assert_string_equal(url.fragment, "z");
    sentry_url_cleanup(&url);
}

SENTRY_TEST(url_parsing_partial)
{
    sentry_url_t url;
    assert_int_equal(
        sentry_url_parse(&url, "http://username:password@example.com/foo/bar"),
        0);
    assert_string_equal(url.scheme, "http");
    assert_string_equal(url.host, "example.com");
    assert_int_equal(url.port, 80);
    assert_string_equal(url.username, "username");
    assert_string_equal(url.password, "password");
    assert_string_equal(url.path, "/foo/bar");
    assert_true(url.query == NULL);
    assert_true(url.fragment == NULL);
    sentry_url_cleanup(&url);
}

SENTRY_TEST(url_parsing_invalid)
{
    sentry_url_t url;
    assert_int_equal(sentry_url_parse(&url, "http:"), 1);
}