#ifndef SENTRY_LOGS_H_INCLUDED
#define SENTRY_LOGS_H_INCLUDED

#include "sentry_boot.h"

/**
 * Sentry levels for events and breadcrumbs.
 * TODO should these differ from sentry_level_e? (has no `trace` level)
 */
typedef enum sentry_log_level_e {
    SENTRY_LOG_LEVEL_TRACE,
    SENTRY_LOG_LEVEL_DEBUG,
    SENTRY_LOG_LEVEL_INFO,
    SENTRY_LOG_LEVEL_WARN,
    SENTRY_LOG_LEVEL_ERROR,
    SENTRY_LOG_LEVEL_FATAL
} sentry_log_level_t;

void sentry__logs_log(
    sentry_log_level_t level, const char *message, va_list args);

// TODO again, think about API (functions vs Macros)
void sentry_logger_trace(const char *message, ...);
void sentry_logger_debug(const char *message, ...);
void sentry_logger_info(const char *message, ...);
void sentry_logger_warn(const char *message, ...);
void sentry_logger_error(const char *message, ...);
void sentry_logger_fatal(const char *message, ...);

#define SENTRY_LOGGER_TRACE(message, ...)                                      \
    sentry__logs_log(SENTRY_LOG_LEVEL_TRACE, message, __VA_ARGS__)

#define SENTRY_LOGGER_DEBUG(message, ...)                                      \
    sentry__logs_log(SENTRY_LOG_LEVEL_DEBUG, message, __VA_ARGS__)

#define SENTRY_LOGGER_INFO(message, ...)                                       \
    sentry__logs_log(SENTRY_LOG_LEVEL_INFO, message, __VA_ARGS__)

#define SENTRY_LOGGER_WARN(message, ...)                                       \
    sentry__logs_log(SENTRY_LOG_LEVEL_WARN, message, __VA_ARGS__)

#define SENTRY_LOGGER_ERROR(message, ...)                                      \
    sentry__logs_log(SENTRY_LOG_LEVEL_ERROR, message, __VA_ARGS__)

#define SENTRY_LOGGER_FATAL(message, ...)                                      \
    sentry__logs_log(SENTRY_LOG_LEVEL_FATAL, message, __VA_ARGS__)

#endif
