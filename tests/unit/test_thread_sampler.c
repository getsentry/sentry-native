#include "sentry_testsupport.h"

#if defined(__linux__) || defined(__ANDROID__)
#    include <pthread.h>
#    include <sys/syscall.h>
#    include <unistd.h>

SENTRY_TEST(thread_sampler_samples_self)
{
    void *frames[32];
    pid_t tid = (pid_t)syscall(SYS_gettid);
    size_t n = sentry_unwind_thread_stack((int)tid, frames, 32);
    // Unit tests run on Linux with the vendored libunwind compiled in, so the
    // sampler must successfully unwind at least one frame from this thread.
    // A zero result here indicates a regression in signal delivery, the TID
    // guard, or libunwind initialisation.
    TEST_CHECK(n >= 1);
    TEST_CHECK(n <= 32);
    TEST_CHECK(frames[0] != NULL);
}

struct sampler_worker_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t tid_published;
    pthread_cond_t exit_requested;
    pid_t tid;
    int should_exit;
};

static void *
sampler_worker_thread(void *arg)
{
    struct sampler_worker_ctx *ctx = (struct sampler_worker_ctx *)arg;
    pthread_mutex_lock(&ctx->mutex);
    ctx->tid = (pid_t)syscall(SYS_gettid);
    pthread_cond_signal(&ctx->tid_published);
    // Park until the main thread is done sampling. The SIGRTMIN+5 delivery
    // interrupts the futex wait transparently; the while-loop also guards
    // against spurious wakeups.
    while (!ctx->should_exit) {
        pthread_cond_wait(&ctx->exit_requested, &ctx->mutex);
    }
    pthread_mutex_unlock(&ctx->mutex);
    return NULL;
}

SENTRY_TEST(thread_sampler_samples_other_thread)
{
    // The self-sample test exercises signal delivery from a thread to itself.
    // This test exercises the cross-thread tgkill path that ANR / frozen-frame
    // callers actually use, where the sampler thread and the sampled thread
    // are distinct.
    struct sampler_worker_ctx ctx;
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.tid_published, NULL);
    pthread_cond_init(&ctx.exit_requested, NULL);
    ctx.tid = 0;
    ctx.should_exit = 0;

    pthread_t worker;
    TEST_CHECK_INT_EQUAL(
        pthread_create(&worker, NULL, sampler_worker_thread, &ctx), 0);

    pthread_mutex_lock(&ctx.mutex);
    while (ctx.tid == 0) {
        pthread_cond_wait(&ctx.tid_published, &ctx.mutex);
    }
    const pid_t worker_tid = ctx.tid;
    pthread_mutex_unlock(&ctx.mutex);

    void *frames[32];
    size_t n = sentry_unwind_thread_stack((int)worker_tid, frames, 32);
    TEST_CHECK(n >= 1);
    TEST_CHECK(n <= 32);
    TEST_CHECK(frames[0] != NULL);

    pthread_mutex_lock(&ctx.mutex);
    ctx.should_exit = 1;
    pthread_cond_signal(&ctx.exit_requested);
    pthread_mutex_unlock(&ctx.mutex);
    pthread_join(worker, NULL);

    pthread_cond_destroy(&ctx.tid_published);
    pthread_cond_destroy(&ctx.exit_requested);
    pthread_mutex_destroy(&ctx.mutex);
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

SENTRY_TEST(thread_sampler_samples_other_thread)
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
