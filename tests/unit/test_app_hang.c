#include "sentry_app_hang_latch.h"
#include "sentry_app_hang_monitor.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"
#include "sentry_thread_stackwalk.h"
#include "sentry_utils.h"

#if defined(SENTRY_PLATFORM_WINDOWS)
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    include <pthread.h>
#    include <signal.h>
#    include <sys/syscall.h>
#    include <unistd.h>
#endif

SENTRY_TEST(app_hang_should_capture)
{
    // disabled (timeout == 0) -> never captures
    TEST_CHECK(!sentry__app_hang_should_capture(100, 100000, 0, 0));
    // never heartbeated (hb == 0) -> no capture
    TEST_CHECK(!sentry__app_hang_should_capture(0, 100000, 2000, 0));
    // fresh heartbeat (within timeout) -> no capture
    TEST_CHECK(!sentry__app_hang_should_capture(99000, 100000, 2000, 0));
    // stale (now - hb >= timeout) -> capture
    TEST_CHECK(sentry__app_hang_should_capture(98000, 100000, 2000, 0));
    // already fired for this hb -> cooldown, no capture
    TEST_CHECK(!sentry__app_hang_should_capture(98000, 100000, 2000, 98000));
}

SENTRY_TEST(app_hang_latch)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    sentry__app_hang_latch_reset();
    sentry__app_hang_set_active(true);
    sentry_app_hang_latch_t l = sentry__app_hang_current_latch();
    TEST_CHECK(l.target_tid == 0);
    TEST_CHECK(l.last_heartbeat_ms == 0);

    // first heartbeat latches the calling thread + records a timestamp
    sentry_app_hang_heartbeat();
    l = sentry__app_hang_current_latch();
    TEST_CHECK(l.target_tid == sentry__app_hang_current_tid());
    TEST_CHECK(l.target_tid != 0);
    uint64_t first = l.last_heartbeat_ms;
    TEST_CHECK(first != 0);

    sentry__app_hang_latch_reset();
    l = sentry__app_hang_current_latch();
    TEST_CHECK(l.target_tid == 0);
    sentry__app_hang_set_active(false);
}

SENTRY_TEST(app_hang_pause_resumes_on_heartbeat)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    sentry__app_hang_latch_reset();
    sentry__app_hang_set_active(true);
    sentry_app_hang_heartbeat();

    sentry_app_hang_latch_t l = sentry__app_hang_current_latch();
    uint64_t target = l.target_tid;
    uint64_t first_heartbeat = l.last_heartbeat_ms;
    TEST_CHECK(target != 0);
    TEST_CHECK(first_heartbeat != 0);

    sentry_app_hang_pause();
    l = sentry__app_hang_current_latch();
    TEST_CHECK(sentry__app_hang_is_paused());
    TEST_CHECK(l.target_tid == target);
    TEST_CHECK(l.last_heartbeat_ms == first_heartbeat);

    sleep_ms(1);
    sentry_app_hang_heartbeat();
    l = sentry__app_hang_current_latch();
    TEST_CHECK(!sentry__app_hang_is_paused());
    TEST_CHECK(l.target_tid == target);
    TEST_CHECK(l.last_heartbeat_ms > first_heartbeat);

    sentry__app_hang_latch_reset();
    sentry__app_hang_set_active(false);
}

SENTRY_TEST(app_hang_make_event)
{
    void *ips[2] = { (void *)0x1000, (void *)0x2000 };
    sentry_value_t ev = sentry__app_hang_make_event(ips, 2, 5000);

    sentry_value_t message = sentry_value_get_by_key(ev, "message");
    TEST_CHECK(sentry_value_get_type(message) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(message, "formatted")),
        "App hung for at least 5000 ms.");

    sentry_value_t exc = sentry_value_get_by_index(
        sentry_value_get_by_key(
            sentry_value_get_by_key(ev, "exception"), "values"),
        0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(exc, "type")),
        "AppHang");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(
            sentry_value_get_by_key(exc, "mechanism"), "type")),
        "AppHang");
    TEST_CHECK(sentry_value_is_true(sentry_value_get_by_key(
        sentry_value_get_by_key(exc, "mechanism"), "handled")));
    sentry_value_t frames = sentry_value_get_by_key(
        sentry_value_get_by_key(exc, "stacktrace"), "frames");
    TEST_CHECK(sentry_value_get_length(frames) == 2);

    sentry_value_decref(ev);
}

