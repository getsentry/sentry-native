/**
 * Main test driver for inproc backend stress tests.
 *
 * Usage:
 *   ./inproc_stress_test concurrent-crash
 *   ./inproc_stress_test handler-thread-crash
 *   ./inproc_stress_test simple-crash
 *   ./inproc_stress_test pipe-failure
 */

#include "sentry.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#    include <signal.h>
#    include <unistd.h>
#endif

// From concurrent_crash.c
extern void run_concurrent_crash(void);

static void *invalid_mem = (void *)1;

// on_crash callback that crashes - simulates buggy user code
static sentry_value_t
crashing_on_crash_callback(
    const sentry_ucontext_t *uctx, sentry_value_t event, void *closure)
{
    (void)uctx;
    (void)event;
    (void)closure;

    fprintf(stderr, "on_crash callback about to crash\n");
    fflush(stderr);

    memset((char *)invalid_mem, 1, 100);

    return event;
}

static void
print_envelope(sentry_envelope_t *envelope, void *unused_state)
{
    (void)unused_state;
    size_t size_out = 0;
    char *s = sentry_envelope_serialize(envelope, &size_out);
    printf("%s", s);
    fflush(stdout);
    sentry_free(s);
    sentry_envelope_free(envelope);
}

static int
setup_sentry(const char *database_path)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_database_path(options, database_path);
    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_dsn(options, "https://public@sentry.invalid/1");
    sentry_options_set_debug(options, 1);
    sentry_options_set_transport(options, sentry_transport_new(print_envelope));

    if (sentry_init(options) != 0) {
        fprintf(stderr, "Failed to initialize sentry\n");
        return 1;
    }
    return 0;
}

static int
test_concurrent_crash(const char *database_path)
{
    if (setup_sentry(database_path) != 0) {
        return 1;
    }

    sentry_set_tag("test", "concurrent-crash");

    fprintf(stderr, "Starting concurrent crash test\n");
    fflush(stderr);

    run_concurrent_crash();

    // Should not reach this
    fprintf(stderr, "ERROR: Threads returned without crashing\n");
    sentry_close();
    return 1;
}

static void
trigger_crash(void)
{
    memset((char *)invalid_mem, 1, 100);
}

static int
setup_sentry_with_crashing_on_crash(const char *database_path)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_database_path(options, database_path);
    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_dsn(options, "https://public@sentry.invalid/1");
    sentry_options_set_debug(options, 1);
    sentry_options_set_transport(options, sentry_transport_new(print_envelope));

    // Set the crashing on_crash callback
    sentry_options_set_on_crash(options, crashing_on_crash_callback, NULL);

    if (sentry_init(options) != 0) {
        fprintf(stderr, "Failed to initialize sentry\n");
        return 1;
    }
    return 0;
}

static int
test_handler_thread_crash(const char *database_path)
{
    if (setup_sentry_with_crashing_on_crash(database_path) != 0) {
        return 1;
    }

    sentry_set_tag("test", "handler-thread-crash");

    fprintf(stderr, "Starting handler thread crash test\n");
    fflush(stderr);

    // This will crash, trigger the handler thread, which will call
    // on_crash callback, which will crash the handler thread.
    // The fallback should then process in the signal handler.
    trigger_crash();

    fprintf(stderr, "ERROR: Should have crashed\n");
    sentry_close();
    return 1;
}

static int
test_simple_crash(const char *database_path)
{
    if (setup_sentry(database_path) != 0) {
        return 1;
    }

    sentry_set_tag("test", "simple-crash");

    fprintf(stderr, "Starting simple crash test\n");
    fflush(stderr);

    trigger_crash();

    fprintf(stderr, "ERROR: Should have crashed\n");
    sentry_close();
    return 1;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <test-name> [database-path]\n", argv[0]);
        fprintf(stderr, "Tests:\n");
        fprintf(stderr, "  concurrent-crash       - Multiple threads crash simultaneously\n");
        fprintf(stderr, "  simple-crash           - Single thread crash (baseline)\n");
        fprintf(stderr, "  handler-thread-crash   - Handler thread crashes in on_crash\n");
        return 1;
    }

    const char *test_name = argv[1];
    const char *database_path = argc > 2 ? argv[2] : ".sentry-native";

    if (strcmp(test_name, "concurrent-crash") == 0) {
        return test_concurrent_crash(database_path);
    }
    if (strcmp(test_name, "simple-crash") == 0) {
        return test_simple_crash(database_path);
    }
    if (strcmp(test_name, "handler-thread-crash") == 0) {
        return test_handler_thread_crash(database_path);
    }
    fprintf(stderr, "Unknown test: %s\n", test_name);
    return 1;
}
