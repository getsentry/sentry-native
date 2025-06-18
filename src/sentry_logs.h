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

#endif