static long g_app_hang_seen;
static char g_app_hang_type[32];

static size_t
fake_stackwalk(uint64_t tid, void **ips, size_t max)
{
    (void)tid;
    if (max < 2) {
        return 0;
    }
    ips[0] = (void *)0x4000;
    ips[1] = (void *)0x5000;
    return 2;
}

static sentry_value_t
capture_before_send(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    (void)hint;
    (void)data;
    sentry_value_t exc = sentry_value_get_by_index(
        sentry_value_get_by_key(
            sentry_value_get_by_key(event, "exception"), "values"),
        0);
    const char *type
        = sentry_value_as_string(sentry_value_get_by_key(exc, "type"));
    if (type) {
        strncpy(g_app_hang_type, type, sizeof(g_app_hang_type) - 1);
    }
    sentry__atomic_store(&g_app_hang_seen, 1);
    sentry_value_decref(event);
    return sentry_value_new_null();
}

SENTRY_TEST(app_hang_monitor_fires)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    g_app_hang_seen = 0;
    g_app_hang_type[0] = '\0';
    sentry__app_hang_latch_reset();
    sentry__app_hang_monitor_set_stackwalk_fn(fake_stackwalk);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send(options, capture_before_send, NULL);
    sentry_options_set_enable_app_hang_tracking(options, 1);
    sentry_options_set_app_hang_timeout(options, 50);
    sentry_init(options);

    sentry_app_hang_heartbeat();

    for (int i = 0; i < 300 && !sentry__atomic_fetch(&g_app_hang_seen); i++) {
        sleep_ms(10);
    }

    TEST_CHECK(sentry__atomic_fetch(&g_app_hang_seen) == 1);
    TEST_CHECK_STRING_EQUAL(g_app_hang_type, "AppHang");

    sentry_close();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL);
}

SENTRY_TEST(app_hang_pause_prevents_capture)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    g_app_hang_seen = 0;
    g_app_hang_type[0] = '\0';
    sentry__app_hang_latch_reset();
    sentry__app_hang_monitor_set_stackwalk_fn(fake_stackwalk);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send(options, capture_before_send, NULL);
    sentry_options_set_enable_app_hang_tracking(options, 1);
    sentry_options_set_app_hang_timeout(options, 50);
    sentry_init(options);

    sentry_app_hang_heartbeat();
    sentry_app_hang_pause();

    // Wait beyond the timeout and several poll cycles while the worker is
    // paused.
    sleep_ms(100);
    TEST_CHECK(sentry__atomic_fetch(&g_app_hang_seen) == 0);

    sentry_app_hang_heartbeat();
    for (int i = 0; i < 300 && !sentry__atomic_fetch(&g_app_hang_seen); i++) {
        sleep_ms(10);
    }

    TEST_CHECK(sentry__atomic_fetch(&g_app_hang_seen) == 1);
    TEST_CHECK_STRING_EQUAL(g_app_hang_type, "AppHang");

    sentry_close();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL);
}

SENTRY_TEST(app_hang_disarm_prevents_capture)
{
    // Mirrors app_hang_monitor_fires, but disarms after latching. This is the
    // crash-handler path: once disarmed, the watchdog must not capture an
    // app-hang even though the latched thread stops heart-beating (so a crash
    // is never also reported as an app-hang).
    g_app_hang_seen = 0;
    g_app_hang_type[0] = '\0';
    sentry__app_hang_latch_reset();
    sentry__app_hang_monitor_set_stackwalk_fn(fake_stackwalk);

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send(options, capture_before_send, NULL);
    sentry_options_set_enable_app_hang_tracking(options, 1);
    sentry_options_set_app_hang_timeout(options, 50);
    sentry_init(options);

    sentry_app_hang_heartbeat();
    // Simulate entering the crash handler: disable -> never heartbeat again.
    sentry__app_hang_set_active(false);

    // Wait well past several timeout/poll cycles to be sure nothing fires.
    for (int i = 0; i < 50; i++) {
        sleep_ms(10);
    }

    TEST_CHECK(sentry__atomic_fetch(&g_app_hang_seen) == 0);

    sentry_close();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL);
}

