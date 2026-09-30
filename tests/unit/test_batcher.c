#include "sentry_batcher.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"
#include "sentry_value.h"

typedef struct {
    long started;
    long release;
} blocking_task_t;

typedef struct {
    sentry_batcher_t *batcher;
    long started;
    long completed;
} shutdown_task_t;

typedef struct {
    sentry_batcher_ref_t *ref;
    long completed;
} ref_shutdown_task_t;

typedef struct {
    long started;
    long release;
    long sent;
} blocking_transport_t;

typedef struct {
    sentry_batcher_t *batcher;
    long started;
    long completed;
} crash_flush_task_t;

static void
blocking_task_exec(void *data)
{
    blocking_task_t *task = data;
    sentry__atomic_store(&task->started, 1);
    while (!sentry__atomic_fetch(&task->release)) {
        sentry__thread_yield();
    }
}

SENTRY_THREAD_FN
shutdown_task_exec(void *data)
{
    shutdown_task_t *task = data;
    sentry__atomic_store(&task->started, 1);
    sentry__batcher_shutdown(task->batcher, 0);
    sentry__atomic_store(&task->completed, 1);
    return 0;
}

SENTRY_THREAD_FN
ref_shutdown_task_exec(void *data)
{
    ref_shutdown_task_t *task = data;
    sentry_batcher_t *batcher = sentry__batcher_swap(task->ref, NULL);
    if (batcher) {
        sentry__batcher_shutdown(batcher, 0);
        sentry__batcher_release(batcher);
    }
    sentry__atomic_store(&task->completed, 1);
    return 0;
}

SENTRY_THREAD_FN
crash_flush_task_exec(void *data)
{
    crash_flush_task_t *task = data;
    sentry__atomic_store(&task->started, 1);
    sentry__batcher_flush_crash_safe(task->batcher);
    sentry__atomic_store(&task->completed, 1);
    return 0;
}

static sentry_envelope_item_t *
pending_batch_func(sentry_envelope_t *envelope, sentry_value_t items)
{
    (void)items;
    return sentry__envelope_add_from_buffer(envelope, "{}", 2, "event");
}

static sentry_batcher_t *
new_batcher(sentry_batch_func_t batch_func, sentry_threadpool_t *pool)
{
    sentry_batcher_t *batcher = sentry__batcher_new(1, pool);
    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, batch_func);
    return batcher;
}

static long g_first_batch_calls;
static long g_second_batch_calls;

static sentry_envelope_item_t *
first_batch_func(sentry_envelope_t *envelope, sentry_value_t items)
{
    sentry__atomic_fetch_and_add(&g_first_batch_calls, 1);
    return pending_batch_func(envelope, items);
}

static sentry_envelope_item_t *
second_batch_func(sentry_envelope_t *envelope, sentry_value_t items)
{
    sentry__atomic_fetch_and_add(&g_second_batch_calls, 1);
    return pending_batch_func(envelope, items);
}

typedef struct {
    sentry_run_t *run;
    long calls;
    long first_started;
    long release_first;
    long worker_completed;
} crash_dump_test_t;

static crash_dump_test_t *g_crash_dump_test;

static sentry_envelope_item_t *
crash_dump_batch_func(sentry_envelope_t *envelope, sentry_value_t items)
{
    crash_dump_test_t *test = g_crash_dump_test;
    const long call = sentry__atomic_fetch_and_add(&test->calls, 1);
    if (call == 0) {
        sentry__atomic_store(&test->first_started, 1);
        while (!sentry__atomic_fetch(&test->release_first)) {
            sentry__thread_yield();
        }
    } else {
        sentry__atomic_store(&test->release_first, 1);
        const uint64_t deadline = sentry__monotonic_time() + 1000;
        while (!sentry__atomic_fetch(&test->run->retain)
            && sentry__monotonic_time() < deadline) {
            sentry__thread_yield();
        }
        sentry__atomic_store(
            &test->worker_completed, sentry__atomic_fetch(&test->run->retain));
    }
    return pending_batch_func(envelope, items);
}

static void
counting_transport_send(sentry_envelope_t *envelope, void *data)
{
    sentry__atomic_fetch_and_add((long *)data, 1);
    sentry_envelope_free(envelope);
}

static void
blocking_transport_send(sentry_envelope_t *envelope, void *data)
{
    blocking_transport_t *transport = data;
    sentry__atomic_store(&transport->started, 1);
    while (!sentry__atomic_fetch(&transport->release)) {
        sentry__thread_yield();
    }
    sentry__atomic_fetch_and_add(&transport->sent, 1);
    sentry_envelope_free(envelope);
}

