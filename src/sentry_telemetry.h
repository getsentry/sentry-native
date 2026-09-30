#ifndef SENTRY_TELEMETRY_H_INCLUDED
#define SENTRY_TELEMETRY_H_INCLUDED

#include "sentry_boot.h"

void sentry__telemetry_startup(const sentry_options_t *options);
void sentry__telemetry_shutdown(const sentry_options_t *options);
void sentry__telemetry_force_flush(void);
void sentry__telemetry_flush_crash_safe(void);

#ifdef SENTRY_UNITTEST
#    include "sentry_batcher.h"

/**
 * Returns an owned reference to the telemetry batcher, or NULL.
 * The caller must release it with sentry__batcher_release.
 */
sentry_batcher_t *sentry__telemetry_get_batcher(void);
#endif

#endif
