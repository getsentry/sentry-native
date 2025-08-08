#include "sentry_logs.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_os.h"
#include "sentry_scope.h"
#include "sentry_sync.h"

#include <stdarg.h>
#include <string.h>

const int QUEUE_LENGTH = 10;

struct logs_queue {
    sentry_value_t queue[QUEUE_LENGTH]; // could just be sentry_value_t list?
    long index;
    long adding;
} typedef LogQueue;

LogQueue lq;

static void
flush_logs()
{
    sentry_value_t logs = sentry_value_new_object();
    sentry_value_t logs_list = sentry_value_new_list();
    for (int i = 0; i < QUEUE_LENGTH; i++) {
        sentry_value_append(logs_list, lq.queue[i]);
    }
    sentry_value_set_by_key(logs, "items", logs_list);
    // sending of the envelope
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_logs(envelope, logs);
    // TODO remove debug write to file below
    sentry_envelope_write_to_file(envelope, "logs_envelope.json");
    SENTRY_WITH_OPTIONS (options) {
        sentry__capture_envelope(options->transport, envelope);
    }
    // free the logs object since envelope doesn't take
    sentry_value_decref(logs);
    sentry__atomic_store(&lq.index, 0);
}

static bool
enqueu_log(sentry_value_t log)
{
    // atomically fetch index and add one
    long log_idx = sentry__atomic_fetch_and_add(&lq.index, 1);
    sentry__atomic_fetch_and_add(&lq.adding, 1);
    if (log_idx >= 100) { // already flushing
        // TODO send client report?
        SENTRY_WARN("Losing log");
        return false;
    }
    lq.queue[log_idx] = log;
    if (log_idx == QUEUE_LENGTH - 1) {
        flush_logs();
    }
    sentry__atomic_fetch_and_add(&lq.adding, -1);
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