static sentry_run_t *
new_test_run(const char *name, sentry_path_t **database_path)
{
    *database_path = sentry__path_from_str(name);
    TEST_ASSERT(!!*database_path);
    sentry__path_remove_all(*database_path);
    TEST_ASSERT(!sentry__path_create_dir_all(*database_path));
    sentry_run_t *run = sentry__run_new(*database_path);
    TEST_ASSERT(!!run);
    return run;
}

static void
free_test_run(sentry_run_t *run, sentry_path_t *database_path)
{
    sentry__run_clean(run, true);
    sentry__run_free(run);
    sentry__path_remove_all(database_path);
    sentry__path_free(database_path);
}

SENTRY_TEST(batcher_sync_flush_sends)
{
    sentry_batcher_t *batcher = sentry__batcher_new(2, NULL);
    TEST_ASSERT(!!batcher);
    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_TRACE_METRIC, pending_batch_func);
    sentry__batcher_set_queue(
        batcher, 1, SENTRY_DATA_CATEGORY_LOG_ITEM, pending_batch_func);
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-sync-flush", &database_path);
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);
    batcher->run = run;
    batcher->transport = transport;

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_TRACE_METRIC, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_flush(batcher, false));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 2);

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_TRACE_METRIC, sentry_value_new_null()));
    sentry__atomic_store(&batcher->queues[0].flushing, 1);
    TEST_CHECK(!sentry__batcher_flush(batcher, false));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 3);
    sentry__atomic_store(&batcher->queues[0].flushing, 0);
    TEST_CHECK(sentry__batcher_flush(batcher, false));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 4);

    sentry__batcher_release(batcher);
    sentry_transport_free(transport);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_flushes_requested)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-group", &database_path);
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);

    sentry_batcher_t *batcher = sentry__batcher_new(2, NULL);
    TEST_ASSERT(!!batcher);
    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, first_batch_func);
    sentry__batcher_set_queue(
        batcher, 1, SENTRY_DATA_CATEGORY_TRACE_METRIC, second_batch_func);
    sentry_options_t options = { .run = run };
    options.transport = transport;
    sentry__atomic_store(&g_first_batch_calls, 0);
    sentry__atomic_store(&g_second_batch_calls, 0);

    sentry__batcher_startup(batcher, &options);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&batcher->thread_state),
        SENTRY_BATCHER_THREAD_IDLE);
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_TRACE_METRIC, sentry_value_new_null()));
    sentry__batcher_wait_for_thread_startup(batcher);
    for (int i = 0; i < SENTRY_BATCHER_QUEUE_LENGTH; i++) {
        TEST_CHECK(sentry__batcher_enqueue(
            batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    }

    const uint64_t deadline = sentry__monotonic_time() + 1000;
    while (!sentry__atomic_fetch(&g_first_batch_calls)
        && sentry__monotonic_time() < deadline) {
        sentry__thread_yield();
    }
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_first_batch_calls), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_second_batch_calls), 0);

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    sentry__batcher_force_flush(batcher);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_first_batch_calls), 2);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_second_batch_calls), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 3);

    sentry__batcher_shutdown(batcher, 0);
    sentry__batcher_release(batcher);
    sentry_transport_free(transport);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_crash_flushes_all)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-group-crash", &database_path);
    sentry_batcher_t *batcher = sentry__batcher_new(2, NULL);
    TEST_ASSERT(!!batcher);
    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, first_batch_func);
    sentry__batcher_set_queue(
        batcher, 1, SENTRY_DATA_CATEGORY_TRACE_METRIC, second_batch_func);
    sentry_options_t options = { .run = run };
    sentry__atomic_store(&g_first_batch_calls, 0);
    sentry__atomic_store(&g_second_batch_calls, 0);

    sentry__batcher_startup(batcher, &options);
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_TRACE_METRIC, sentry_value_new_null()));
    sentry__batcher_wait_for_thread_startup(batcher);
    sentry__batcher_flush_crash_safe(batcher);

    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_first_batch_calls), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_second_batch_calls), 1);
    TEST_CHECK(sentry__atomic_fetch(&run->retain));

    sentry__batcher_shutdown(batcher, 0);
    sentry__batcher_release(batcher);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_enqueue_overflow)
{
    sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
    TEST_ASSERT(!!pool);
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);

    for (int i = 0;
        i < SENTRY_BATCHER_BUFFER_COUNT * SENTRY_BATCHER_QUEUE_LENGTH; i++) {
        TEST_CHECK(sentry__batcher_enqueue(
            batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    }
    TEST_CHECK(!sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));

    sentry__batcher_release(batcher);
    sentry__threadpool_free(pool);
}

