#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_testsupport.h"
#include "sentry_uuid.h"
#include "sentry_value.h"

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#elif !defined(SENTRY_PLATFORM_NX) && !defined(SENTRY_PLATFORM_PS)
#    include <utime.h>
#endif

static int
set_file_mtime(const sentry_path_t *path, time_t mtime)
{
#ifdef SENTRY_PLATFORM_WINDOWS
    HANDLE h = CreateFileW(path->path_w, FILE_WRITE_ATTRIBUTES, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return -1;
    }
    // 100 ns intervals since January 1, 1601 (UTC)
    uint64_t t = ((uint64_t)mtime * 10000000ULL) + 116444736000000000ULL;
    FILETIME ft = { (DWORD)t, (DWORD)(t >> 32) };
    BOOL rv = SetFileTime(h, NULL, NULL, &ft);
    CloseHandle(h);
    return rv ? 0 : -1;
#elif !defined(SENTRY_PLATFORM_NX) && !defined(SENTRY_PLATFORM_PS)
    struct utimbuf times = { .modtime = mtime, .actime = mtime };
    return utime(path->path, &times);
#else
    (void)path;
    (void)mtime;
    return -1;
#endif
}

SENTRY_TEST(cache_keep)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_cache_keep(options, true);
    sentry_init(options);

    sentry_path_t *cache_path
        = sentry__path_join_str(options->database_path, "cache");
    TEST_ASSERT(!!cache_path);
    TEST_ASSERT(sentry__path_remove_all(cache_path) == 0);

    sentry_path_t *old_run_path
        = sentry__path_join_str(options->database_path, "old.run");
    TEST_ASSERT(!!old_run_path);
    TEST_ASSERT(sentry__path_create_dir_all(old_run_path) == 0);

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_ASSERT(!!envelope);
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    sentry_value_t event = sentry__value_new_event_with_id(&event_id);
    sentry__envelope_add_event(envelope, event);

    char *envelope_filename = sentry__uuid_as_filename(&event_id, ".envelope");
    TEST_ASSERT(!!envelope_filename);
    sentry_path_t *old_envelope_path
        = sentry__path_join_str(old_run_path, envelope_filename);
    TEST_ASSERT(
        sentry_envelope_write_to_path(envelope, old_envelope_path) == 0);
    sentry_envelope_free(envelope);

    sentry_path_t *cached_envelope_path
        = sentry__path_join_str(cache_path, envelope_filename);
    TEST_ASSERT(!!cached_envelope_path);

    TEST_ASSERT(sentry__path_is_file(old_envelope_path));
    TEST_ASSERT(!sentry__path_is_file(cached_envelope_path));

    sentry__process_old_runs(options, 0);

    TEST_ASSERT(!sentry__path_is_file(old_envelope_path));
    TEST_ASSERT(sentry__path_is_file(cached_envelope_path));

    sentry__path_free(old_envelope_path);
    sentry__path_free(cached_envelope_path);
    sentry__path_free(old_run_path);
    sentry__path_free(cache_path);
    sentry_free(envelope_filename);
    sentry_close();
}

SENTRY_TEST(cache_max_size)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_cache_keep(options, true);
    sentry_options_set_cache_max_size(options, 10 * 1024); // 10 kb
    sentry_init(options);

    sentry_path_t *cache_path
        = sentry__path_join_str(options->database_path, "cache");
    TEST_ASSERT(!!cache_path);
    TEST_ASSERT(sentry__path_remove_all(cache_path) == 0);
    TEST_ASSERT(sentry__path_create_dir_all(cache_path) == 0);

    // 10 x 5 kb files
    for (int i = 0; i < 10; i++) {
        sentry_uuid_t event_id = sentry_uuid_new_v4();
        char *filename = sentry__uuid_as_filename(&event_id, ".envelope");
        TEST_ASSERT(!!filename);
        sentry_path_t *filepath = sentry__path_join_str(cache_path, filename);
        sentry_free(filename);

        sentry_filewriter_t *fw = sentry__filewriter_new(filepath);
        TEST_ASSERT(!!fw);
        for (int j = 0; j < 500; j++) {
            sentry__filewriter_write(fw, "0123456789", 10);
        }
        TEST_CHECK_INT_EQUAL(sentry__filewriter_byte_count(fw), 5000);
        sentry__filewriter_free(fw);
        sentry__path_free(filepath);
    }

    sentry__cleanup_cache(options);

    int cache_count = 0;
    size_t cache_size = 0;
    sentry_pathiter_t *iter = sentry__path_iter_directory(cache_path);
    const sentry_path_t *entry;
    while (iter && (entry = sentry__pathiter_next(iter)) != NULL) {
        cache_count++;
        cache_size += sentry__path_get_size(entry);
    }
    sentry__pathiter_free(iter);

    TEST_CHECK_INT_EQUAL(cache_count, 2);
    TEST_CHECK(cache_size <= 10 * 1024);

    sentry__path_free(cache_path);
    sentry_close();
}

