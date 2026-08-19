#include "sentry_telemetry.h"
#include "sentry_logger.h"
#include "sentry_logs.h"
#include "sentry_metrics.h"
#include "sentry_options.h"

void
sentry__telemetry_startup(const sentry_options_t *options)
{
    if (options->enable_logs) {
        sentry__logs_startup(options);
    }
    if (options->enable_metrics) {
        sentry__metrics_startup(options);
    }
}

void
sentry__telemetry_shutdown(const sentry_options_t *options)
{
    if (!options->enable_logs && !options->enable_metrics) {
        return;
    }

    SENTRY_DEBUG("shutting down telemetry");
    sentry__logs_shutdown(options->shutdown_timeout);
    sentry__metrics_shutdown(options->shutdown_timeout);
    SENTRY_DEBUG("telemetry shutdown complete");
}

void
sentry__telemetry_force_flush(void)
{
    uintptr_t ltoken = sentry__logs_force_flush_begin();
    uintptr_t mtoken = sentry__metrics_force_flush_begin();
    sentry__logs_force_flush_wait(ltoken);
    sentry__metrics_force_flush_wait(mtoken);
}

void
sentry__telemetry_flush_crash_safe(void)
{
    SENTRY_SIGNAL_SAFE_LOG("DEBUG crash-safe telemetry flush");
    sentry__logs_flush_crash_safe();
    sentry__metrics_flush_crash_safe();
    SENTRY_SIGNAL_SAFE_LOG("DEBUG crash-safe telemetry flush complete");
}