SENTRY_TEST(batcher_force_flush_sends)
{
    sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
    TEST_ASSERT(!!pool);
    TEST_ASSERT(!sentry__threadpool_start(pool));
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-force-flush", &database_path);
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);
    batcher->run = run;
    batcher->transport = transport;

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    sentry__batcher_force_flush(batcher);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 1);

    sentry__batcher_release(batcher);
    sentry_transport_free(transport);
    sentry__threadpool_shutdown(pool);
    sentry__threadpool_free(pool);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_full_pool_discards)
{
    sentry_threadpool_t *pool = sentry__threadpool_new(1, 1);
    TEST_ASSERT(!!pool);
    TEST_ASSERT(sentry__threadpool_start(pool) == 0);

    // fill the pool to force rejection
    blocking_task_t task = { 0 };
    TEST_ASSERT(!sentry__threadpool_submit(
        pool, blocking_task_exec, NULL, NULL, &task));

    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-rejected-submit", &database_path);
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);
    batcher->run = run;
    batcher->transport = transport;
    sentry__client_report_reset();

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_flush(batcher, false));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 0);
    sentry_client_report_t report;
    TEST_CHECK(sentry__client_report_save(&report));
    TEST_CHECK_INT_EQUAL(report.counts[SENTRY_DISCARD_REASON_QUEUE_OVERFLOW]
                                      [SENTRY_DATA_CATEGORY_LOG_ITEM],
        1);

    sentry__batcher_release(batcher);
    sentry_transport_free(transport);
    sentry__atomic_store(&task.release, 1);
    sentry__threadpool_free(pool);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_shutdown_wait)
{
    {
        sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
        TEST_ASSERT(!!pool);
        TEST_ASSERT(!sentry__threadpool_start(pool));

        blocking_task_t blocking_task = { 0 };
        TEST_ASSERT(!sentry__threadpool_submit(
            pool, blocking_task_exec, NULL, NULL, &blocking_task));
        while (!sentry__atomic_fetch(&blocking_task.started)) {
            sentry__thread_yield();
        }

        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
        TEST_ASSERT(!!batcher);
        sentry__batcher_startup(batcher, options);
        sentry__batcher_wait_for_thread_startup(batcher);

        shutdown_task_t shutdown_task = { .batcher = batcher };
        sentry_threadid_t shutdown_thread;
        sentry__thread_init(&shutdown_thread);
        TEST_ASSERT(!sentry__thread_spawn(
            &shutdown_thread, shutdown_task_exec, &shutdown_task));
        while (!sentry__atomic_fetch(&shutdown_task.started)) {
            sentry__thread_yield();
        }

        const uint64_t deadline = sentry__monotonic_time() + 1000;
        while (!sentry__atomic_fetch(&shutdown_task.completed)
            && sentry__monotonic_time() < deadline) {
            sentry__thread_yield();
        }
        TEST_CHECK(sentry__atomic_fetch(&shutdown_task.completed));

        sentry__atomic_store(&blocking_task.release, 1);
        sentry__thread_join(shutdown_thread);
        sentry__thread_free(&shutdown_thread);
        sentry__threadpool_flush(pool);

        sentry__batcher_release(batcher);
        sentry_options_free(options);
        sentry__threadpool_shutdown(pool);
        sentry__threadpool_free(pool);
    }

    {
        sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
        TEST_ASSERT(!!pool);
        TEST_ASSERT(!sentry__threadpool_start(pool));

        sentry_path_t *database_path = NULL;
        sentry_run_t *run = new_test_run(
            SENTRY_TEST_PATH_PREFIX ".batcher-shutdown-wait", &database_path);
        blocking_transport_t blocking_transport = { 0 };
        sentry_transport_t *transport
            = sentry_transport_new(blocking_transport_send);
        TEST_ASSERT(!!transport);
        sentry_transport_set_state(transport, &blocking_transport);

        SENTRY_TEST_OPTIONS_NEW(options);
        options->run = run;
        sentry_options_set_transport(options, transport);

        sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
        TEST_ASSERT(!!batcher);
        sentry__batcher_startup(batcher, options);

        TEST_CHECK(sentry__batcher_enqueue(
            batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
        sentry__batcher_wait_for_thread_startup(batcher);
        TEST_CHECK(sentry__batcher_flush(batcher, false));
        while (!sentry__atomic_fetch(&blocking_transport.started)) {
            sentry__thread_yield();
        }

        shutdown_task_t shutdown_task = { .batcher = batcher };
        sentry_threadid_t shutdown_thread;
        sentry__thread_init(&shutdown_thread);
        TEST_ASSERT(!sentry__thread_spawn(
            &shutdown_thread, shutdown_task_exec, &shutdown_task));
        while (!sentry__atomic_fetch(&shutdown_task.started)) {
            sentry__thread_yield();
        }

        const uint64_t deadline = sentry__monotonic_time() + 100;
        while (!sentry__atomic_fetch(&shutdown_task.completed)
            && sentry__monotonic_time() < deadline) {
            sentry__thread_yield();
        }
        TEST_CHECK(!sentry__atomic_fetch(&shutdown_task.completed));

        sentry__atomic_store(&blocking_transport.release, 1);
        sentry__thread_join(shutdown_thread);
        sentry__thread_free(&shutdown_thread);
        sentry__threadpool_flush(pool);

        TEST_CHECK(sentry__atomic_fetch(&shutdown_task.completed));
        TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&blocking_transport.sent), 1);

        sentry__batcher_release(batcher);
        sentry__run_clean(run, true);
        sentry_options_free(options);
        sentry__path_remove_all(database_path);
        sentry__path_free(database_path);
        sentry__threadpool_shutdown(pool);
        sentry__threadpool_free(pool);
    }
}

