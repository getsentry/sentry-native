#include "sentry_logs.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_os.h"
#include "sentry_scope.h"
#include "sentry_sync.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

// TODO think about this
#ifdef SENTRY_UNITTEST
#    define QUEUE_LENGTH 5
#else
#    define QUEUE_LENGTH 100
#endif
#define FLUSH_TIMER 5

typedef struct {
    sentry_value_t logs[QUEUE_LENGTH];
    volatile long count; // Number of logs in this buffer
} log_buffer_t;

struct logs_queue {
    log_buffer_t buffers[2]; // Double buffer
    volatile long
        active_buffer; // 0 or 1 - which buffer is currently active for writing
    volatile long
        timer_running; // atomic flag: 1 if timer thread is active, 0 otherwise
    volatile long flushing; // atomic flag: 1 if currently flushing, 0 otherwise
} typedef LogQueue;

// Global logs state including bgworker for timer management
static struct {
    LogQueue queue;
    sentry_bgworker_t *timer_worker;
    volatile long
        timer_task_submitted; // 1 if timer task is submitted, 0 otherwise
} g_logs_state = { .queue = { .buffers = { { .count = 0 }, { .count = 0 } },
                       .active_buffer = 0,
                       .timer_running = 0,
                       .flushing = 0 },
    .timer_worker = NULL,
    .timer_task_submitted = 0 };

// Convenience macro to access the queue
#define lq g_logs_state.queue

// Forward declaration for timer task function
static void timer_task_func(void *task_data, void *worker_state);

static void
flush_logs(void)
{
    // Use atomic compare-and-swap to ensure only one thread flushes at a time
    long expected = 0;
    // TODO platform-agnostic?
    if (!__sync_bool_compare_and_swap(&lq.flushing, expected, 1)) {
        // Another thread is already flushing
        return;
    }

    // Determine which buffer to flush
    long current_active = sentry__atomic_fetch(&lq.active_buffer);
    long flush_buffer_idx
        = current_active; // Start by trying to flush the active buffer

    // Check if active buffer has any logs to flush
    if (sentry__atomic_fetch(&lq.buffers[current_active].count) == 0) {
        // Active buffer is empty, try the other buffer
        // TODO do we want this?
        flush_buffer_idx = 1 - current_active;
        if (sentry__atomic_fetch(&lq.buffers[flush_buffer_idx].count) == 0) {
            // Both buffers are empty, nothing to flush
            sentry__atomic_store(&lq.flushing, 0);
            return;
        }
    } else {
        // Active buffer has logs, switch to the other buffer for new writes
        long new_active = 1 - current_active;
        sentry__atomic_store(&lq.active_buffer, new_active);
    }

    log_buffer_t *flush_buffer = &lq.buffers[flush_buffer_idx];
    long count = sentry__atomic_fetch(&flush_buffer->count);

    if (count == 0) { // TODO when can this happen?
        // Buffer became empty while we were setting up, nothing to do
        sentry__atomic_store(&lq.flushing, 0);
        return;
    }

    // Create envelope with logs from the buffer being flushed
    sentry_value_t logs = sentry_value_new_object();
    sentry_value_t logs_list = sentry_value_new_list();

    for (long i = 0; i < count; i++) {
        sentry_value_append(logs_list, flush_buffer->logs[i]);
    }

    sentry_value_set_by_key(logs, "items", logs_list);

    // Send the envelope
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_logs(envelope, logs);
    // TODO remove debug write to file below
    // sentry_envelope_write_to_file(envelope, "logs_envelope.json");
    SENTRY_WITH_OPTIONS (options) {
        sentry__capture_envelope(options->transport, envelope);
    }

    // Free the logs object since envelope doesn't take ownership
    sentry_value_decref(logs);

    // Reset the flushed buffer - this is now safe since we switched active
    // buffer
    sentry__atomic_store(&flush_buffer->count, 0);

    // Clear flushing flag
    sentry__atomic_store(&lq.flushing, 0);
}

