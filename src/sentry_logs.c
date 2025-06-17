#include "sentry_logs.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_scope.h"

#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <stdio.h>

char *
log_level_as_string(sentry_log_level_t level)
{
    switch (level) {
    case SENTRY_LOG_LEVEL_TRACE:
        return "trace";
    case SENTRY_LOG_LEVEL_DEBUG:
        return "debug";
    case SENTRY_LOG_LEVEL_INFO:
        return "info";
    case SENTRY_LOG_LEVEL_WARN:
        return "warn";
    case SENTRY_LOG_LEVEL_ERROR:
        return "error";
    case SENTRY_LOG_LEVEL_FATAL:
        return "fatal";
    default:
        return "UNKNOWN";
    }
}

static void
populate_message_parameters(
    const char *message, va_list args, sentry_value_t attributes)
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

            // Skip flags, width, and precision
            while (*fmt_ptr
                && (*fmt_ptr == '-' || *fmt_ptr == '+' || *fmt_ptr == ' '
                    || *fmt_ptr == '#' || *fmt_ptr == '0')) {
                fmt_ptr++;
            }
            while (*fmt_ptr && (*fmt_ptr >= '0' && *fmt_ptr <= '9')) {
                fmt_ptr++;
            }
            if (*fmt_ptr == '.') {
                fmt_ptr++;
                while (*fmt_ptr && (*fmt_ptr >= '0' && *fmt_ptr <= '9')) {
                    fmt_ptr++;
                }
            }

            // Skip length modifiers
            while (*fmt_ptr
                && (*fmt_ptr == 'h' || *fmt_ptr == 'l' || *fmt_ptr == 'L'
                    || *fmt_ptr == 'z' || *fmt_ptr == 'j' || *fmt_ptr == 't')) {
                fmt_ptr++;
            }

            if (*fmt_ptr == '%') {
                // Escaped '%', not a format specifier
                fmt_ptr++;
                continue;
            }

            // Get the conversion specifier
            char conversion = *fmt_ptr;
            if (conversion) {
                char key[64];
                snprintf(key, sizeof(key), "sentry.message.parameter.%d",
                    param_index);

                sentry_value_t param_obj = sentry_value_new_object();

                switch (conversion) {
                case 'd':
                case 'i': {
                    int val = va_arg(args_copy, int);
                    sentry_value_set_by_key(
                        param_obj, "value", sentry_value_new_int32(val));
                    sentry_value_set_by_key(
                        param_obj, "type", sentry_value_new_string("integer"));
                    break;
                }
                case 'u':
                case 'x':
                case 'X':
                case 'o': {
                    unsigned int val = va_arg(args_copy, unsigned int);
                    sentry_value_set_by_key(param_obj, "value",
                        sentry_value_new_int32((int32_t)val));
                    sentry_value_set_by_key(
                        param_obj, "type", sentry_value_new_string("integer"));
                    break;
                }
                case 'f':
                case 'F':
                case 'e':
                case 'E':
                case 'g':
                case 'G': {
                    double val = va_arg(args_copy, double);
                    sentry_value_set_by_key(
                        param_obj, "value", sentry_value_new_double(val));
                    sentry_value_set_by_key(
                        param_obj, "type", sentry_value_new_string("double"));
                    break;
                }
                case 'c': {
                    int val = va_arg(args_copy, int);
                    char str[2] = { (char)val, '\0' };
                    sentry_value_set_by_key(
                        param_obj, "value", sentry_value_new_string(str));
                    sentry_value_set_by_key(
                        param_obj, "type", sentry_value_new_string("string"));
                    break;
                }
                case 's': {
                    const char *val = va_arg(args_copy, const char *);
                    if (val) {
                        sentry_value_set_by_key(
                            param_obj, "value", sentry_value_new_string(val));
                    } else {
                        sentry_value_set_by_key(param_obj, "value",
                            sentry_value_new_string("(null)"));
                    }
                    sentry_value_set_by_key(
                        param_obj, "type", sentry_value_new_string("string"));
                    break;
                }
                case 'p': {
                    void *val = va_arg(args_copy, void *);
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
                    (void)va_arg(args_copy, void *);
                    sentry_value_set_by_key(param_obj, "value",
                        sentry_value_new_string("(unknown)"));
                    sentry_value_set_by_key(
                        param_obj, "type", sentry_value_new_string("string"));
                    break;
                }

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

static sentry_value_t
construct_log(sentry_log_level_t level, const char *message, va_list args)
{
    sentry_value_t log = sentry_value_new_object();
    sentry_value_t attributes = sentry_value_new_object();

    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, message, args_copy) + 1;
    va_end(args_copy);
    char *fmt_message = sentry_malloc(size);
    if (!fmt_message) {
        return sentry_value_new_null();
    }

    vsnprintf(fmt_message, size, message, args);

    sentry_value_set_by_key(log, "body", sentry_value_new_string(fmt_message));
    sentry_free(fmt_message);
    sentry_value_set_by_key(
        log, "level", sentry_value_new_string(log_level_as_string(level)));

    // timestamp in seconds
    sentry_value_set_by_key(log, "timestamp",
        sentry_value_new_double(sentry__usec_time() / 1000000.0));

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
        // TODO should we only add if either exists?
        sentry_value_set_by_key(
            attributes, "sentry.trace.parent_span_id", parent_span_id);
    }

    SENTRY_WITH_OPTIONS (options) {
        if (options->environment) {
            sentry_value_t environment = sentry_value_new_object();
            sentry_value_set_by_key(environment, "value",
                sentry_value_new_string(options->environment));
            sentry_value_set_by_key(
                environment, "type", sentry_value_new_string("string"));
            sentry_value_set_by_key(
                attributes, "sentry.environment", environment);
        }
        if (options->release) {
            sentry_value_t release = sentry_value_new_object();
            sentry_value_set_by_key(
                release, "value", sentry_value_new_string(options->release));
            sentry_value_set_by_key(
                release, "type", sentry_value_new_string("string"));
            sentry_value_set_by_key(attributes, "sentry.release", release);
        }
    }

    sentry_value_t sdk_name = sentry_value_new_object();
    sentry_value_set_by_key(
        sdk_name, "value", sentry_value_new_string("sentry.native"));
    sentry_value_set_by_key(
        sdk_name, "type", sentry_value_new_string("string"));
    sentry_value_set_by_key(attributes, "sentry.sdk.name", sdk_name);

    sentry_value_t sdk_version = sentry_value_new_object();
    sentry_value_set_by_key(
        sdk_version, "value", sentry_value_new_string(sentry_sdk_version()));
    sentry_value_set_by_key(
        sdk_version, "type", sentry_value_new_string("string"));
    sentry_value_set_by_key(attributes, "sentry.sdk.name", sdk_version);

    sentry_value_t message_template = sentry_value_new_object();
    sentry_value_set_by_key(
        message_template, "value", sentry_value_new_string(message));
    sentry_value_set_by_key(
        message_template, "type", sentry_value_new_string("string"));
    sentry_value_set_by_key(
        attributes, "sentry.message.template", message_template);

    // Parse variadic arguments and add them to attributes
    populate_message_parameters(message, args, attributes);

    sentry_value_set_by_key(log, "attributes", attributes);

    return log;
}