SENTRY_TEST(batcher_crash_flush_buffers)
{
    sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
    TEST_ASSERT(!!pool);
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-crash-flush", &database_path);
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);
    batcher->run = run;
    sentry__atomic_store(
        &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_RUNNING);

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    sentry__batcher_flush_crash_safe(batcher);
    TEST_CHECK(sentry__atomic_fetch(&run->retain));

    sentry__batcher_release(batcher);
    sentry__threadpool_free(pool);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_crash_flush_stopped_buffers)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-stopped-crash-flush", &database_path);
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, NULL);
    TEST_ASSERT(!!batcher);
    batcher->run = run;

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    sentry__batcher_flush_crash_safe(batcher);
    TEST_CHECK(sentry__atomic_fetch(&run->retain));

    sentry__batcher_release(batcher);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_crash_flush_pending)
{
    sentry_path_t *database_path = sentry__path_from_str(
        SENTRY_TEST_PATH_PREFIX ".batcher-pending-crash-flush");
    TEST_ASSERT(!!database_path);
    sentry__path_remove_all(database_path);
    TEST_ASSERT(!sentry__path_create_dir_all(database_path));
    sentry_run_t *run = sentry__run_new(database_path);
    TEST_ASSERT(!!run);

    sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
    TEST_ASSERT(!!pool);
    TEST_ASSERT(!sentry__threadpool_start(pool));

    blocking_task_t blocking_task = { 0 };
    TEST_ASSERT(!sentry__threadpool_submit(
        pool, blocking_task_exec, NULL, NULL, &blocking_task));
    while (!sentry__atomic_fetch(&blocking_task.started)) {
        sentry__thread_yield();
    }

    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);
    batcher->run = run;
    batcher->transport = transport;

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_flush(batcher, false));
    TEST_CHECK(!sentry__atomic_fetch(&run->retain));

    sentry__batcher_flush_crash_safe(batcher);
    TEST_CHECK(sentry__atomic_fetch(&run->retain));

    sentry__atomic_store(&blocking_task.release, 1);
    sentry__threadpool_flush(pool);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 0);
    sentry__threadpool_shutdown(pool);
    sentry__threadpool_free(pool);
    sentry__batcher_release(batcher);
    sentry_transport_free(transport);
    sentry__run_clean(run, true);
    sentry__run_free(run);
    sentry__path_remove_all(database_path);
    sentry__path_free(database_path);
}

SENTRY_TEST(batcher_shutdown_waits_after_crash_flush)
{
    sentry_path_t *database_path = sentry__path_from_str(
        SENTRY_TEST_PATH_PREFIX ".batcher-shutdown-after-crash-flush");
    TEST_ASSERT(!!database_path);
    sentry__path_remove_all(database_path);
    TEST_ASSERT(!sentry__path_create_dir_all(database_path));
    sentry_run_t *run = sentry__run_new(database_path);
    TEST_ASSERT(!!run);

    sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
    TEST_ASSERT(!!pool);
    TEST_ASSERT(!sentry__threadpool_start(pool));

    blocking_task_t blocking_task = { 0 };
    TEST_ASSERT(!sentry__threadpool_submit(
        pool, blocking_task_exec, NULL, NULL, &blocking_task));
    while (!sentry__atomic_fetch(&blocking_task.started)) {
        sentry__thread_yield();
    }

    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);

    SENTRY_TEST_OPTIONS_NEW(options);
    options->run = run;
    sentry_options_set_transport(options, transport);

    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);
    sentry__batcher_startup(batcher, options);

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    sentry__batcher_wait_for_thread_startup(batcher);
    TEST_CHECK(sentry__batcher_flush(batcher, false));
    TEST_CHECK(!sentry__atomic_fetch(&run->retain));

    sentry__batcher_flush_crash_safe(batcher);
    TEST_CHECK(sentry__atomic_fetch(&run->retain));

    shutdown_task_t shutdown_task = { .batcher = batcher };
    sentry_threadid_t shutdown_thread;
    sentry__thread_init(&shutdown_thread);
    TEST_ASSERT(!sentry__thread_spawn(
        &shutdown_thread, shutdown_task_exec, &shutdown_task));
    while (!sentry__atomic_fetch(&shutdown_task.started)) {
        sentry__thread_yield();
    }

    const uint64_t deadline = sentry__monotonic_time() + 100;
    while (!sentry__atomic_fetch(&shutdown_task.completed)
        && sentry__monotonic_time() < deadline) {
        sentry__thread_yield();
    }
    TEST_CHECK(!sentry__atomic_fetch(&shutdown_task.completed));

    sentry__atomic_store(&blocking_task.release, 1);
    sentry__thread_join(shutdown_thread);
    sentry__thread_free(&shutdown_thread);
    sentry__threadpool_flush(pool);

    TEST_CHECK(sentry__atomic_fetch(&shutdown_task.completed));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 0);

    sentry__batcher_release(batcher);
    sentry__run_clean(run, true);
    sentry_options_free(options);
    sentry__path_remove_all(database_path);
    sentry__path_free(database_path);
    sentry__threadpool_shutdown(pool);
    sentry__threadpool_free(pool);
}