static bool
enqueu_log(sentry_value_t log)
{
    // Retry loop in case buffer switches during our attempt
    // TODO do we want this? adds busy waiting of 1ms
    for (int retry = 0; retry < 1; retry++) {
        // Get current active buffer index
        long buffer_idx = sentry__atomic_fetch(&lq.active_buffer);
        log_buffer_t *current_buffer = &lq.buffers[buffer_idx];

        // Try to get a slot in the current buffer
        long log_idx = sentry__atomic_fetch_and_add(&current_buffer->count, 1);

        if (log_idx < QUEUE_LENGTH) {
            // Successfully got a slot, write the log
            current_buffer->logs[log_idx] = log;

            // Check if this buffer is now full and trigger flush
            if (log_idx == QUEUE_LENGTH - 1) {
                flush_logs();
            }

            // Start timer thread if this is the first log in any buffer
            bool should_start_timer = false;
            if (log_idx == 0) {
                // Check if the other buffer is also empty
                long other_buffer_idx = 1 - buffer_idx;
                if (sentry__atomic_fetch(&lq.buffers[other_buffer_idx].count)
                    == 0) {
                    should_start_timer = true;
                }
            }

            if (should_start_timer) {
                long was_running
                    = sentry__atomic_fetch_and_add(&lq.timer_running, 1);
                if (was_running == 0) {
                    // We're the first to set the timer, ensure bgworker exists
                    // and submit task
                    if (!g_logs_state.timer_worker) {
                        g_logs_state.timer_worker
                            = sentry__bgworker_new(NULL, NULL);
                        if (g_logs_state.timer_worker) {
                            sentry__bgworker_setname(
                                g_logs_state.timer_worker, "sentry-timer");
                            if (sentry__bgworker_start(
                                    g_logs_state.timer_worker)
                                != 0) {
                                SENTRY_WARN("Failed to start timer bgworker");
                                sentry__bgworker_decref(
                                    g_logs_state.timer_worker);
                                g_logs_state.timer_worker = NULL;
                                sentry__atomic_fetch_and_add(
                                    &lq.timer_running, -1);
                            }
                        } else {
                            SENTRY_WARN("Failed to create timer bgworker");
                            sentry__atomic_fetch_and_add(&lq.timer_running, -1);
                        }
                    }

                    // Submit timer task if worker is available and no task is
                    // already submitted
                    if (g_logs_state.timer_worker
                        && !sentry__atomic_fetch(
                            &g_logs_state.timer_task_submitted)) {
                        sentry__atomic_store(
                            &g_logs_state.timer_task_submitted, 1);
                        if (sentry__bgworker_submit(g_logs_state.timer_worker,
                                timer_task_func, NULL, NULL)
                            != 0) {
                            SENTRY_WARN("Failed to submit timer task");
                            sentry__atomic_fetch_and_add(&lq.timer_running, -1);
                            sentry__atomic_store(
                                &g_logs_state.timer_task_submitted, 0);
                        }
                    } else if (!g_logs_state.timer_worker) {
                        // No worker available, reset the running flag
                        sentry__atomic_fetch_and_add(&lq.timer_running, -1);
                    } else {
                        // Task already submitted, reset the running flag
                        sentry__atomic_fetch_and_add(&lq.timer_running, -1);
                    }
                } else {
                    // Timer was already running, decrement back
                    sentry__atomic_fetch_and_add(&lq.timer_running, -1);
                }
            }

            return true;
        } else {
            // Buffer is full, roll back our increment
            sentry__atomic_fetch_and_add(&current_buffer->count, -1);

            // Try to trigger a flush to switch buffers
            flush_logs();

            // Brief pause before retry to allow flush to complete
#ifdef SENTRY_PLATFORM_WINDOWS
            Sleep(1);
#else
            usleep(1000); // 1ms
#endif
        }
    }

    // All retries exhausted - both buffers are likely full
    SENTRY_WARN("Unable to enqueue log - all buffers full");
    return false;
}