SENTRY_TEST(cache_max_age)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_cache_keep(options, true);
    sentry_options_set_cache_max_age(options, 5 * 24 * 60 * 60); // 5 days
    sentry_init(options);

    sentry_path_t *cache_path
        = sentry__path_join_str(options->database_path, "cache");
    TEST_ASSERT(!!cache_path);
    TEST_ASSERT(sentry__path_remove_all(cache_path) == 0);
    TEST_ASSERT(sentry__path_create_dir_all(cache_path) == 0);

    // 0,2,4,6,8 days ago
    time_t now = time(NULL);
    for (int i = 0; i < 5; i++) {
        sentry_uuid_t event_id = sentry_uuid_new_v4();
        char *filename = sentry__uuid_as_filename(&event_id, ".envelope");
        TEST_ASSERT(!!filename);
        sentry_path_t *filepath = sentry__path_join_str(cache_path, filename);
        sentry_free(filename);

        TEST_ASSERT(sentry__path_touch(filepath) == 0);
        time_t mtime = now - (i * 2 * 24 * 60 * 60); // N days ago
        TEST_ASSERT(set_file_mtime(filepath, mtime) == 0);
        sentry__path_free(filepath);
    }

    sentry__cleanup_cache(options);

    int cache_count = 0;
    sentry_pathiter_t *iter = sentry__path_iter_directory(cache_path);
    const sentry_path_t *entry;
    while (iter && (entry = sentry__pathiter_next(iter)) != NULL) {
        cache_count++;
        time_t mtime = sentry__path_get_mtime(entry);
        TEST_CHECK(now - mtime <= (5 * 24 * 60 * 60));
    }
    sentry__pathiter_free(iter);

    TEST_CHECK_INT_EQUAL(cache_count, 3);

    sentry__path_free(cache_path);
    sentry_close();
}

SENTRY_TEST(cache_max_items)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_cache_keep(options, true);
    sentry_options_set_cache_max_items(options, 5);
    sentry_init(options);

    sentry_path_t *cache_path
        = sentry__path_join_str(options->database_path, "cache");
    TEST_ASSERT(!!cache_path);
    TEST_ASSERT(sentry__path_remove_all(cache_path) == 0);
    TEST_ASSERT(sentry__path_create_dir_all(cache_path) == 0);

    time_t now = time(NULL);
    for (int i = 0; i < 10; i++) {
        sentry_uuid_t event_id = sentry_uuid_new_v4();
        char *filename = sentry__uuid_as_filename(&event_id, ".envelope");
        TEST_ASSERT(!!filename);
        sentry_path_t *filepath = sentry__path_join_str(cache_path, filename);
        sentry_free(filename);

        TEST_ASSERT(sentry__path_touch(filepath) == 0);
        time_t mtime = now - (i * 60);
        TEST_ASSERT(set_file_mtime(filepath, mtime) == 0);
        sentry__path_free(filepath);
    }

    sentry__cleanup_cache(options);

    int cache_count = 0;
    sentry_pathiter_t *iter = sentry__path_iter_directory(cache_path);
    const sentry_path_t *entry;
    while (iter && (entry = sentry__pathiter_next(iter)) != NULL) {
        cache_count++;
    }
    sentry__pathiter_free(iter);

    TEST_CHECK_INT_EQUAL(cache_count, 5);

    sentry__path_free(cache_path);
    sentry_close();
}

SENTRY_TEST(cache_max_size_and_age)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    // Verify size pruning keeps newer entries, removes all older once limit
    // hit. A (5KB), B (6KB), C (3KB) newest-to-oldest, max_size=10KB A+B=11KB
    // exceeds limit -> B pruned, C (older) also pruned
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_cache_keep(options, true);
    sentry_options_set_cache_max_size(options, 10 * 1024); // 10 KB
    sentry_init(options);

    sentry_path_t *cache_path
        = sentry__path_join_str(options->database_path, "cache");
    TEST_ASSERT(!!cache_path);
    TEST_ASSERT(sentry__path_remove_all(cache_path) == 0);
    TEST_ASSERT(sentry__path_create_dir_all(cache_path) == 0);

    time_t now = time(NULL);

    // A (newest, 5KB), B (middle, 6KB), C (oldest, 3KB)
    struct {
        const char *name;
        size_t size;
        time_t age;
    } files[] = {
        { "a.envelope", 5 * 1024, 0 }, // newest
        { "b.envelope", 6 * 1024, 60 }, // 1 min old
        { "c.envelope", 3 * 1024, 120 }, // 2 min old
    };

    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        sentry_path_t *filepath
            = sentry__path_join_str(cache_path, files[i].name);
        TEST_ASSERT(!!filepath);

        sentry_filewriter_t *fw = sentry__filewriter_new(filepath);
        TEST_ASSERT(!!fw);
        for (size_t j = 0; j < files[i].size / 10; j++) {
            sentry__filewriter_write(fw, "0123456789", 10);
        }
        sentry__filewriter_free(fw);

        TEST_ASSERT(set_file_mtime(filepath, now - files[i].age) == 0);
        sentry__path_free(filepath);
    }

    sentry__cleanup_cache(options);

    // Verify: A kept, B and C removed (once limit hit, all older removed)
    sentry_path_t *a_path = sentry__path_join_str(cache_path, "a.envelope");
    sentry_path_t *b_path = sentry__path_join_str(cache_path, "b.envelope");
    sentry_path_t *c_path = sentry__path_join_str(cache_path, "c.envelope");

    TEST_CHECK(sentry__path_is_file(a_path)); // newest, kept
    TEST_CHECK(!sentry__path_is_file(b_path)); // size-pruned
    TEST_CHECK(!sentry__path_is_file(c_path)); // older than B, also pruned

    sentry__path_free(a_path);
    sentry__path_free(b_path);
    sentry__path_free(c_path);
    sentry__path_free(cache_path);
    sentry_close();
}