SENTRY_TEST(batcher_crash_flush_completing)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(SENTRY_TEST_PATH_PREFIX
        ".batcher-completing-crash-flush",
        &database_path);
    sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
    TEST_ASSERT(!!pool);
    TEST_ASSERT(!sentry__threadpool_start(pool));

    blocking_transport_t transport_state = { 0 };
    sentry_transport_t *transport
        = sentry_transport_new(blocking_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &transport_state);

    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);
    batcher->run = run;
    batcher->transport = transport;

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_flush(batcher, false));
    while (!sentry__atomic_fetch(&transport_state.started)) {
        sentry__thread_yield();
    }

    crash_flush_task_t crash_flush_task = { .batcher = batcher };
    sentry_threadid_t crash_flush_thread;
    sentry__thread_init(&crash_flush_thread);
    TEST_ASSERT(!sentry__thread_spawn(
        &crash_flush_thread, crash_flush_task_exec, &crash_flush_task));
    while (!sentry__atomic_fetch(&crash_flush_task.started)) {
        sentry__thread_yield();
    }

    const uint64_t deadline = sentry__monotonic_time() + 100;
    while (!sentry__atomic_fetch(&crash_flush_task.completed)
        && sentry__monotonic_time() < deadline) {
        sentry__thread_yield();
    }
    TEST_CHECK(!sentry__atomic_fetch(&crash_flush_task.completed));

    sentry__atomic_store(&transport_state.release, 1);
    sentry__thread_join(crash_flush_thread);
    sentry__thread_free(&crash_flush_thread);
    sentry__threadpool_flush(pool);
    TEST_CHECK(sentry__atomic_fetch(&crash_flush_task.completed));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&transport_state.sent), 1);

    sentry__threadpool_shutdown(pool);
    sentry__threadpool_free(pool);
    sentry__batcher_release(batcher);
    sentry_transport_free(transport);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_crash_dump_does_not_block_workers)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-crash-dump-workers", &database_path);
    sentry_threadpool_t *pool = sentry__threadpool_new(1, 10);
    TEST_ASSERT(!!pool);
    TEST_ASSERT(!sentry__threadpool_start(pool));

    crash_dump_test_t test = { .run = run };
    g_crash_dump_test = &test;
    sentry_batcher_t *batcher = new_batcher(crash_dump_batch_func, pool);
    TEST_ASSERT(!!batcher);
    batcher->run = run;

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_flush(batcher, false));
    while (!sentry__atomic_fetch(&test.first_started)) {
        sentry__thread_yield();
    }
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_flush(batcher, false));

    sentry__batcher_flush_crash_safe(batcher);
    TEST_CHECK(sentry__atomic_fetch(&test.worker_completed));

    sentry__threadpool_flush(pool);
    g_crash_dump_test = NULL;
    sentry__threadpool_shutdown(pool);
    sentry__threadpool_free(pool);
    sentry__batcher_release(batcher);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_crash_flush_after_release)
{
    // the crash thread pins the batcher before sentry_close() swaps the global
    // reference, so shutdown must not release it until crash flushing finishes.
    sentry_batcher_ref_t ref = SENTRY_BATCHER_REF_INIT;
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, NULL);
    TEST_ASSERT(!!batcher);

    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-crash-after-release", &database_path);
    batcher->run = run;

    sentry__batcher_swap(&ref, batcher);

    // crash thread: pin the global reference without taking its spinlock
    sentry_batcher_t *pinned = sentry__batcher_pin(&ref);
    TEST_CHECK(pinned == batcher);

    // shutdown thread: sentry__logs_shutdown() waits for the pin before release
    ref_shutdown_task_t task = { .ref = &ref };
    sentry_threadid_t shutdown_thread;
    sentry__thread_init(&shutdown_thread);
    TEST_ASSERT(
        !sentry__thread_spawn(&shutdown_thread, ref_shutdown_task_exec, &task));
    while (true) {
        sentry_batcher_t *current = sentry__batcher_pin(&ref);
        if (!current) {
            break;
        }
        sentry__batcher_unpin(&ref);
        sentry__thread_yield();
    }
    TEST_CHECK(!sentry__atomic_fetch(&task.completed));

    // crash thread can safely dereference the pinned batcher
    sentry__batcher_flush_crash_safe(pinned);
    sentry__batcher_unpin(&ref);

    sentry__thread_join(shutdown_thread);
    sentry__thread_free(&shutdown_thread);
    TEST_CHECK(sentry__atomic_fetch(&task.completed));
    free_test_run(run, database_path);
}