static void
timer_task_func(void *task_data, void *worker_state)
{
    (void)task_data; // unused parameter
    (void)worker_state; // unused parameter

    // Sleep for 5 seconds
#ifdef SENTRY_PLATFORM_WINDOWS
    Sleep(FLUSH_TIMER * 1000);
#else
    sleep(FLUSH_TIMER);
#endif

    // Try to flush logs - this will flush whichever buffer has content
    flush_logs();

    // Reset timer state - decrement the counter and mark task as completed
    sentry__atomic_fetch_and_add(&lq.timer_running, -1);
    sentry__atomic_store(&g_logs_state.timer_task_submitted, 0);
}

static const char *
level_as_string(sentry_level_t level)
{
    switch (level) {
    case SENTRY_LEVEL_TRACE:
        return "trace";
    case SENTRY_LEVEL_DEBUG:
        return "debug";
    case SENTRY_LEVEL_INFO:
        return "info";
    case SENTRY_LEVEL_WARNING:
        return "warn";
    case SENTRY_LEVEL_ERROR:
        return "error";
    case SENTRY_LEVEL_FATAL:
        return "fatal";
    default:
        return "unknown";
    }
}

static sentry_value_t
construct_param_from_conversion(const char conversion, va_list *args_copy)
{
    sentry_value_t param_obj = sentry_value_new_object();
    switch (conversion) {
    case 'd':
    case 'i': {
        long long val = va_arg(*args_copy, long long);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_int64(val));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("integer"));
        break;
    }
    case 'u':
    case 'x':
    case 'X':
    case 'o': {
        unsigned long long int val = va_arg(*args_copy, unsigned long long int);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_uint64(val));
        // TODO update once unsigned 64-bit can be sent
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G': {
        double val = va_arg(*args_copy, double);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_double(val));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("double"));
        break;
    }
    case 'c': {
        int val = va_arg(*args_copy, int);
        char str[2] = { (char)val, '\0' };
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_string(str));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    case 's': {
        const char *val = va_arg(*args_copy, const char *);
        if (val) {
            sentry_value_set_by_key(
                param_obj, "value", sentry_value_new_string(val));
        } else {
            sentry_value_set_by_key(
                param_obj, "value", sentry_value_new_string("(null)"));
        }
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    case 'p': {
        void *val = va_arg(*args_copy, void *);
        char ptr_str[32];
        snprintf(ptr_str, sizeof(ptr_str), "%p", val);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_string(ptr_str));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }
    default:
        // Unknown format specifier, skip the argument
        (void)va_arg(*args_copy, void *);
        sentry_value_set_by_key(
            param_obj, "value", sentry_value_new_string("(unknown)"));
        sentry_value_set_by_key(
            param_obj, "type", sentry_value_new_string("string"));
        break;
    }

    return param_obj;
}

static const char *
skip_flags(const char *fmt_ptr)
{
    while (*fmt_ptr
        && (*fmt_ptr == '-' || *fmt_ptr == '+' || *fmt_ptr == ' '
            || *fmt_ptr == '#' || *fmt_ptr == '0')) {
        fmt_ptr++;
    }
    return fmt_ptr;
}

static const char *
skip_width(const char *fmt_ptr)
{
    while (*fmt_ptr && (*fmt_ptr >= '0' && *fmt_ptr <= '9')) {
        fmt_ptr++;
    }
    return fmt_ptr;
}

static const char *
skip_precision(const char *fmt_ptr)
{

    if (*fmt_ptr == '.') {
        fmt_ptr++;
        while (*fmt_ptr && (*fmt_ptr >= '0' && *fmt_ptr <= '9')) {
            fmt_ptr++;
        }
    }
    return fmt_ptr;
}

static const char *
skip_length(const char *fmt_ptr)
{
    while (*fmt_ptr
        && (*fmt_ptr == 'h' || *fmt_ptr == 'l' || *fmt_ptr == 'L'
            || *fmt_ptr == 'z' || *fmt_ptr == 'j' || *fmt_ptr == 't')) {
        fmt_ptr++;
    }
    return fmt_ptr;
}

