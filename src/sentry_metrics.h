#ifndef SENTRY_METRICS_H_INCLUDED
#define SENTRY_METRICS_H_INCLUDED

#include "sentry_boot.h"

/**
 * Sets up the metrics timer/flush thread
 */
void sentry__metrics_startup(void);

/**
 * Begin non-blocking shutdown of the metrics timer/flush thread.
 */
bool sentry__metrics_shutdown_begin(void);

/**
 * Wait for the metrics timer/flush thread to complete shutdown.
 * Should only be called if sentry__metrics_shutdown_begin returned true.
 */
void sentry__metrics_shutdown_wait(uint64_t timeout);

/**
 * Crash-safe metrics flush that avoids thread synchronization.
 * This should be used during crash handling to flush metrics without
 * waiting for the batching thread to shut down cleanly.
 */
void sentry__metrics_flush_crash_safe(void);

/**
 * Begin non-blocking force flush of metrics.
 */
void sentry__metrics_force_flush_begin(void);

/**
 * Wait for the metrics force flush to complete.
 */
void sentry__metrics_force_flush_wait(void);

#ifdef SENTRY_UNITTEST
/**
 * Wait for the metrics batching thread to be ready.
 * This is a test-only helper to avoid race conditions in tests.
 */
void sentry__metrics_wait_for_thread_startup(void);
#endif

#endif