static void
noop_task_exec(void *UNUSED(data))
{
}

typedef struct {
    sentry_batcher_t *batcher;
    long ready;
    long go;
    long enqueued;
} first_use_task_t;

SENTRY_THREAD_FN
first_enqueue_exec(void *data)
{
    first_use_task_t *task = data;
    const long index = sentry__atomic_fetch_and_add(&task->ready, 1);
    while (!sentry__atomic_fetch(&task->go)) {
        sentry__thread_yield();
    }
    const sentry_data_category_t category
        = task->batcher->queues[(size_t)index % task->batcher->num_queues]
              .data_category;
    if (sentry__batcher_enqueue(
            task->batcher, category, sentry_value_new_null())) {
        sentry__atomic_fetch_and_add(&task->enqueued, 1);
    }
    return 0;
}

static void
check_concurrent_start(bool shutdown, size_t num_queues)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-first-use", &database_path);
    SENTRY_TEST_OPTIONS_NEW(options);
    options->run = run;
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    sentry_transport_set_state(transport, &sent);
    sentry_options_set_transport(options, transport);

    for (int iteration = 0; iteration < 50; iteration++) {
        sentry_threadpool_t *pool = sentry__threadpool_new(2, 10);
        TEST_ASSERT(!!pool);
        sentry_batcher_t *batcher = sentry__batcher_new(num_queues, pool);
        TEST_ASSERT(!!batcher);
        sentry__batcher_set_queue(
            batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, pending_batch_func);
        if (num_queues > 1) {
            sentry__batcher_set_queue(batcher, 1,
                SENTRY_DATA_CATEGORY_TRACE_METRIC, pending_batch_func);
        }
        sentry__batcher_startup(batcher, options);
        first_use_task_t task = { .batcher = batcher };
        sentry_threadid_t threads[4];
        for (size_t i = 0; i < 4; i++) {
            sentry__thread_init(&threads[i]);
            TEST_ASSERT(
                !sentry__thread_spawn(&threads[i], first_enqueue_exec, &task));
        }
        while (sentry__atomic_fetch(&task.ready) != 4) {
            sentry__thread_yield();
        }
        sentry__atomic_store(&task.go, 1);
        if (shutdown) {
            sentry__batcher_shutdown(batcher, 0);
            sentry__threadpool_free(pool);
        }
        for (size_t i = 0; i < 4; i++) {
            sentry__thread_join(threads[i]);
            sentry__thread_free(&threads[i]);
        }
        TEST_CHECK_INT_EQUAL(task.enqueued, 4);
        if (!shutdown) {
            sentry__batcher_wait_for_thread_startup(batcher);
            TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&batcher->thread_state),
                SENTRY_BATCHER_THREAD_RUNNING);
            sentry__batcher_shutdown(batcher, 0);
            sentry__threadpool_free(pool);
            TEST_CHECK_INT_EQUAL(
                sentry__atomic_fetch(&sent), (iteration + 1) * num_queues);
        }
        TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&batcher->thread_state),
            SENTRY_BATCHER_THREAD_STOPPED);
        sentry__batcher_release(batcher);
    }

    sentry__run_clean(run, true);
    sentry_options_free(options);
    sentry__path_remove_all(database_path);
    sentry__path_free(database_path);
}

SENTRY_TEST(batcher_concurrent_start) { check_concurrent_start(false, 1); }

SENTRY_TEST(batcher_start_during_shutdown) { check_concurrent_start(true, 1); }

