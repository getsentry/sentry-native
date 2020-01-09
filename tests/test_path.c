#include "../src/sentry_path.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(path_joining_unix)
{
#ifndef SENTRY_PLATFORM_UNIX
    SKIP_TEST();
#else
    sentry_path_t *path = sentry__path_new("foo/bar/baz.txt");
    sentry_path_t *joined;

    TEST_CHECK(strcmp(path->path, "foo/bar/baz.txt") == 0);
    TEST_CHECK(strcmp(sentry__path_filename(path), "baz.txt") == 0);

    joined = sentry__path_join_str(path, "extra");
    TEST_CHECK(strcmp(joined->path, "foo/bar/baz.txt/extra") == 0);
    TEST_CHECK(strcmp(sentry__path_filename(joined), "extra") == 0);
    sentry__path_free(joined);

    joined = sentry__path_join_str(path, "/root/path");
    TEST_CHECK(strcmp(joined->path, "/root/path") == 0);
    TEST_CHECK(strcmp(sentry__path_filename(joined), "path") == 0);
    sentry__path_free(joined);

    sentry__path_free(path);
#endif
}

SENTRY_TEST(path_joining_windows)
{
#ifndef SENTRY_PLATFORM_WINDOWS
    SKIP_TEST();
#else
    sentry_path_t *path = sentry__path_new(L"foo/bar/baz.txt");
    sentry_path_t *winpath = sentry__path_new(L"foo\\bar\\baz.txt");
    sentry_path_t *awinpath = sentry__path_from_str("foo\\bar\\baz.txt");
    sentry_path_t *cpath = sentry__path_from_str("C:\\foo\\bar\\baz.txt");
    sentry_path_t *joined;

    TEST_CHECK(_wcsicmp(path->path, L"foo/bar/baz.txt") == 0);
    TEST_CHECK(_wcsicmp(sentry__path_filename(path), L"baz.txt") == 0);
    TEST_CHECK(_wcsicmp(winpath->path, L"foo\\bar\\baz.txt") == 0);
    TEST_CHECK(_wcsicmp(awinpath->path, L"foo\\bar\\baz.txt") == 0);
    TEST_CHECK(_wcsicmp(sentry__path_filename(winpath), L"baz.txt") == 0);

    joined = sentry__path_join_str(path, "extra");
    TEST_CHECK(_wcsicmp(joined->path, L"foo/bar/baz.txt\\extra") == 0);
    TEST_CHECK(_wcsicmp(sentry__path_filename(joined), L"extra") == 0);
    sentry__path_free(joined);

    joined = sentry__path_join_str(path, "/root/path");
    TEST_CHECK(_wcsicmp(joined->path, L"/root/path") == 0);
    TEST_CHECK(_wcsicmp(sentry__path_filename(joined), L"path") == 0);
    sentry__path_free(joined);

    joined = sentry__path_join_str(cpath, "/root/path");
    TEST_CHECK(_wcsicmp(joined->path, L"C:/root/path") == 0);
    sentry__path_free(joined);

    joined = sentry__path_join_str(cpath, "D:\\root\\path");
    TEST_CHECK(_wcsicmp(joined->path, L"D:\\root\\path") == 0);
    sentry__path_free(joined);

    joined = sentry__path_join_str(cpath, "\\root\\path");
    TEST_CHECK(_wcsicmp(joined->path, L"C:\\root\\path") == 0);
    sentry__path_free(joined);

    sentry__path_free(cpath);
    sentry__path_free(awinpath);
    sentry__path_free(winpath);
    sentry__path_free(path);
#endif
}

SENTRY_TEST(path_basics)
{
    size_t items = 0;
    const sentry_path_t *p;
    sentry_path_t *path = sentry__path_from_str(".");
    TEST_CHECK(!!path);

    sentry_pathiter_t *piter = sentry__path_iter_directory(path);
    while ((p = sentry__pathiter_next(piter)) != NULL) {
        bool is_file = sentry__path_is_file(p);
        bool is_dir = sentry__path_is_dir(p);
        TEST_CHECK(is_file || is_dir);
        items += 1;
    }
    TEST_CHECK(items > 0);

    sentry__pathiter_free(piter);
    sentry__path_free(path);
}