static void
populate_message_parameters(
    sentry_value_t attributes, const char *message, va_list args)
{
    if (!message || sentry_value_is_null(attributes)) {
        return;
    }

    const char *fmt_ptr = message;
    int param_index = 0;
    va_list args_copy;
    va_copy(args_copy, args);

    while (*fmt_ptr) {
        // Find the next format specifier
        if (*fmt_ptr == '%') {
            fmt_ptr++; // Skip the '%'

            if (*fmt_ptr == '%') {
                // Escaped '%', not a format specifier
                fmt_ptr++;
                continue;
            }

            fmt_ptr = skip_flags(fmt_ptr);
            fmt_ptr = skip_width(fmt_ptr);
            fmt_ptr = skip_precision(fmt_ptr);
            fmt_ptr = skip_length(fmt_ptr);

            // Get the conversion specifier
            char conversion = *fmt_ptr;
            if (conversion) {
                char key[64];
                snprintf(key, sizeof(key), "sentry.message.parameter.%d",
                    param_index);
                sentry_value_t param_obj
                    = construct_param_from_conversion(conversion, &args_copy);
                sentry_value_set_by_key(attributes, key, param_obj);
                param_index++;
                fmt_ptr++;
            }
        } else {
            fmt_ptr++;
        }
    }

    va_end(args_copy);
}

static void
add_attribute(sentry_value_t attributes, sentry_value_t value, const char *type,
    const char *name)
{
    sentry_value_t param_obj = sentry_value_new_object();
    sentry_value_set_by_key(param_obj, "value", value);
    sentry_value_set_by_key(param_obj, "type", sentry_value_new_string(type));
    sentry_value_set_by_key(attributes, name, param_obj);
}

static sentry_value_t
construct_log(sentry_level_t level, const char *message, va_list args)
{
    sentry_value_t log = sentry_value_new_object();
    sentry_value_t attributes = sentry_value_new_object();

    va_list args_copy_1, args_copy_2, args_copy_3;
    va_copy(args_copy_1, args);
    va_copy(args_copy_2, args);
    va_copy(args_copy_3, args);
    int len = vsnprintf(NULL, 0, message, args_copy_1) + 1;
    va_end(args_copy_1);
    size_t size = (size_t)len;
    char *fmt_message = sentry_malloc(size);
    if (!fmt_message) {
        va_end(args_copy_2);
        va_end(args_copy_3);
        return sentry_value_new_null();
    }

    vsnprintf(fmt_message, size, message, args_copy_2);
    va_end(args_copy_2);

    sentry_value_set_by_key(log, "body", sentry_value_new_string(fmt_message));
    sentry_free(fmt_message);
    sentry_value_set_by_key(
        log, "level", sentry_value_new_string(level_as_string(level)));

    // timestamp in seconds
    uint64_t usec_time = sentry__usec_time();
    sentry_value_set_by_key(log, "timestamp",
        sentry_value_new_double((double)usec_time / 1000000.0));

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_set_by_key(log, "trace_id",
            sentry__value_clone(sentry_value_get_by_key(
                sentry_value_get_by_key(scope->propagation_context, "trace"),
                "trace_id")));

        sentry_value_t parent_span_id = sentry_value_new_object();
        if (scope->transaction_object) {
            sentry_value_set_by_key(parent_span_id, "value",
                sentry__value_clone(sentry_value_get_by_key(
                    scope->transaction_object->inner, "span_id")));
        }

        if (scope->span) {
            sentry_value_set_by_key(parent_span_id, "value",
                sentry__value_clone(
                    sentry_value_get_by_key(scope->span->inner, "span_id")));
        }
        sentry_value_set_by_key(
            parent_span_id, "type", sentry_value_new_string("string"));
        if (scope->transaction_object || scope->span) {
            sentry_value_set_by_key(
                attributes, "sentry.trace.parent_span_id", parent_span_id);
        } else {
            sentry_value_decref(parent_span_id);
        }

        if (!sentry_value_is_null(scope->user)) {
            sentry_value_t user_id = sentry_value_get_by_key(scope->user, "id");
            if (!sentry_value_is_null(user_id)) {
                add_attribute(attributes, user_id, "string", "user.id");
            }
            sentry_value_t user_username
                = sentry_value_get_by_key(scope->user, "username");
            if (!sentry_value_is_null(user_username)) {
                add_attribute(attributes, user_username, "string", "user.name");
            }
            sentry_value_t user_email
                = sentry_value_get_by_key(scope->user, "email");
            if (!sentry_value_is_null(user_email)) {
                add_attribute(attributes, user_email, "string", "user.email");
            }
        }
        sentry_value_t os_context = sentry__get_os_context();
        if (!sentry_value_is_null(os_context)) {
            sentry_value_t os_name = sentry__value_clone(
                sentry_value_get_by_key(os_context, "name"));
            sentry_value_t os_version = sentry__value_clone(
                sentry_value_get_by_key(os_context, "version"));
            if (!sentry_value_is_null(os_name)) {
                add_attribute(attributes, os_name, "string", "os.name");
            }
            if (!sentry_value_is_null(os_version)) {
                add_attribute(attributes, os_version, "string", "os.version");
            }
        }
        sentry_value_decref(os_context);
    }
    SENTRY_WITH_OPTIONS (options) {
        if (options->environment) {
            add_attribute(attributes,
                sentry_value_new_string(options->environment), "string",
                "sentry.environment");
        }
        if (options->release) {
            add_attribute(attributes, sentry_value_new_string(options->release),
                "string", "sentry.release");
        }
    }

    add_attribute(attributes, sentry_value_new_string("sentry.native"),
        "string", "sentry.sdk.name");
    add_attribute(attributes, sentry_value_new_string(sentry_sdk_version()),
        "string", "sentry.sdk.version");
    add_attribute(attributes, sentry_value_new_string(message), "string",
        "sentry.message.template");

    // Parse variadic arguments and add them to attributes
    populate_message_parameters(attributes, message, args_copy_3);
    va_end(args_copy_3);

    sentry_value_set_by_key(log, "attributes", attributes);

    return log;
}

