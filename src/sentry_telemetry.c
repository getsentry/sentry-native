#include "sentry_telemetry.h"
#include "sentry_batcher.h"
#include "sentry_logger.h"
#include "sentry_logs.h"
#include "sentry_metrics.h"
#include "sentry_options.h"
#include "sentry_sync.h"

static sentry_threadpool_t *g_telemetry_pool = NULL;
static sentry_batcher_ref_t g_telemetry_batcher = SENTRY_BATCHER_REF_INIT;
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
    if (!g_telemetry_pool) {
        SENTRY_WARN(
            "telemetry pool unavailable; serializing in batcher thread");
    }

    sentry_batcher_t *batcher = sentry__batcher_new(2, g_telemetry_pool);
    if (!batcher) {
        sentry__threadpool_shutdown(g_telemetry_pool);
        sentry__threadpool_free(g_telemetry_pool);
        g_telemetry_pool = NULL;
        SENTRY_WARN("failed to allocate telemetry batcher");
        sentry__mutex_unlock(&g_telemetry_lock);
        return;
    }

    sentry__batcher_set_queue(
        batcher, 0, SENTRY_DATA_CATEGORY_LOG_ITEM, sentry__envelope_add_logs);
    sentry__batcher_set_queue(batcher, 1, SENTRY_DATA_CATEGORY_TRACE_METRIC,
        sentry__envelope_add_metrics);
    sentry__batcher_startup(batcher, options);
    sentry__batcher_swap(&g_telemetry_batcher, batcher);
    sentry__logs_startup(batcher);
    sentry__metrics_startup(batcher);
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
    sentry_batcher_t *batcher
        = sentry__batcher_swap(&g_telemetry_batcher, NULL);
    sentry__batcher_shutdown(batcher, options->shutdown_timeout);
    sentry__threadpool_flush(g_telemetry_pool);
    sentry__threadpool_shutdown(g_telemetry_pool);
    sentry__threadpool_free(g_telemetry_pool);
    g_telemetry_pool = NULL;
    sentry__batcher_release(batcher);
    SENTRY_DEBUG("telemetry shutdown complete");

    sentry__mutex_unlock(&g_telemetry_lock);
}

void
sentry__telemetry_force_flush(void)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_telemetry_lock);
    sentry__mutex_lock(&g_telemetry_lock);

    sentry_batcher_t *batcher = sentry__batcher_acquire(&g_telemetry_batcher);
    if (batcher) {
        sentry__batcher_force_flush(batcher);
        sentry__batcher_release(batcher);
    }

    sentry__mutex_unlock(&g_telemetry_lock);
}

void
sentry__telemetry_flush_crash_safe(void)
{
    SENTRY_SIGNAL_SAFE_LOG("DEBUG crash-safe telemetry flush");
    sentry_batcher_t *batcher = sentry__batcher_pin(&g_telemetry_batcher);
    if (batcher) {
        sentry__batcher_flush_crash_safe(batcher);
        sentry__batcher_unpin(&g_telemetry_batcher);
    }
    SENTRY_SIGNAL_SAFE_LOG("DEBUG crash-safe telemetry flush complete");
}

#ifdef SENTRY_UNITTEST
sentry_batcher_t *
sentry__telemetry_get_batcher(void)
{
    return sentry__batcher_acquire(&g_telemetry_batcher);
}
#endif
