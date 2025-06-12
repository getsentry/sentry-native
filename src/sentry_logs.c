#include "sentry_logs.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_options.h"

#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <stdio.h>

char *
log_level_as_string(sentry_log_level_t level)
{
    switch (level) {
    case SENTRY_LOG_LEVEL_TRACE:
        return "TRACE";
    case SENTRY_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case SENTRY_LOG_LEVEL_INFO:
        return "INFO";
    case SENTRY_LOG_LEVEL_WARN:
        return "WARN";
    case SENTRY_LOG_LEVEL_ERROR:
        return "ERROR";
    case SENTRY_LOG_LEVEL_FATAL:
        return "FATAL";
    }
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
    sentry_value_t log = sentry_value_new_object();
    sentry_value_set_by_key(log, "body", sentry_value_new_string(message));
    sentry_value_set_by_key(
        log, "level", sentry_value_new_string(log_level_as_string(level)));
    // TODO add actual trace_id
    sentry_value_set_by_key(log, "trace_id",
        sentry_value_new_string("5b8efff798038103d269b633813fc60c"));
    // timestamp in seconds
    sentry_value_set_by_key(log, "timestamp",
        sentry_value_new_double(sentry__usec_time() / 1000000.0));
    // TODO add log attributes
    //    https://develop.sentry.dev/sdk/telemetry/logs/#default-attributes

    // TODO split up the code below for batched log sending
    //    e.g. could we store logs on the scope?
    sentry_value_t logs = sentry_value_new_object();
    sentry_value_t logs_list = sentry_value_new_list();
    sentry_value_append(logs_list, log);
    sentry_value_set_by_key(logs, "items", logs_list);
    // sending of the envelope
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_logs(envelope, logs);
    sentry_envelope_write_to_file(envelope, "logs_envelope.json");
    SENTRY_WITH_OPTIONS (options) {
        sentry__capture_envelope(options->transport, envelope);
    }
}

// TODO think about the structure below, is this how we want the API to
//     function?
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