void
sentry__logs_log(sentry_level_t level, const char *message, va_list args)
{
    bool enable_logs = false;
    SENTRY_WITH_OPTIONS (options) {
        if (options->enable_logs)
            enable_logs = true;
    }
    if (enable_logs) {
        // create log from message
        sentry_value_t log = construct_log(level, message, args);
        enqueu_log(log);
    }
}

void
sentry_log_trace(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LEVEL_TRACE, message, args);
    va_end(args);
}

void
sentry_log_debug(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LEVEL_DEBUG, message, args);
    va_end(args);
}

void
sentry_log_info(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LEVEL_INFO, message, args);
    va_end(args);
}

void
sentry_log_warn(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LEVEL_WARNING, message, args);
    va_end(args);
}

void
sentry_log_error(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LEVEL_ERROR, message, args);
    va_end(args);
}

void
sentry_log_fatal(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LEVEL_FATAL, message, args);
    va_end(args);
}

void
sentry__logs_shutdown(uint64_t timeout)
{
    SENTRY_DEBUG("shutting down logs system");

    // Shutdown the timer bgworker if it exists
    if (g_logs_state.timer_worker) {
        if (sentry__bgworker_shutdown(g_logs_state.timer_worker, timeout)
            != 0) {
            SENTRY_WARN(
                "timer bgworker did not shut down cleanly within timeout");
        }
        // TODO this already happens inside worker_thread
        // sentry__bgworker_decref(g_logs_state.timer_worker);
        // g_logs_state.timer_worker = NULL;
    }

    // Reset state flags
    sentry__atomic_store(&g_logs_state.timer_task_submitted, 0);
    sentry__atomic_store(&lq.timer_running, 0);

    // Perform final flush to ensure any remaining logs are sent
    flush_logs();

    SENTRY_DEBUG("logs system shutdown complete");
}
