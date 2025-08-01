#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_testsupport.h"

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#    define sleep_s(SECONDS) Sleep(SECONDS * 1000)
#else
#    include <unistd.h>
#    define sleep_s(SECONDS) sleep(SECONDS)
#endif

SENTRY_TEST(recursive_paths)
{
    sentry_path_t *base = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".foo");
    TEST_ASSERT(!!base);
    sentry_path_t *nested = sentry__path_join_str(base, "bar");
    TEST_ASSERT(!!nested);
    sentry_path_t *nested2 = sentry__path_join_str(nested, "baz");
    TEST_ASSERT(!!nested2);
    sentry_path_t *file =
#ifdef SENTRY_PLATFORM_WINDOWS
        sentry__path_join_wstr(nested2, L"unicode ❤️ Юля.txt");
#else
        sentry__path_join_str(nested2, "unicode ❤️ Юля.txt");
#endif
    TEST_ASSERT(!!file);

    sentry__path_create_dir_all(nested2);
    sentry__path_touch(file);

    TEST_CHECK(sentry__path_is_file(file));

    sentry__path_remove_all(nested);

    TEST_CHECK(!sentry__path_is_file(file));
    TEST_CHECK(!sentry__path_is_file(nested));
    TEST_CHECK(sentry__path_is_dir(base));

    sentry__path_remove_all(base);

    sentry__path_free(file);
    sentry__path_free(nested2);
    sentry__path_free(nested);
    sentry__path_free(base);
}

SENTRY_TEST(path_joining_unix)
{
#ifndef SENTRY_PLATFORM_UNIX
    SKIP_TEST();
#else
    sentry_path_t *path = sentry__path_new("foo/bar/baz.txt");
    TEST_ASSERT(!!path);
    sentry_path_t *joined;

    TEST_CHECK(strcmp(path->path, "foo/bar/baz.txt") == 0);
    TEST_CHECK(strcmp(sentry__path_filename(path), "baz.txt") == 0);

    joined = sentry__path_join_str(path, "extra");
    TEST_ASSERT(!!joined);
    TEST_CHECK(strcmp(joined->path, "foo/bar/baz.txt/extra") == 0);
    TEST_CHECK(strcmp(sentry__path_filename(joined), "extra") == 0);
    sentry__path_free(joined);

    joined = sentry__path_join_str(path, "/root/path");
    TEST_ASSERT(!!joined);
    TEST_CHECK(strcmp(joined->path, "/root/path") == 0);
    TEST_CHECK(strcmp(sentry__path_filename(joined), "path") == 0);
    sentry__path_free(joined);

    sentry__path_free(path);
#endif
}

SENTRY_TEST(path_from_str_null)
{
    TEST_CHECK(NULL == sentry__path_from_str(NULL));
    TEST_CHECK(NULL == sentry__path_from_str_n(NULL, 0));
    TEST_CHECK(NULL == sentry__path_from_str_n(NULL, 10));
}

SENTRY_TEST(path_from_str_n_wo_null_termination)
{
    // provide non-null-terminated path string with buffer character at the end.
    char path_str[] = { 't', 'e', 's', 't', 'X' };
    sentry_path_t *test_path = sentry__path_from_str_n(path_str, 4);
    TEST_ASSERT(!!test_path);
#ifdef SENTRY_PLATFORM_WINDOWS
    TEST_CHECK_WSTRING_EQUAL(test_path->path, L"test");
#else
    TEST_CHECK_STRING_EQUAL(test_path->path, "test");
#endif
    sentry__path_free(test_path);
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

SENTRY_TEST(path_relative_filename)
{
    sentry_path_t *path = sentry__path_from_str("foobar.txt");
    TEST_ASSERT(!!path);
#ifdef SENTRY_PLATFORM_WINDOWS
    char *filename = sentry__string_from_wstr(sentry__path_filename(path));
    TEST_CHECK_STRING_EQUAL(filename, "foobar.txt");
    sentry_free(filename);
#else
    TEST_CHECK_STRING_EQUAL(sentry__path_filename(path), "foobar.txt");
#endif
    sentry__path_free(path);
}

SENTRY_TEST(path_basics)
{
    size_t items = 0;
    const sentry_path_t *p;
    sentry_path_t *path = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".");
    TEST_CHECK(!!path);

    sentry_pathiter_t *piter = sentry__path_iter_directory(path);
    TEST_ASSERT(piter != NULL);
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

SENTRY_TEST(path_current_exe)
{
    sentry_path_t *path = sentry__path_current_exe();
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    // Not available on NX or PS
    TEST_CHECK(!path);
#else
    TEST_CHECK(!!path);
    if (path) {
        TEST_CHECK(sentry__path_is_file(path));
        sentry__path_free(path);
    }
#endif
}

SENTRY_TEST(path_directory)
{
    sentry_path_t *path_1
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX "foo");
    TEST_ASSERT(!!path_1);
    sentry_path_t *path_2
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX "foo/bar");
    TEST_ASSERT(!!path_2);
#ifdef SENTRY_PLATFORM_WINDOWS
    sentry_path_t *path_3
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX "foo/bar\\baz");

    // `%TEMP%\sentry_test_unit`
    wchar_t temp_folder[MAX_PATH];
    GetEnvironmentVariableW(L"TEMP", temp_folder, sizeof(temp_folder));
    sentry_path_t *path_4 = sentry__path_from_wstr(temp_folder);
    path_4 = sentry__path_join_str(path_4, "sentry_test_unit");
#endif

    // cleanup before tests
    sentry__path_remove_all(path_1);

    // create single directory
    sentry__path_create_dir_all(path_1);
    TEST_CHECK(sentry__path_is_dir(path_1));

    sentry__path_remove(path_1);
    TEST_CHECK(!sentry__path_is_dir(path_1));

    // create directories by path with forward slash
    sentry__path_create_dir_all(path_2);
    TEST_CHECK(sentry__path_is_dir(path_2));

    sentry__path_remove_all(path_2);
    TEST_CHECK(!sentry__path_is_dir(path_2));

#ifdef SENTRY_PLATFORM_WINDOWS
    // create directories by path with forward slash and backward slashes
    sentry__path_create_dir_all(path_3);
    TEST_CHECK(sentry__path_is_dir(path_3));

    sentry__path_remove_all(path_3);
    TEST_CHECK(!sentry__path_is_dir(path_3));
    sentry__path_free(path_3);

    // create directories with absolute path
    sentry__path_create_dir_all(path_4);
    TEST_CHECK(sentry__path_is_dir(path_4));

    sentry__path_remove_all(path_4);
    TEST_CHECK(!sentry__path_is_dir(path_4));
    sentry__path_free(path_4);
#endif

    sentry__path_remove_all(path_1);
    sentry__path_free(path_1);
    sentry__path_free(path_2);
}

SENTRY_TEST(path_mtime)
{
    sentry_path_t *path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX "foo.txt");
    TEST_ASSERT(!!path);

    TEST_CHECK(sentry__path_remove(path) == 0);
    TEST_CHECK(!sentry__path_is_file(path));
    TEST_CHECK(sentry__path_get_mtime(path) <= 0);

    sentry__path_touch(path);
    TEST_CHECK(sentry__path_is_file(path));

    const time_t before = sentry__path_get_mtime(path);
    TEST_CHECK(before > 0);

    sleep_s(1);

    sentry__path_write_buffer(path, "after", 5);
    const time_t after = sentry__path_get_mtime(path);
    TEST_CHECK(after > 0);
    TEST_CHECK(before < after);

    sentry__path_remove(path);
    sentry__path_free(path);
}
