#include "sentry_sync.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/**
 * Task locking and refcounts:
 *
 * The task list is arranged as an intrusive linked list. Whenever the list
 * itself is accessed or modified, the `task_lock` must be locked.
 * Additionally, each task has an atomic refcount, which indicates that the
 * task is _inside the list_ and _is currently being executed_.
 * That way, a task can be removed from the list, even though it is currently
 * being executed concurrently.
 */

struct sentry_bgworker_task_s;
typedef struct sentry_bgworker_task_s {
    struct sentry_bgworker_task_s *next_task;
    long refcount;
    sentry_task_function_t exec_func;
    sentry_task_function_t cleanup_func;
    void *data;
} sentry_bgworker_task_t;

struct sentry_bgworker_s {
    sentry_cond_t submit_signal;
    sentry_mutex_t submit_signal_lock;
    sentry_cond_t done_signal;
    sentry_mutex_t done_signal_lock;
    sentry_mutex_t task_lock;
    sentry_threadid_t thread_id;
    sentry_bgworker_task_t *first_task;
    sentry_bgworker_task_t *last_task;
    size_t task_count;
    bool running;
    bool free_self;
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
sentry__task_incref(sentry_bgworker_task_t *task)
{
    sentry__atomic_fetch_and_add(&task->refcount, 1);
}

static void
sentry__task_decref(sentry_bgworker_task_t *task)
{
    if (sentry__atomic_fetch_and_add(&task->refcount, -1) == 1) {
        if (task->cleanup_func) {
            task->cleanup_func(task->data);
        }
        sentry_free(task);
    }
}

static void
shutdown_task(void *data)
{
    sentry_bgworker_t *bgw = data;
    sentry__mutex_lock(&bgw->task_lock);
    bgw->running = false;
    sentry__mutex_unlock(&bgw->task_lock);
}

#ifdef _MSC_VER
#    define THREAD_FUNCTION_API __stdcall
#else
#    define THREAD_FUNCTION_API
#endif

#if defined(__MINGW32__) && !defined(__MINGW64__)
#    define UNSIGNED_MINGW unsigned
#else
#    define UNSIGNED_MINGW
#endif

// pthreads use `void *` return types, whereas windows uses `DWORD`
#ifdef SENTRY_PLATFORM_WINDOWS
static UNSIGNED_MINGW DWORD THREAD_FUNCTION_API
#else
static void *
#endif
worker_thread(void *data)
{
    sentry_bgworker_t *bgw = data;
    SENTRY_TRACE("background worker thread started");
    while (true) {
        sentry__mutex_lock(&bgw->task_lock);
        sentry_bgworker_task_t *task = bgw->first_task;
        bool is_done = !bgw->running && bgw->task_count == 0 && !task;
        if (task) {
            sentry__task_incref(task);
        }
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
            SENTRY_TRACE("executing task on worker thread");
            task->exec_func(task->data);
            sentry__task_decref(task);

            sentry__mutex_lock(&bgw->task_lock);
            if (bgw->first_task == task) {
                bgw->first_task = task->next_task;
                if (task == bgw->last_task) {
                    bgw->last_task = NULL;
                }
                sentry__task_decref(task);
                bgw->task_count--;
            }
            sentry__cond_wake(&bgw->done_signal);
            sentry__mutex_unlock(&bgw->task_lock);
        }
    }
    if (bgw->free_self) {
        sentry_free(bgw);
    }
    SENTRY_TRACE("background worker thread shut down");
    return 0;
}

