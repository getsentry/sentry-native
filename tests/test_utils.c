#include "../src/sentry_utils.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(url_parsing_complete)
{
    sentry__url_t url;
    assert_int_equal(sentry__url_parse(&url,
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
    sentry__url_cleanup(&url);
}

SENTRY_TEST(url_parsing_partial)
{
    sentry__url_t url;
    assert_int_equal(
        sentry__url_parse(&url, "http://username:password@example.com/foo/bar"),
        0);
    assert_string_equal(url.scheme, "http");
    assert_string_equal(url.host, "example.com");
    assert_int_equal(url.port, 80);
    assert_string_equal(url.username, "username");
    assert_string_equal(url.password, "password");
    assert_string_equal(url.path, "/foo/bar");
    assert_true(url.query == NULL);
    assert_true(url.fragment == NULL);
    sentry__url_cleanup(&url);
}

SENTRY_TEST(url_parsing_invalid)
{
    sentry__url_t url;
    assert_int_equal(sentry__url_parse(&url, "http:"), 1);
}

SENTRY_TEST(dsn_parsing_complete)
{
    sentry__dsn_t dsn;
    assert_int_equal(
        sentry__dsn_parse(
            &dsn, "http://username:password@example.com/foo/bar/42?x=y#z"),
        0);
    assert_false(dsn.is_secure);
    assert_string_equal(dsn.host, "example.com");
    assert_int_equal(dsn.port, 80);
    assert_string_equal(dsn.public_key, "username");
    assert_string_equal(dsn.secret_key, "password");
    assert_string_equal(dsn.path, "/foo/bar");
    assert_int_equal((int)dsn.project_id, 42);
    sentry__dsn_cleanup(&dsn);
}

SENTRY_TEST(dsn_parsing_invalid)
{
    sentry__dsn_t dsn;
    assert_int_equal(sentry__dsn_parse(&dsn,
                         "http://username:password@example.com/foo/bar?x=y#z"),
        1);
}