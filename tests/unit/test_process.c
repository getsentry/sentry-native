#include "sentry_path.h"
#include "sentry_process.h"
#include "sentry_testsupport.h"

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#    define sleep_ms(MILLISECONDS) Sleep(MILLISECONDS)
#else
#    include <unistd.h>
#    define sleep_ms(SECONDS) usleep(SECONDS * 1000)
#endif

SENTRY_TEST(process_invalid)
{
    TEST_CHECK(!sentry__process_spawn(NULL, NULL));

    sentry_path_t *empty = sentry__path_from_str("");
    TEST_CHECK(!sentry__process_spawn(empty, NULL));
    sentry__path_free(empty);

    sentry_path_t *nul = sentry__path_from_str_owned(NULL);
    TEST_CHECK(!sentry__process_spawn(nul, NULL));
    sentry__path_free(nul);
}

SENTRY_TEST(process_spawn)
{
#if defined(SENTRY_PLATFORM_WINDOWS) || defined(SENTRY_PLATFORM_MACOS)         \
    || (defined(SENTRY_PLATFORM_LINUX) && !defined(SENTRY_PLATFORM_ANDROID))
    sentry_path_t *exe = sentry__path_current_exe();
    TEST_ASSERT(!!exe);
    TEST_CHECK(sentry__path_is_file(exe));

    sentry_path_t *dst = sentry__path_from_str(
        SENTRY_TEST_PATH_PREFIX ".sentry_test_unit_process_spawn");
    TEST_ASSERT(!!dst);
    TEST_CHECK(sentry__path_remove(dst) == 0);
    TEST_CHECK(!sentry__path_is_file(dst));

#    ifdef SENTRY_PLATFORM_WINDOWS
    // cmd /C copy <src> <dst>
    sentry_path_t *cmd = sentry__path_from_str("cmd.exe");
    TEST_ASSERT(!!cmd);
    TEST_CHECK(
        sentry__process_spawn(cmd, L"/C", L"copy", exe->path, dst->path, NULL));
    sentry__path_free(cmd);
#    else
    // cp <src> <dst>
    sentry_path_t *cp = sentry__path_from_str("cp");
    TEST_ASSERT(!!cp);
    TEST_CHECK(sentry__process_spawn(cp, exe->path, dst->path, NULL));
    sentry__path_free(cp);
#    endif

    size_t size = sentry__path_get_size(exe);

    // wait up to 5s for the detached copy process
    int i = 0;
    while (i++ < 100
        && (!sentry__path_is_file(dst) || sentry__path_get_size(dst) < size)) {
        sleep_ms(50);
    }
    TEST_CHECK(sentry__path_is_file(dst));
    TEST_CHECK_INT_EQUAL(sentry__path_get_size(dst), size);

    sentry__path_remove(dst);
    sentry__path_free(exe);
    sentry__path_free(dst);
#else
    SKIP_TEST();
#endif
}