static long g_real_seen;
static long g_real_frames;
static volatile long g_keep_spinning;

static sentry_value_t
real_before_send(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    (void)hint;
    (void)data;
    sentry_value_t exc = sentry_value_get_by_index(
        sentry_value_get_by_key(
            sentry_value_get_by_key(event, "exception"), "values"),
        0);
    sentry_value_t frames = sentry_value_get_by_key(
        sentry_value_get_by_key(exc, "stacktrace"), "frames");
    sentry__atomic_store(&g_real_frames, (long)sentry_value_get_length(frames));
    sentry__atomic_store(&g_real_seen, 1);
    sentry_value_decref(event);
    return sentry_value_new_null();
}

SENTRY_THREAD_FN
spinner(void *arg)
{
    (void)arg;
    sentry_app_hang_heartbeat(); // latch this thread
    while (sentry__atomic_fetch(&g_keep_spinning)) {
        // busy-wait: alive & sampleable but never heartbeats again -> hung
        volatile int x = 0;
        for (int i = 0; i < 100000; i++) {
            x += i;
        }
    }
    return 0;
}

SENTRY_TEST(app_hang_end_to_end)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    g_real_seen = 0;
    g_real_frames = 0;
    sentry__atomic_store(&g_keep_spinning, 1);
    sentry__app_hang_latch_reset();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL); // use the REAL stackwalker

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send(options, real_before_send, NULL);
    sentry_options_set_enable_app_hang_tracking(options, 1);
    sentry_options_set_app_hang_timeout(options, 50);
    sentry_init(options);

    sentry_threadid_t t;
    sentry__thread_spawn(&t, spinner, NULL);

    for (int i = 0; i < 500 && !sentry__atomic_fetch(&g_real_seen); i++) {
        sleep_ms(10);
    }

    sentry__atomic_store(&g_keep_spinning, 0);
    sentry__thread_join(t);

    TEST_CHECK(sentry__atomic_fetch(&g_real_seen) == 1);
    TEST_CHECK(sentry__atomic_fetch(&g_real_frames) > 0);

    sentry_close();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL);
}

SENTRY_TEST(app_hang_thread_list)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#else
    sentry_thread_info_t threads[SENTRY_APP_HANG_MAX_THREADS];
    size_t count = sentry__thread_list(threads, SENTRY_APP_HANG_MAX_THREADS);
    TEST_CHECK(count > 0);

    const uint64_t self = sentry__app_hang_current_tid();
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (threads[i].tid == self) {
            found = true;
        }
    }
    TEST_CHECK(found);

    TEST_CHECK(sentry__thread_list(threads, 1) == 1);
#endif
}

typedef struct {
    long seen;
    long has_threads;
    long thread_count;
    long crashed_count;
    long crashed_matches_exception;
    long crashed_has_stacktrace;
    long others_with_frames;
} all_threads_result_t;

static all_threads_result_t g_all_threads;

