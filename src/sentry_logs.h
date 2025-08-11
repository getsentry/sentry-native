#ifndef SENTRY_LOGS_H_INCLUDED
#define SENTRY_LOGS_H_INCLUDED

#include "sentry_boot.h"

void sentry__logs_log(sentry_level_t level, const char *message, va_list args);

/**
 * Instructs the logs bgworker to shut down.
 *
 * Returns 0 on success.
 */
void sentry__logs_shutdown(uint64_t timeout);

#endif