void
sentry__bgworker_start(sentry_bgworker_t *bgw)
{
    SENTRY_TRACE("starting background worker thread");
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
    SENTRY_TRACE("shutting down background worker thread");
    assert(bgw->running);

    /* submit a task to shut down the queue */
    sentry__bgworker_submit(bgw, shutdown_task, NULL, bgw);

    uint64_t started = sentry__monotonic_time();
    while (true) {
        sentry__mutex_lock(&bgw->task_lock);
        bool done = bgw->task_count == 0 && !bgw->running;
        sentry__mutex_unlock(&bgw->task_lock);
        if (done) {
            sentry__thread_join(bgw->thread_id);
            return 0;
        }
        sentry__mutex_lock(&bgw->done_signal_lock);
        sentry__cond_wait_timeout(
            &bgw->done_signal, &bgw->done_signal_lock, 250);
        sentry__mutex_unlock(&bgw->done_signal_lock);
        uint64_t now = sentry__monotonic_time();
        if (now > started && now - started > timeout) {
            SENTRY_WARN(
                "background thread failed to shut down cleanly within timeout");
            return 1;
        }
    }
}

bool
sentry__bgworker_free(sentry_bgworker_t *bgw)
{
    if (!bgw) {
        return true;
    }

    sentry__mutex_lock(&bgw->task_lock);
    sentry_bgworker_task_t *task = bgw->first_task;
    while (task) {
        sentry_bgworker_task_t *next_task = task->next_task;
        sentry__task_decref(task);
        task = next_task;
    }
    bgw->first_task = NULL;
    bgw->last_task = NULL;
    bgw->task_count = 0;
    sentry__mutex_unlock(&bgw->task_lock);

    // When the thread did not shut down cleanly, we rather leak it than to risk
    // use-after-free
    bool is_shutdown = !bgw->running;
    if (is_shutdown) {
        sentry_free(bgw);
    } else {
        bgw->free_self = true;
    }
    return is_shutdown;
}

int
sentry__bgworker_submit(sentry_bgworker_t *bgw,
    sentry_task_function_t exec_func, sentry_task_function_t cleanup_func,
    void *data)
{
    assert(bgw->running);

    sentry_bgworker_task_t *task = SENTRY_MAKE(sentry_bgworker_task_t);
    if (!task) {
        return 1;
    }
    task->next_task = NULL;
    task->refcount = 1;
    task->exec_func = exec_func;
    task->cleanup_func = cleanup_func;
    task->data = data;

    SENTRY_TRACE("submitting task to background worker thread");
    sentry__mutex_lock(&bgw->task_lock);
    if (!bgw->first_task) {
        bgw->first_task = task;
    }
    if (bgw->last_task) {
        bgw->last_task->next_task = task;
    }
    bgw->last_task = task;
    bgw->task_count++;
    sentry__cond_wake(&bgw->submit_signal);
    sentry__mutex_unlock(&bgw->task_lock);

    return 0;
}

size_t
sentry__bgworker_foreach_matching(sentry_bgworker_t *bgw,
    sentry_task_function_t exec_func,
    bool (*callback)(void *task_data, void *data), void *data)
{
    sentry__mutex_lock(&bgw->task_lock);
    sentry_bgworker_task_t *task = bgw->first_task;
    sentry_bgworker_task_t *prev_task = NULL;

    size_t dropped = 0;

    while (task) {
        bool drop_task = false;
        // only consider tasks matching this exec_func
        if (task->exec_func == exec_func) {
            drop_task = callback(task->data, data);
        }

        sentry_bgworker_task_t *next_task = task->next_task;
        if (drop_task) {
            if (task == bgw->first_task) {
                bgw->first_task = next_task;
            }

            sentry__task_decref(task);
            bgw->task_count--;
            dropped++;
        } else {
            if (prev_task) {
                prev_task->next_task = task;
            }
            prev_task = task;
        }

        task = next_task;
    }

    bgw->last_task = prev_task;

    sentry__mutex_unlock(&bgw->task_lock);

    return dropped;
}

#ifdef SENTRY_PLATFORM_UNIX
#    include "sentry_unix_spinlock.h"

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
    sentry__block_for_signal_handler();
    g_signal_handling_thread = sentry__current_thread();
    __sync_fetch_and_or(&g_in_signal_handler, 1);
}

void
sentry__leave_signal_handler(void)
{
    __sync_fetch_and_and(&g_in_signal_handler, 0);
}
#endif
