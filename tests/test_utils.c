#include "../src/sentry_utils.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(url_parsing_complete)
{
    sentry_url_t url;
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
    sentry_url_t url;
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
    sentry_url_t url;
    assert_int_equal(sentry__url_parse(&url, "http:"), 1);
}

SENTRY_TEST(dsn_parsing_complete)
{
    sentry_dsn_t dsn;
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
    sentry_dsn_t dsn;
    assert_int_equal(sentry__dsn_parse(&dsn,
                         "http://username:password@example.com/foo/bar?x=y#z"),
        1);
}

SENTRY_TEST(dsn_store_url_with_path)
{
    sentry_uuid_t uuid
        = sentry_uuid_from_string("4c7e771a-f17d-4220-bc8f-5b1edcdb5faa");
    sentry_dsn_t dsn;
    assert_int_equal(
        sentry__dsn_parse(
            &dsn, "http://username:password@example.com/foo/bar/42?x=y#z"),
        0);
    char *url = sentry__dsn_get_store_url(&dsn);
    assert_string_equal(url, "http://example.com:80/foo/bar/api/42/store/");
    sentry_free(url);
    url = sentry__dsn_get_minidump_url(&dsn);
    assert_string_equal(url,
        "http://example.com:80/foo/bar/api/42/minidump/?sentry_key=username");
    sentry_free(url);
    url = sentry__dsn_get_attachment_url(&dsn, &uuid);
    assert_string_equal(url,
        "http://example.com:80/foo/bar/api/42/events/"
        "4c7e771a-f17d-4220-bc8f-5b1edcdb5faa/attachments/");
    sentry_free(url);
    sentry__dsn_cleanup(&dsn);
}

SENTRY_TEST(dsn_store_url_without_path)
{
    sentry_uuid_t uuid
        = sentry_uuid_from_string("4c7e771a-f17d-4220-bc8f-5b1edcdb5faa");
    sentry_dsn_t dsn;
    assert_int_equal(sentry__dsn_parse(
                         &dsn, "http://username:password@example.com/42?x=y#z"),
        0);
    char *url = sentry__dsn_get_store_url(&dsn);
    assert_string_equal(url, "http://example.com:80/api/42/store/");
    sentry_free(url);
    url = sentry__dsn_get_minidump_url(&dsn);
    assert_string_equal(
        url, "http://example.com:80/api/42/minidump/?sentry_key=username");
    sentry_free(url);
    url = sentry__dsn_get_attachment_url(&dsn, &uuid);
    assert_string_equal(url,
        "http://example.com:80/api/42/events/"
        "4c7e771a-f17d-4220-bc8f-5b1edcdb5faa/attachments/");
    sentry_free(url);
    sentry__dsn_cleanup(&dsn);
}