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
}

SENTRY_TEST(process_touch_one)
{
#if !defined(SENTRY_PLATFORM_LINUX) && !defined(SENTRY_PLATFORM_MACOS)
    SKIP_TEST();
#endif
    sentry_path_t *arg
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX "arg.txt");
    TEST_ASSERT(!!arg);
    TEST_CHECK(sentry__path_remove(arg) == 0);
    TEST_CHECK(!sentry__path_is_file(arg));

    sentry_path_t *touch = sentry__path_from_str("/usr/bin/touch");
    TEST_ASSERT(!!touch);
    TEST_CHECK(sentry__process_spawn(touch, arg->path, NULL));

    int i = 0;
    while (i++ < 100 && (!sentry__path_is_file(arg))) {
        sleep_ms(50);
    }
    TEST_CHECK(sentry__path_is_file(arg));

    sentry__path_remove(arg);
    sentry__path_free(arg);
    sentry__path_free(touch);
}

SENTRY_TEST(process_touch_two)
{
#if !defined(SENTRY_PLATFORM_LINUX) && !defined(SENTRY_PLATFORM_MACOS)
    SKIP_TEST();
#endif
    sentry_path_t *arg0
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX "arg0.txt");
    TEST_ASSERT(!!arg0);
    TEST_CHECK(sentry__path_remove(arg0) == 0);
    TEST_CHECK(!sentry__path_is_file(arg0));

    sentry_path_t *arg1
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX "arg1.txt");
    TEST_ASSERT(!!arg1);
    TEST_CHECK(sentry__path_remove(arg1) == 0);
    TEST_CHECK(!sentry__path_is_file(arg1));

    sentry_path_t *touch = sentry__path_from_str("/usr/bin/touch");
    TEST_ASSERT(!!touch);
    TEST_CHECK(sentry__process_spawn(touch, arg0->path, arg1->path, NULL));

    int i = 0;
    while (i++ < 100
        && (!sentry__path_is_file(arg0) || !sentry__path_is_file(arg1))) {
        sleep_ms(50);
    }
    TEST_CHECK(sentry__path_is_file(arg0));
    TEST_CHECK(sentry__path_is_file(arg1));

    sentry__path_remove(arg0);
    sentry__path_remove(arg1);
    sentry__path_free(arg0);
    sentry__path_free(arg1);
    sentry__path_free(touch);
}
