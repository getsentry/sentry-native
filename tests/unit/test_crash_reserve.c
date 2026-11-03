#include "sentry_crash_reserve.h"
#include "sentry_testsupport.h"

#if defined(SENTRY_PLATFORM_WINDOWS)                                           \
    || (defined(SENTRY_PLATFORM_UNIX) && !defined(SENTRY_PLATFORM_PS))
#    define CRASH_RESERVE_SUPPORTED
#endif

SENTRY_TEST(crash_reserve_alloc_release)
{
#ifndef CRASH_RESERVE_SUPPORTED
    SKIP_TEST();
#else
    sentry__crash_reserve_alloc(64 * 1024);
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 64 * 1024);

    sentry__crash_reserve_release();
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 0);

    // releasing again is a no-op
    sentry__crash_reserve_release();
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 0);
#endif
}

SENTRY_TEST(crash_reserve_disabled)
{
    sentry__crash_reserve_alloc(0);
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 0);
    sentry__crash_reserve_release();
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 0);
}

SENTRY_TEST(crash_reserve_init_close)
{
#ifndef CRASH_RESERVE_SUPPORTED
    SKIP_TEST();
#else
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_crash_memory_reserve(options, 128 * 1024);
    sentry_init(options);
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 128 * 1024);

    sentry_close();
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 0);

    SENTRY_TEST_OPTIONS_NEW(default_options);
    sentry_init(default_options);
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(),
        (size_t)SENTRY_CRASH_MEMORY_RESERVE * 1024);
    sentry_close();
#endif
}

SENTRY_TEST(crash_reserve_realloc)
{
#ifndef CRASH_RESERVE_SUPPORTED
    SKIP_TEST();
#else
    sentry__crash_reserve_alloc(64 * 1024);
    sentry__crash_reserve_alloc(256 * 1024);
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 256 * 1024);

    sentry__crash_reserve_release();
    TEST_CHECK_INT_EQUAL(sentry__crash_reserve_size(), 0);
#endif
}
