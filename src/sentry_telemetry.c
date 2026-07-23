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

    if (!options->enable_logs && !options->enable_metrics) {
        sentry__mutex_unlock(&g_telemetry_lock);
        return;
    }

    // use two workers for serializing telemetry batches off the batcher threads
    g_telemetry_pool = sentry__threadpool_new(2);
    if (!g_telemetry_pool) {
        SENTRY_WARN("failed to allocate telemetry serialization pool");
        sentry__mutex_unlock(&g_telemetry_lock);
        return;
    }
    if (sentry__threadpool_start(g_telemetry_pool) != 0) {
        SENTRY_WARN("failed to start telemetry serialization pool");
        sentry__threadpool_free(g_telemetry_pool);
        g_telemetry_pool = NULL;
        sentry__mutex_unlock(&g_telemetry_lock);
        return;
    }

    if (options->enable_logs) {
        sentry__logs_startup(options, g_telemetry_pool);
    }
    if (options->enable_metrics) {
        sentry__metrics_startup(options, g_telemetry_pool);
    }
    sentry__mutex_unlock(&g_telemetry_lock);
}

void
sentry__telemetry_shutdown(uint64_t timeout)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_telemetry_lock);
    sentry__mutex_lock(&g_telemetry_lock);

    sentry__logs_shutdown(timeout);
    sentry__metrics_shutdown(timeout);
    sentry__threadpool_shutdown(g_telemetry_pool);
    sentry__threadpool_free(g_telemetry_pool);
    g_telemetry_pool = NULL;

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
    sentry__logs_flush_crash_safe();
    sentry__metrics_flush_crash_safe();
}