static sentry_value_t
all_threads_before_send(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    (void)hint;
    (void)data;
    all_threads_result_t r;
    memset(&r, 0, sizeof(r));

    sentry_value_t exc = sentry_value_get_by_index(
        sentry_value_get_by_key(
            sentry_value_get_by_key(event, "exception"), "values"),
        0);
    const char *exc_tid
        = sentry_value_as_string(sentry_value_get_by_key(exc, "thread_id"));

    char hung_buf[32];
    snprintf(hung_buf, sizeof(hung_buf), "%llu",
        (unsigned long long)sentry__app_hang_current_latch().target_tid);

    sentry_value_t threads = sentry_value_get_by_key(
        sentry_value_get_by_key(event, "threads"), "values");
    r.has_threads = !sentry_value_is_null(threads);
    r.thread_count = (long)sentry_value_get_length(threads);
    for (size_t i = 0; i < sentry_value_get_length(threads); i++) {
        sentry_value_t thread = sentry_value_get_by_index(threads, i);
        const char *id
            = sentry_value_as_string(sentry_value_get_by_key(thread, "id"));
        sentry_value_t frames = sentry_value_get_by_key(
            sentry_value_get_by_key(thread, "stacktrace"), "frames");
        if (sentry_value_is_true(sentry_value_get_by_key(thread, "crashed"))) {
            r.crashed_count++;
            r.crashed_matches_exception = exc_tid && strcmp(id, exc_tid) == 0
                && strcmp(id, hung_buf) == 0;
            r.crashed_has_stacktrace = !sentry_value_is_null(frames);
        } else if (sentry_value_get_length(frames) > 0) {
            r.others_with_frames++;
        }
    }

    g_all_threads = r;
    sentry__atomic_store(&g_all_threads.seen, 1);
    sentry_value_decref(event);
    return sentry_value_new_null();
}

static volatile long g_keep_idling;

SENTRY_THREAD_FN
idler(void *arg)
{
    (void)arg;
    while (sentry__atomic_fetch(&g_keep_idling)) {
        sleep_ms(5);
    }
    return 0;
}

#define MAX_IDLE_THREADS 300

static void
run_all_threads_hang(bool attach_all_threads, size_t idle_count)
{
    static sentry_threadid_t idle_threads[MAX_IDLE_THREADS];
    memset(&g_all_threads, 0, sizeof(g_all_threads));
    sentry__atomic_store(&g_keep_spinning, 1);
    sentry__atomic_store(&g_keep_idling, 1);
    sentry__app_hang_latch_reset();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send(options, all_threads_before_send, NULL);
    sentry_options_set_enable_app_hang_tracking(options, 1);
    sentry_options_set_app_hang_timeout(options, 50);
    sentry_options_set_app_hang_attach_all_threads(options, attach_all_threads);
    sentry_init(options);

    for (size_t i = 0; i < idle_count; i++) {
        sentry__thread_spawn(&idle_threads[i], idler, NULL);
    }
    sentry_threadid_t hung_thread;
    sentry__thread_spawn(&hung_thread, spinner, NULL);

    for (int i = 0; i < 1000 && !sentry__atomic_fetch(&g_all_threads.seen);
        i++) {
        sleep_ms(10);
    }

    sentry__atomic_store(&g_keep_spinning, 0);
    sentry__atomic_store(&g_keep_idling, 0);
    sentry__thread_join(hung_thread);
    for (size_t i = 0; i < idle_count; i++) {
        sentry__thread_join(idle_threads[i]);
    }
    sentry_close();
}

SENTRY_TEST(app_hang_without_all_threads)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    run_all_threads_hang(false, 1);
    TEST_CHECK(sentry__atomic_fetch(&g_all_threads.seen) == 1);
    TEST_CHECK(!g_all_threads.has_threads);
}

SENTRY_TEST(app_hang_with_all_threads)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    run_all_threads_hang(true, 1);
    TEST_CHECK(sentry__atomic_fetch(&g_all_threads.seen) == 1);
    TEST_CHECK(g_all_threads.crashed_count == 1);
    TEST_CHECK(g_all_threads.crashed_matches_exception);
    TEST_CHECK(!g_all_threads.crashed_has_stacktrace);
    // The test runner and the idle thread.
    TEST_CHECK(g_all_threads.others_with_frames >= 2);
}