void
sentry__logs_log(sentry_log_level_t level, const char *message, va_list args)
{
    SENTRY_WITH_OPTIONS (options) {
        if (!options->enable_logs)
            return;
    }
    SENTRY_INFOF("Logging level: %i\n", level);
    vprintf(message, args);
    // create log from message
    sentry_value_t log = construct_log(level, message, args);

    // TODO split up the code below for batched log sending
    //    e.g. could we store logs on the scope?
    sentry_value_t logs = sentry_value_new_object();
    sentry_value_t logs_list = sentry_value_new_list();
    sentry_value_append(logs_list, log);
    sentry_value_set_by_key(logs, "items", logs_list);
    // sending of the envelope
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_logs(envelope, logs);
    // TODO remove debug write to file below
    sentry_envelope_write_to_file(envelope, "logs_envelope.json");
    SENTRY_WITH_OPTIONS (options) {
        sentry__capture_envelope(options->transport, envelope);
    }
}

// TODO think about the structure below, is this how we want the API to
//     function?
void
sentry_logger_trace(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LOG_LEVEL_TRACE, message, args);
    va_end(args);
}

void
sentry_logger_debug(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LOG_LEVEL_DEBUG, message, args);
    va_end(args);
}

void
sentry_logger_info(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LOG_LEVEL_INFO, message, args);
    va_end(args);
}

void
sentry_logger_warn(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LOG_LEVEL_WARN, message, args);
    va_end(args);
}

void
sentry_logger_error(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LOG_LEVEL_ERROR, message, args);
    va_end(args);
}

void
sentry_logger_fatal(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    sentry__logs_log(SENTRY_LOG_LEVEL_FATAL, message, args);
    va_end(args);
}
