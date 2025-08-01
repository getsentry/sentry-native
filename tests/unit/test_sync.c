#include "sentry_core.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#    define sleep_s(SECONDS) Sleep((SECONDS) * 1000)
#else
#    include <unistd.h>
#    define sleep_s(SECONDS) sleep(SECONDS)
#endif

struct task_state {
    int executed;
    bool running;
};

static void
task_func(void *data, void *UNUSED(state))
{
    struct task_state *state = data;
    state->executed++;
}

static void
cleanup_func(void *data)
{
    struct task_state *state = data;
    state->running = false;
}

SENTRY_TEST(background_worker)
{
    for (size_t i = 0; i < 100; i++) {
        sentry_bgworker_t *bgw = sentry__bgworker_new(NULL, NULL);
        TEST_ASSERT(!!bgw);

        sentry__bgworker_start(bgw);

        struct task_state ts;
        ts.executed = 0;
        ts.running = true;
        for (size_t j = 0; j < 10; j++) {
            sentry__bgworker_submit(bgw, task_func, cleanup_func, &ts);
        }

        TEST_CHECK_INT_EQUAL(sentry__bgworker_shutdown(bgw, 5000), 0);
        sentry__bgworker_decref(bgw);

        TEST_CHECK_INT_EQUAL(ts.executed, 10);
        TEST_CHECK(!ts.running);
    }
}

static void
sleep_task(void *UNUSED(data), void *UNUSED(state))
{
    sleep_s(1);
}

static sentry_cond_t trailing_task_done;
static sentry_mutex_t executed_lock = SENTRY__MUTEX_INIT;

static void
trailing_task(void *data, void *UNUSED(state))
{
    sentry__mutex_lock(&executed_lock);
    bool *executed = (bool *)data;
    *executed = true;
    sentry__mutex_unlock(&executed_lock);
    sentry__cond_wake(&trailing_task_done);
}

static bool
drop_lessthan(void *task, void *data)
{
    return (size_t)task < (size_t)data;
}

static bool
drop_greaterthan(void *task, void *data)
{
    return (size_t)task > (size_t)data;
}

static bool
collect(void *task, void *data)
{
    sentry_value_t *list = (sentry_value_t *)data;
    sentry_value_append(*list, sentry_value_new_int32((int32_t)(size_t)task));
    return true;
}

SENTRY_TEST(task_queue)
{
    sentry__cond_init(&trailing_task_done);
    sentry_bgworker_t *bgw = sentry__bgworker_new(NULL, NULL);
    TEST_ASSERT(!!bgw);
    sentry__bgworker_submit(bgw, sleep_task, NULL, NULL);
    sentry__bgworker_decref(bgw);

    bgw = sentry__bgworker_new(NULL, NULL);
    TEST_ASSERT(!!bgw);

    // submit before starting
    for (size_t i = 0; i < 20; i++) {
        sentry__bgworker_submit(bgw, sleep_task, NULL, (void *)(i % 10));
    }

    sentry__bgworker_start(bgw);

    size_t dropped = 0;
    dropped = sentry__bgworker_foreach_matching(
        bgw, sleep_task, drop_lessthan, (void *)4);
    TEST_CHECK_INT_EQUAL(dropped, 8);
    dropped = sentry__bgworker_foreach_matching(
        bgw, sleep_task, drop_greaterthan, (void *)6);
    TEST_CHECK_INT_EQUAL(dropped, 6);

    int shutdown = sentry__bgworker_shutdown(bgw, 500);
    TEST_CHECK_INT_EQUAL(shutdown, 1);

    // submit another task to the worker which is still in shutdown
    bool executed_after_shutdown = false;
    sentry__bgworker_submit(bgw, trailing_task, NULL, &executed_after_shutdown);

    sentry_value_t list = sentry_value_new_list();
    dropped
        = sentry__bgworker_foreach_matching(bgw, sleep_task, collect, &list);
    TEST_CHECK_INT_EQUAL(dropped, 6);
    TEST_CHECK_JSON_VALUE(list, "[4,5,6,4,5,6]");
    sentry_value_decref(list);

    sentry__bgworker_decref(bgw);

    // wait for the trailing task to be finished
    sentry__mutex_lock(&executed_lock);
    sentry__cond_wait_timeout(&trailing_task_done, &executed_lock, 1000);
    TEST_CHECK(executed_after_shutdown);
}

SENTRY_TEST(bgworker_flush)
{
    sentry_bgworker_t *bgw = sentry__bgworker_new(NULL, NULL);
    TEST_ASSERT(!!bgw);
    sentry__bgworker_submit(bgw, sleep_task, NULL, NULL);

    sentry__bgworker_start(bgw);

    // first flush times out
    int flush = sentry__bgworker_flush(bgw, 500);
    TEST_CHECK_INT_EQUAL(flush, 1);

    // second flush succeeds
    flush = sentry__bgworker_flush(bgw, 1000);
    TEST_CHECK_INT_EQUAL(flush, 0);

    int shutdown = sentry__bgworker_shutdown(bgw, 500);
    TEST_CHECK_INT_EQUAL(shutdown, 0);
    sentry__bgworker_decref(bgw);
}
