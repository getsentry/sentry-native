#ifndef SENTRY_LOGS_H_INCLUDED
#define SENTRY_LOGS_H_INCLUDED

#include "sentry_boot.h"

void sentry__logs_log(sentry_level_t level, const char *message, va_list args);

/**
 * Sets up the logs timer/flush thread
 */
void sentry__logs_startup(void);

/**
 * Instructs the logs timer/flush thread to shut down.
 */
void sentry__logs_shutdown(uint64_t timeout);

#endif
