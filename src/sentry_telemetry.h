#ifndef SENTRY_TELEMETRY_H_INCLUDED
#define SENTRY_TELEMETRY_H_INCLUDED

#include "sentry_boot.h"

void sentry__telemetry_startup(const sentry_options_t *options);
void sentry__telemetry_shutdown(const sentry_options_t *options);
void sentry__telemetry_force_flush(void);
void sentry__telemetry_flush_crash_safe(void);

#endif
