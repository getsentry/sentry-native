#include "../src/sentry_sync.h"
#include "sentry_testsupport.h"

struct task_state {
    int executed;
    bool running;
};

static void
task_func(void *data)
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
        sentry_bgworker_t *bgw = sentry__bgworker_new();
        assert_true(bgw);

        sentry__bgworker_start(bgw);

        struct task_state ts;
        ts.executed = false;
        ts.running = true;
        for (size_t j = 0; j < 10; j++) {
            sentry__bgworker_submit(bgw, task_func, cleanup_func, &ts);
        }

        assert_int_equal(sentry__bgworker_shutdown(bgw, 5000), 0);
        sentry__bgworker_free(bgw);

        __sync_synchronize();
        assert_int_equal(ts.executed, 10);
        assert_false(ts.running);
    }
}