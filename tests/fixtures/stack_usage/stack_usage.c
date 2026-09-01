#include "sentry_boot.h"

#include "sentry.h"
#include "sentry_alloc.h"
#include "sentry_integration.h"
#include "sentry_options.h"

#include <stdint.h>
#include <string.h>

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <io.h>
#else
#    include <unistd.h>
#endif

#ifndef STDOUT_FILENO
#    define STDOUT_FILENO 1
#endif

#if defined(_MSC_VER)
#    define NOINLINE __declspec(noinline)
#    define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#    define NOINLINE __attribute__((noinline))
#    define THREAD_LOCAL __thread
#else
#    define NOINLINE
#    define THREAD_LOCAL _Thread_local
#endif

// Crash callbacks may run on a 16 KiB alternate signal stack. Paint 12 KiB
// and leave 4 KiB for the signal frame and callback machinery.
#define STACK_PAINT_SIZE (12 * 1024)
#define STACK_PATTERN(offset) ((unsigned char)(0x5aU + (offset) * 131U))

static THREAD_LOCAL uintptr_t g_stack_usage_low;
static THREAD_LOCAL uintptr_t g_stack_usage_high;
static THREAD_LOCAL unsigned int g_stack_usage_depth;

static NOINLINE void
paint_stack(void)
{
    // Handler calls reuse this frame after it returns, leaving a high-water
    // mark in the pattern.
    volatile unsigned char stack[STACK_PAINT_SIZE];
    for (size_t i = 0; i < sizeof(stack); i++) {
        stack[i] = STACK_PATTERN(i);
    }
    g_stack_usage_low = (uintptr_t)&stack[0];
    g_stack_usage_high = (uintptr_t)&stack[sizeof(stack)];
}

static NOINLINE size_t
measure_stack(void)
{
    for (uintptr_t ptr = g_stack_usage_low; ptr < g_stack_usage_high; ptr++) {
        size_t offset = (size_t)(ptr - g_stack_usage_low);
        if (*(volatile unsigned char *)ptr != STACK_PATTERN(offset)) {
            return (size_t)(g_stack_usage_high - ptr);
        }
    }
    return 0;
}

static NOINLINE void
write_measurement(size_t value)
{
    static const char prefix[] = "[STACK] ";
    static const char suffix[] = " bytes\n";
    char buf[sizeof(prefix) + 3 * sizeof(size_t) + sizeof(suffix)];
    size_t len = 0;
    for (size_t i = 0; i < sizeof(prefix) - 1; i++) {
        buf[len++] = prefix[i];
    }

    size_t digits_start = len;
    do {
        buf[len++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);

    size_t digits_len = len - digits_start;
    for (size_t i = 0; i < digits_len / 2; i++) {
        char tmp = buf[digits_start + i];
        buf[digits_start + i] = buf[len - i - 1];
        buf[len - i - 1] = tmp;
    }
    for (size_t i = 0; i < sizeof(suffix) - 1; i++) {
        buf[len++] = suffix[i];
    }
#ifdef SENTRY_PLATFORM_WINDOWS
    (void)!_write(STDOUT_FILENO, buf, (unsigned int)len);
#else
    (void)!write(STDOUT_FILENO, buf, len);
#endif
}

static void
enter_crash_handler(void *ptr)
{
    (void)ptr;
    if (g_stack_usage_depth++ == 0) {
        paint_stack();
    }
}

static void
exit_crash_handler(void *ptr)
{
    (void)ptr;
    if (!g_stack_usage_depth || --g_stack_usage_depth) {
        return;
    }

    size_t used = measure_stack();
    g_stack_usage_low = 0;
    g_stack_usage_high = 0;
    write_measurement(used);
}

static sentry_integration_t *
stack_usage_integration_new(void)
{
    sentry_integration_t *integration = SENTRY_MAKE(sentry_integration_t);
    if (!integration) {
        return NULL;
    }

    integration->crash_handler_enter_func = enter_crash_handler;
    integration->crash_handler_exit_func = exit_crash_handler;
    return integration;
}

static void *invalid_mem = (void *)1;

static sentry_value_t
on_crash(const sentry_ucontext_t *uctx, sentry_value_t event, void *data)
{
    (void)uctx;
    (void)data;
    return event;
}

static sentry_http_client_t *
dummy_http_client_factory(void *data)
{
    return (sentry_http_client_t *)data;
}

static int
dummy_http_client_send(sentry_http_client_t *client,
    sentry_http_request_t *request, sentry_http_response_t *response)
{
    (void)client;
    (void)request;
    sentry_http_response_set_status_code(response, 200);
    return 1;
}

int
main(void)
{
    static char dummy_http_client;
    sentry_options_t *options = sentry_options_new();
    sentry__options_add_integration(options, stack_usage_integration_new());
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_database_path(options, ".sentry-native");
    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_debug(options, 1);
    sentry_options_set_on_crash(options, on_crash, NULL);

    // Keep the HTTP transport's queue drain in the measured crash path. A
    // function transport skips that work, while this client avoids network I/O.
    sentry_options_set_transport(options,
        sentry_http_transport_new(dummy_http_client_factory, &dummy_http_client,
            dummy_http_client_send, NULL));

    if (sentry_init(options) != 0) {
        return 1;
    }

    sentry_value_t bytes
        = sentry_attachment_from_bytes("\xc0\xff\xee", 3, "bytes.bin");
    sentry_attachment_set_content_type(bytes, "application/octet-stream");
    sentry_add_attachment(bytes);
    sentry_start_session();
    sentry_crash();
    return 0;
}
