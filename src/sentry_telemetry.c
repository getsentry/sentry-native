#include "sentry_telemetry.h"
#include "sentry_batcher.h"
#include "sentry_logger.h"
#include "sentry_logs.h"
#include "sentry_metrics.h"
#include "sentry_options.h"
#include "sentry_sync.h"

static sentry_threadpool_t *g_telemetry_pool = NULL;
#ifdef SENTRY__MUTEX_INIT_DYN
SENTRY__MUTEX_INIT_DYN(g_telemetry_lock)
#else
static sentry_mutex_t g_telemetry_lock = SENTRY__MUTEX_INIT;
#endif

void
sentry__telemetry_startup(const sentry_options_t *options)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_telemetry_lock);
    sentry__mutex_lock(&g_telemetry_lock);

    // use two workers for serializing telemetry batches off the batcher threads
    // and cap to 10x100 batches to respect the max 1000-item buffer limit:
    // https://develop.sentry.dev/sdk/telemetry/logs/#buffering
    g_telemetry_pool = sentry__threadpool_new(2, 10);
    sentry__threadpool_setname(g_telemetry_pool, "sentry-tele");
    if (!g_telemetry_pool || sentry__threadpool_start(g_telemetry_pool) != 0) {
        sentry__threadpool_free(g_telemetry_pool);
        g_telemetry_pool = NULL;
        SENTRY_WARN(
            "telemetry pool unavailable; serializing in batcher thread");
    }

    sentry__logs_startup(options, g_telemetry_pool);
    sentry__metrics_startup(options, g_telemetry_pool);
    sentry__mutex_unlock(&g_telemetry_lock);
}

void
sentry__telemetry_shutdown(const sentry_options_t *options)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_telemetry_lock);
    sentry__mutex_lock(&g_telemetry_lock);

    SENTRY_DEBUG("shutting down telemetry");
    sentry__logs_shutdown(options->shutdown_timeout);
    sentry__metrics_shutdown(options->shutdown_timeout);
    sentry__threadpool_flush(g_telemetry_pool);
    sentry__threadpool_shutdown(g_telemetry_pool);
    sentry__threadpool_free(g_telemetry_pool);
    g_telemetry_pool = NULL;
    SENTRY_DEBUG("telemetry shutdown complete");

    sentry__mutex_unlock(&g_telemetry_lock);
}

void
sentry__telemetry_force_flush(void)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_telemetry_lock);
    sentry__mutex_lock(&g_telemetry_lock);

    uintptr_t ltoken = sentry__logs_force_flush_begin();
    uintptr_t mtoken = sentry__metrics_force_flush_begin();
    sentry__logs_force_flush_wait(ltoken);
    sentry__metrics_force_flush_wait(mtoken);

    sentry__mutex_unlock(&g_telemetry_lock);
}

void
sentry__telemetry_flush_crash_safe(void)
{
    SENTRY_SIGNAL_SAFE_LOG("DEBUG crash-safe telemetry flush");
    sentry__logs_flush_crash_safe();
    sentry__metrics_flush_crash_safe();
    SENTRY_SIGNAL_SAFE_LOG("DEBUG crash-safe telemetry flush complete");
}
