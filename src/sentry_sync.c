#include "sentry_sync.h"
#include "sentry_alloc.h"
#include "sentry_utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct sentry_bgworker_task_s;
struct sentry_bgworker_task_s {
    sentry_task_function_t exec_func;
    sentry_task_function_t cleanup_func;
    void *data;
    struct sentry_bgworker_task_s *next_task;
    struct sentry_bgworker_task_s *prev_task;
};

struct sentry_bgworker_s {
    sentry_cond_t submit_signal;
    sentry_mutex_t submit_signal_lock;
    sentry_cond_t done_signal;
    sentry_mutex_t done_signal_lock;
    sentry_mutex_t task_lock;
    sentry_threadid_t thread_id;
    struct sentry_bgworker_task_s *first_task;
    struct sentry_bgworker_task_s *last_task;
    size_t task_count;
    bool running;
};

sentry_bgworker_t *
sentry__bgworker_new(void)
{
    sentry_bgworker_t *rv = SENTRY_MAKE(sentry_bgworker_t);
    if (!rv) {
        return NULL;
    }
    memset(rv, 0, sizeof(sentry_bgworker_t));
    sentry__mutex_init(&rv->submit_signal_lock);
    sentry__mutex_init(&rv->done_signal_lock);
    sentry__mutex_init(&rv->task_lock);
    sentry__cond_init(&rv->submit_signal);
    sentry__cond_init(&rv->done_signal);
    return rv;
}

static void
shutdown_task(void *data)
{
    sentry_bgworker_t *bgw = data;
    sentry__mutex_lock(&bgw->task_lock);
    bgw->running = false;
    sentry__mutex_unlock(&bgw->task_lock);
}

static int
worker_thread(void *data)
{
    sentry_bgworker_t *bgw = data;
    while (true) {
        struct sentry_bgworker_task_s *task = NULL;
        sentry__mutex_lock(&bgw->task_lock);
        if (bgw->first_task) {
            task = bgw->first_task;
            bgw->first_task = task->prev_task;
            if (task == bgw->last_task) {
                bgw->last_task = NULL;
            }
        }
        bool is_done = !bgw->running && bgw->task_count == 0 && !task;
        sentry__mutex_unlock(&bgw->task_lock);

        if (is_done) {
            sentry__cond_wake(&bgw->done_signal);
            break;
        } else if (!task) {
            sentry__mutex_lock(&bgw->submit_signal_lock);
            sentry__cond_wait_timeout(
                &bgw->submit_signal, &bgw->submit_signal_lock, 1000);
            sentry__mutex_unlock(&bgw->submit_signal_lock);
        } else {
            task->exec_func(task->data);
            if (task->cleanup_func) {
                task->cleanup_func(task->data);
            }
            sentry_free(task);

            sentry__mutex_lock(&bgw->task_lock);
            bgw->task_count--;
            sentry__cond_wake(&bgw->done_signal);
            sentry__mutex_unlock(&bgw->task_lock);
        }
    }
    return 0;
}

void
sentry__bgworker_start(sentry_bgworker_t *bgw)
{
    sentry__mutex_lock(&bgw->task_lock);
    bgw->running = true;
    if (sentry__thread_spawn(&bgw->thread_id, &worker_thread, bgw) != 0) {
        bgw->running = false;
    }
    sentry__mutex_unlock(&bgw->task_lock);
}

int
sentry__bgworker_shutdown(sentry_bgworker_t *bgw, uint64_t timeout)
{
    assert(bgw->running);

    /* submit a task to shut down the queue */
    sentry__bgworker_submit(bgw, shutdown_task, NULL, bgw);

    /* TODO: this is dangerous and should be monotonic time */
    uint64_t started = sentry__msec_time();
    while (true) {
        sentry__mutex_lock(&bgw->task_lock);
        bool done = bgw->task_count == 0 || !bgw->running;
        sentry__mutex_unlock(&bgw->task_lock);
        if (done) {
            sentry__thread_join(bgw->thread_id);
            return 0;
        }
        sentry__mutex_lock(&bgw->done_signal_lock);
        sentry__cond_wait_timeout(
            &bgw->done_signal, &bgw->done_signal_lock, 250);
        sentry__mutex_unlock(&bgw->done_signal_lock);
        uint64_t now = sentry__msec_time();
        if (now - started > timeout) {
            return 1;
        }
    }
}

void
sentry__bgworker_free(sentry_bgworker_t *bgw)
{
    if (!bgw) {
        return;
    }

    assert(!bgw->running && bgw->task_count == 0);

    struct sentry_bgworker_task_s *task = bgw->first_task;
    while (task) {
        struct sentry_bgworker_task_s *prev_task = task->prev_task;
        if (task->cleanup_func) {
            task->cleanup_func(task->data);
        }
        sentry_free(task);
        task = prev_task;
    }

    sentry_free(bgw);
}

int
sentry__bgworker_submit(sentry_bgworker_t *bgw,
    sentry_task_function_t exec_func, sentry_task_function_t cleanup_func,
    void *data)
{
    assert(bgw->running);

    struct sentry_bgworker_task_s *task
        = SENTRY_MAKE(struct sentry_bgworker_task_s);
    if (!task) {
        return 1;
    }

    task->exec_func = exec_func;
    task->cleanup_func = cleanup_func;
    task->data = data;

    sentry__mutex_lock(&bgw->task_lock);
    task->prev_task = NULL;
    task->next_task = bgw->last_task;
    if (!bgw->first_task) {
        bgw->first_task = task;
    }
    if (bgw->last_task) {
        bgw->last_task->prev_task = task;
    }
    bgw->last_task = task;
    bgw->task_count++;
    sentry__cond_wake(&bgw->submit_signal);
    sentry__mutex_unlock(&bgw->task_lock);

    return 0;
}

#ifdef SENTRY_PLATFORM_UNIX
#    include "unix/sentry_unix_spinlock.h"

static sig_atomic_t g_in_signal_handler = 0;
static sentry_threadid_t g_signal_handling_thread;

bool
sentry__block_for_signal_handler(void)
{
    while (__sync_fetch_and_add(&g_in_signal_handler, 0)) {
        if (sentry__threadid_equal(
                sentry__current_thread(), g_signal_handling_thread)) {
            return false;
        }
        sentry__cpu_relax();
    }
    return true;
}

void
sentry__enter_signal_handler(void)
{
    g_signal_handling_thread = sentry__current_thread();
    __sync_fetch_and_or(&g_in_signal_handler, 1);
}

void
sentry__leave_signal_handler(void)
{
    __sync_fetch_and_and(&g_in_signal_handler, 0);
}
#endif
