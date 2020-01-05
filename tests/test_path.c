#include "../src/sentry_path.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(path_joining_unix)
{
#ifndef SENTRY_PLATFORM_UNIX
    skip();
#else
    sentry_path_t *path = sentry__path_new("foo/bar/baz.txt");
    sentry_path_t *joined;

    TEST_ASSERT(strcmp(path->path, "foo/bar/baz.txt") == 0);
    TEST_ASSERT(strcmp(sentry__path_filename(path), "baz.txt") == 0);

    joined = sentry__path_join_str(path, "extra");
    TEST_ASSERT(strcmp(joined->path, "foo/bar/baz.txt/extra") == 0);
    TEST_ASSERT(strcmp(sentry__path_filename(joined), "extra") == 0);
    sentry__path_free(joined);

    joined = sentry__path_join_str(path, "/root/path");
    TEST_ASSERT(strcmp(joined->path, "/root/path") == 0);
    TEST_ASSERT(strcmp(sentry__path_filename(joined), "path") == 0);
    sentry__path_free(joined);

    sentry__path_free(path);
#endif
}

SENTRY_TEST(path_basics)
{
    size_t items = 0;
    const sentry_path_t *p;
    sentry_path_t *path = sentry__path_from_str("./src");
    TEST_ASSERT(!!path);

    sentry_pathiter_t *piter = sentry__path_iter_directory(path);
    while ((p = sentry__pathiter_next(piter)) != NULL) {
        bool is_file = sentry__path_is_file(p);
        bool is_dir = sentry__path_is_dir(p);
        TEST_ASSERT(is_file || is_dir);
        items += 1;
    }
    TEST_ASSERT(items > 0);

    sentry__pathiter_free(piter);
}