SENTRY_TEST(batcher_crash_before_start)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-crash-before-start", &database_path);
    sentry_threadpool_t *pool = sentry__threadpool_new(2, 10);
    TEST_ASSERT(!!pool);
    SENTRY_TEST_OPTIONS_NEW(options);
    options->run = run;
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, pool);
    TEST_ASSERT(!!batcher);
    sentry__batcher_startup(batcher, options);

    sentry__batcher_flush_crash_safe(batcher);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&batcher->thread_state),
        SENTRY_BATCHER_THREAD_STOPPED);
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    sentry__batcher_flush_crash_safe(batcher);
    TEST_CHECK(sentry__atomic_fetch(&run->retain));
    sentry__batcher_shutdown(batcher, 0);
    TEST_CHECK(
        sentry__threadpool_submit(pool, noop_task_exec, NULL, NULL, NULL) != 0);

    sentry__batcher_release(batcher);
    sentry__threadpool_free(pool);
    sentry__run_clean(run, true);
    sentry_options_free(options);
    sentry__path_remove_all(database_path);
    sentry__path_free(database_path);
}

SENTRY_TEST(batcher_crash_during_start)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-crash-during-start", &database_path);
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, NULL);
    TEST_ASSERT(!!batcher);
    batcher->run = run;
    sentry__atomic_store(
        &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_SPAWNING);
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));

    crash_flush_task_t task = { .batcher = batcher };
    sentry_threadid_t thread;
    sentry__thread_init(&thread);
    TEST_ASSERT(
        sentry__thread_spawn(&thread, crash_flush_task_exec, &task) == 0);
    const uint64_t deadline = sentry__monotonic_time() + 1000;
    while (!sentry__atomic_fetch(&task.completed)
        && sentry__monotonic_time() < deadline) {
        sentry__thread_yield();
    }
    TEST_CHECK(sentry__atomic_fetch(&task.completed));
    TEST_CHECK(sentry__atomic_fetch(&run->retain));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&batcher->thread_state),
        SENTRY_BATCHER_THREAD_SPAWNING);

    shutdown_task_t shutdown_task = { .batcher = batcher };
    sentry_threadid_t shutdown_thread;
    sentry__thread_init(&shutdown_thread);
    TEST_ASSERT(!sentry__thread_spawn(
        &shutdown_thread, shutdown_task_exec, &shutdown_task));
    const uint64_t shutdown_deadline = sentry__monotonic_time() + 1000;
    while (!sentry__atomic_fetch(&shutdown_task.completed)
        && sentry__monotonic_time() < shutdown_deadline) {
        sentry__thread_yield();
    }
    TEST_CHECK(sentry__atomic_fetch(&shutdown_task.completed));
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&batcher->thread_state),
        SENTRY_BATCHER_THREAD_STOPPED);

    // allow a blocked flush or shutdown to finish before joining
    sentry__atomic_store(
        &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_STOPPED);
    sentry__thread_join(shutdown_thread);
    sentry__thread_free(&shutdown_thread);
    sentry__thread_join(thread);
    sentry__thread_free(&thread);
    sentry__batcher_release(batcher);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_shutdown_flushes_all)
{
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-shutdown-all", &database_path);
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);
    sentry_batcher_t *batcher = sentry__batcher_new(2, NULL);
    TEST_ASSERT(!!batcher);
    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, first_batch_func);
    sentry__batcher_set_queue(
        batcher, 1, SENTRY_DATA_CATEGORY_TRACE_METRIC, second_batch_func);
    const sentry_options_t options = { .run = run, .transport = transport };
    sentry__batcher_startup(batcher, &options);
    sentry__atomic_store(&g_first_batch_calls, 0);
    sentry__atomic_store(&g_second_batch_calls, 0);

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_TRACE_METRIC, sentry_value_new_null()));
    sentry__batcher_shutdown(batcher, 0);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_first_batch_calls), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_second_batch_calls), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 2);

    sentry__batcher_release(batcher);
    sentry_transport_free(transport);
    free_test_run(run, database_path);
}

SENTRY_TEST(batcher_queues_start_together) { check_concurrent_start(false, 2); }

SENTRY_TEST(batcher_queues_start_during_shutdown)
{
    check_concurrent_start(true, 2);
}

SENTRY_TEST(batcher_queues_overflow_independently)
{
    sentry_batcher_t *batcher = sentry__batcher_new(2, NULL);
    TEST_ASSERT(!!batcher);
    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, pending_batch_func);
    sentry__batcher_set_queue(
        batcher, 1, SENTRY_DATA_CATEGORY_TRACE_METRIC, pending_batch_func);

    for (size_t i = 0; i < 2; i++) {
        const sentry_data_category_t category
            = batcher->queues[i].data_category;
        for (int j = 0;
            j < SENTRY_BATCHER_BUFFER_COUNT * SENTRY_BATCHER_QUEUE_LENGTH;
            j++) {
            TEST_CHECK(sentry__batcher_enqueue(
                batcher, category, sentry_value_new_null()));
        }
        TEST_CHECK(!sentry__batcher_enqueue(
            batcher, category, sentry_value_new_null()));
    }

    sentry__batcher_release(batcher);
}