SENTRY_TEST(app_hang_all_threads_limits_thread_count)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#endif
    run_all_threads_hang(true, MAX_IDLE_THREADS);
    TEST_CHECK(sentry__atomic_fetch(&g_all_threads.seen) == 1);
    TEST_CHECK(g_all_threads.thread_count <= SENTRY_APP_HANG_MAX_THREADS + 1);
    TEST_CHECK(g_all_threads.others_with_frames > MAX_IDLE_THREADS / 2);
    TEST_MSG("threads: %ld, with frames: %ld", g_all_threads.thread_count,
        g_all_threads.others_with_frames);
}

#define STRESS_CAPTURES 50

static volatile long g_stress_events;
static volatile long g_stress_threadless_events;
static volatile long g_stress_keep_loading;

static sentry_value_t
stress_before_send(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    (void)hint;
    (void)data;
    sentry_value_t threads = sentry_value_get_by_key(
        sentry_value_get_by_key(event, "threads"), "values");
    if (sentry_value_get_length(threads) < 2) {
        sentry__atomic_fetch_and_add(&g_stress_threadless_events, 1);
    }
    sentry__atomic_fetch_and_add(&g_stress_events, 1);
    sentry_value_decref(event);
    return sentry_value_new_null();
}

SENTRY_THREAD_FN
module_loader(void *arg)
{
    (void)arg;
    while (sentry__atomic_fetch(&g_stress_keep_loading)) {
#if defined(SENTRY_PLATFORM_WINDOWS)
        HMODULE module = LoadLibraryA("winmm.dll");
        if (module) {
            FreeLibrary(module);
        }
        module = LoadLibraryA("sentry-test-missing.dll");
        if (module) {
            FreeLibrary(module);
        }
#else
        void *handle = dlopen("libz.so.1", RTLD_NOW);
        if (handle) {
            dlclose(handle);
        }
        handle = dlopen("libsentry-test-missing.so", RTLD_NOW);
        if (handle) {
            dlclose(handle);
        }
#endif
    }
    return 0;
}

SENTRY_THREAD_FN
repeatedly_hung(void *arg)
{
    (void)arg;
    for (long i = 0; i < STRESS_CAPTURES; i++) {
        sentry_app_hang_heartbeat();
        for (int wait = 0;
            wait < 500 && sentry__atomic_fetch(&g_stress_events) <= i; wait++) {
            sleep_ms(10);
        }
    }
    return 0;
}

SENTRY_TEST(app_hang_all_threads_during_module_loading)
{
#if !SENTRY_HAS_THREAD_STACKWALK
    SKIP_TEST();
#else
    g_stress_events = 0;
    g_stress_threadless_events = 0;
    sentry__atomic_store(&g_stress_keep_loading, 1);
    sentry__app_hang_latch_reset();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send(options, stress_before_send, NULL);
    sentry_options_set_enable_app_hang_tracking(options, 1);
    sentry_options_set_app_hang_timeout(options, 50);
    sentry_options_set_app_hang_attach_all_threads(options, 1);
    sentry_init(options);

    sentry_threadid_t loaders[2];
    for (size_t i = 0; i < 2; i++) {
        sentry__thread_spawn(&loaders[i], module_loader, NULL);
    }
    sentry_threadid_t hung;
    sentry__thread_spawn(&hung, repeatedly_hung, NULL);
    sentry__thread_join(hung);

    sentry__atomic_store(&g_stress_keep_loading, 0);
    for (size_t i = 0; i < 2; i++) {
        sentry__thread_join(loaders[i]);
    }
    sentry_close();

    TEST_CHECK(sentry__atomic_fetch(&g_stress_events) == STRESS_CAPTURES);
    TEST_CHECK(sentry__atomic_fetch(&g_stress_threadless_events) == 0);
#endif
}

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
static volatile long g_blocked_tid;
static volatile long g_keep_blocking;
static long g_blocked_seen[2];
static long g_blocked_has_stack[2];
static volatile long g_blocked_events;

SENTRY_THREAD_FN
signal_blocker(void *arg)
{
    (void)arg;
    sigset_t all;
    sigfillset(&all);
    pthread_sigmask(SIG_BLOCK, &all, NULL);
    sentry__atomic_store(&g_blocked_tid, (long)syscall(SYS_gettid));
    while (sentry__atomic_fetch(&g_keep_blocking)) {
        sleep_ms(5);
    }
    // Delivers the signal of the timed-out sample, which must be ignored.
    pthread_sigmask(SIG_UNBLOCK, &all, NULL);
    while (sentry__atomic_fetch(&g_keep_idling)) {
        sleep_ms(5);
    }
    return 0;
}

