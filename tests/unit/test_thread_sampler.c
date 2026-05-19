#include "sentry_testsupport.h"

#if defined(__linux__) || defined(__ANDROID__)
#    include <sys/syscall.h>
#    include <unistd.h>

SENTRY_TEST(thread_sampler_samples_self)
{
    void *frames[32];
    pid_t tid = (pid_t)syscall(SYS_gettid);
    size_t n = sentry_unwind_thread_stack((int)tid, frames, 32);
    // Unit tests run on Linux with the vendored libunwind compiled in, so the
    // sampler must successfully unwind at least one frame from this thread.
    // A zero result here indicates a regression in signal delivery, the seq
    // guard, or libunwind initialisation.
    TEST_CHECK(n >= 1);
    TEST_CHECK(n <= 32);
    TEST_CHECK(frames[0] != NULL);
}

SENTRY_TEST(thread_sampler_rejects_invalid_tid)
{
    void *frames[32];
    size_t n = sentry_unwind_thread_stack(-1, frames, 32);
    TEST_CHECK_INT_EQUAL((int)n, 0);

    n = sentry_unwind_thread_stack(0, frames, 32);
    TEST_CHECK_INT_EQUAL((int)n, 0);
}

SENTRY_TEST(thread_sampler_rejects_null_buf)
{
    size_t n = sentry_unwind_thread_stack(1, NULL, 32);
    TEST_CHECK_INT_EQUAL((int)n, 0);
}

SENTRY_TEST(thread_sampler_rejects_zero_max)
{
    void *frames[1];
    size_t n = sentry_unwind_thread_stack(1, frames, 0);
    TEST_CHECK_INT_EQUAL((int)n, 0);
}

#else // non-Linux/Android: function must be a no-op returning 0.

SENTRY_TEST(thread_sampler_samples_self)
{
    void *frames[32];
    size_t n = sentry_unwind_thread_stack(1, frames, 32);
    TEST_CHECK_INT_EQUAL((int)n, 0);
}

SENTRY_TEST(thread_sampler_rejects_invalid_tid)
{
    void *frames[32];
    size_t n = sentry_unwind_thread_stack(-1, frames, 32);
    TEST_CHECK_INT_EQUAL((int)n, 0);
    n = sentry_unwind_thread_stack(0, frames, 32);
    TEST_CHECK_INT_EQUAL((int)n, 0);
}

SENTRY_TEST(thread_sampler_rejects_null_buf)
{
    size_t n = sentry_unwind_thread_stack(1, NULL, 32);
    TEST_CHECK_INT_EQUAL((int)n, 0);
}

SENTRY_TEST(thread_sampler_rejects_zero_max)
{
    void *frames[1];
    size_t n = sentry_unwind_thread_stack(1, frames, 0);
    TEST_CHECK_INT_EQUAL((int)n, 0);
}

#endif