SENTRY_TEST(batcher_unconfigured_category)
{
    sentry_batcher_t *batcher = new_batcher(pending_batch_func, NULL);
    TEST_ASSERT(!!batcher);
    sentry_value_t item = sentry_value_new_string("metric");

    TEST_CHECK(!sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_TRACE_METRIC, item));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(item), "metric");

    sentry_value_decref(item);
    sentry__batcher_release(batcher);
}

static blocking_task_t *g_blocking_batch;

static sentry_envelope_item_t *
blocking_batch_func(sentry_envelope_t *envelope, sentry_value_t items)
{
    sentry__atomic_fetch_and_add(&g_first_batch_calls, 1);
    blocking_task_exec(g_blocking_batch);
    return pending_batch_func(envelope, items);
}

SENTRY_TEST(batcher_full_pool_does_not_block)
{
    sentry_threadpool_t *pool = sentry__threadpool_new(1, 1);
    TEST_ASSERT(!!pool);
    sentry_path_t *database_path = NULL;
    sentry_run_t *run = new_test_run(
        SENTRY_TEST_PATH_PREFIX ".batcher-full-pool", &database_path);
    long sent = 0;
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent);
    sentry_batcher_t *batcher = sentry__batcher_new(2, pool);
    TEST_ASSERT(!!batcher);
    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, blocking_batch_func);
    sentry__batcher_set_queue(
        batcher, 1, SENTRY_DATA_CATEGORY_TRACE_METRIC, second_batch_func);
    const sentry_options_t options = { .run = run, .transport = transport };
    blocking_task_t task = { 0 };
    g_blocking_batch = &task;
    sentry__atomic_store(&g_first_batch_calls, 0);
    sentry__atomic_store(&g_second_batch_calls, 0);
    sentry__client_report_reset();
    sentry__batcher_startup(batcher, &options);

    for (int i = 0; i < SENTRY_BATCHER_QUEUE_LENGTH; i++) {
        TEST_CHECK(sentry__batcher_enqueue(
            batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    }
    uint64_t deadline = sentry__monotonic_time() + 1000;
    while (!sentry__atomic_fetch(&task.started)
        && sentry__monotonic_time() < deadline) {
        sentry__thread_yield();
    }
    TEST_CHECK(sentry__atomic_fetch(&task.started));

    for (int i = 0; i < SENTRY_BATCHER_QUEUE_LENGTH; i++) {
        TEST_CHECK(sentry__batcher_enqueue(
            batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
        TEST_CHECK(sentry__batcher_enqueue(batcher,
            SENTRY_DATA_CATEGORY_TRACE_METRIC, sentry_value_new_null()));
    }

    long logs = 0;
    long metrics = 0;
    deadline = sentry__monotonic_time() + 1000;
    while ((logs < SENTRY_BATCHER_QUEUE_LENGTH
               || metrics < SENTRY_BATCHER_QUEUE_LENGTH)
        && sentry__monotonic_time() < deadline) {
        sentry_client_report_t report;
        sentry__client_report_save(&report);
        logs += report.counts[SENTRY_DISCARD_REASON_QUEUE_OVERFLOW]
                             [SENTRY_DATA_CATEGORY_LOG_ITEM];
        metrics += report.counts[SENTRY_DISCARD_REASON_QUEUE_OVERFLOW]
                                [SENTRY_DATA_CATEGORY_TRACE_METRIC];
        sentry__thread_yield();
    }
    TEST_CHECK_INT_EQUAL(logs, SENTRY_BATCHER_QUEUE_LENGTH);
    TEST_CHECK_INT_EQUAL(metrics, SENTRY_BATCHER_QUEUE_LENGTH);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_first_batch_calls), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_second_batch_calls), 0);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 0);

    sentry__atomic_store(&task.release, 1);
    sentry__batcher_force_flush(batcher);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 1);

    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry_value_new_null()));
    sentry__batcher_force_flush(batcher);
    TEST_CHECK(sentry__batcher_enqueue(
        batcher, SENTRY_DATA_CATEGORY_TRACE_METRIC, sentry_value_new_null()));
    sentry__batcher_force_flush(batcher);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_first_batch_calls), 2);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&g_second_batch_calls), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&sent), 3);

    sentry__batcher_shutdown(batcher, 0);
    sentry__batcher_release(batcher);
    sentry__threadpool_free(pool);
    sentry_transport_free(transport);
    free_test_run(run, database_path);
    g_blocking_batch = NULL;
}