static sentry_value_t
blocked_before_send(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    (void)hint;
    (void)data;
    const long index = sentry__atomic_fetch(&g_blocked_events);
    char blocked_id[32];
    snprintf(blocked_id, sizeof(blocked_id), "%ld",
        sentry__atomic_fetch(&g_blocked_tid));
    sentry_value_t threads = sentry_value_get_by_key(
        sentry_value_get_by_key(event, "threads"), "values");
    for (size_t i = 0; index < 2 && i < sentry_value_get_length(threads); i++) {
        sentry_value_t thread = sentry_value_get_by_index(threads, i);
        const char *id
            = sentry_value_as_string(sentry_value_get_by_key(thread, "id"));
        if (strcmp(id, blocked_id) == 0) {
            g_blocked_seen[index] = 1;
            g_blocked_has_stack[index] = !sentry_value_is_null(
                sentry_value_get_by_key(thread, "stacktrace"));
        }
    }
    sentry__atomic_fetch_and_add(&g_blocked_events, 1);
    sentry_value_decref(event);
    return sentry_value_new_null();
}

SENTRY_THREAD_FN
hung_twice(void *arg)
{
    (void)arg;
    for (long i = 0; i < 2; i++) {
        sentry_app_hang_heartbeat();
        for (int wait = 0;
            wait < 500 && sentry__atomic_fetch(&g_blocked_events) <= i;
            wait++) {
            sleep_ms(10);
        }
        sentry__atomic_store(&g_keep_blocking, 0);
        sleep_ms(50);
    }
    return 0;
}
#endif

SENTRY_TEST(app_hang_all_threads_signal_blocked)
{
#if !defined(SENTRY_PLATFORM_LINUX) && !defined(SENTRY_PLATFORM_ANDROID)
    SKIP_TEST();
#else
    memset(g_blocked_seen, 0, sizeof(g_blocked_seen));
    memset(g_blocked_has_stack, 0, sizeof(g_blocked_has_stack));
    g_blocked_events = 0;
    g_blocked_tid = 0;
    sentry__atomic_store(&g_keep_blocking, 1);
    sentry__atomic_store(&g_keep_idling, 1);
    sentry__app_hang_latch_reset();
    sentry__app_hang_monitor_set_stackwalk_fn(NULL);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_send(options, blocked_before_send, NULL);
    sentry_options_set_enable_app_hang_tracking(options, 1);
    sentry_options_set_app_hang_timeout(options, 50);
    sentry_options_set_app_hang_attach_all_threads(options, 1);
    sentry_init(options);

    sentry_threadid_t blocker;
    sentry__thread_spawn(&blocker, signal_blocker, NULL);
    while (!sentry__atomic_fetch(&g_blocked_tid)) {
        sleep_ms(1);
    }

    const uint64_t start = sentry__monotonic_time();
    sentry_threadid_t hung;
    sentry__thread_spawn(&hung, hung_twice, NULL);
    sentry__thread_join(hung);
    const uint64_t elapsed = sentry__monotonic_time() - start;

    sentry__atomic_store(&g_keep_idling, 0);
    sentry__thread_join(blocker);
    sentry_close();

    TEST_CHECK(sentry__atomic_fetch(&g_blocked_events) == 2);
    // The first capture times out on the blocked thread and lists it without
    // a stack trace. The second one walks it after the signals are unblocked.
    TEST_CHECK(g_blocked_seen[0] && !g_blocked_has_stack[0]);
    TEST_CHECK(g_blocked_seen[1] && g_blocked_has_stack[1]);
    TEST_CHECK(elapsed < 3000);
    TEST_MSG("elapsed: %llu ms", (unsigned long long)elapsed);
#endif
}
