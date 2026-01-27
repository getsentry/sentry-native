#ifndef SENTRY_METRICS_H_INCLUDED
#define SENTRY_METRICS_H_INCLUDED

#include "sentry_boot.h"

/**
 * Sets up the metrics timer/flush thread
 */
void sentry__metrics_startup(void);

/**
 * Instructs the metrics timer/flush thread to shut down.
 */
void sentry__metrics_shutdown(uint64_t timeout);

/**
 * Crash-safe metrics flush that avoids thread synchronization.
 * This should be used during crash handling to flush metrics without
 * waiting for the batching thread to shut down cleanly.
 */
void sentry__metrics_flush_crash_safe(void);

void sentry__metrics_force_flush(void);

#ifdef SENTRY_UNITTEST
/**
 * Wait for the metrics batching thread to be ready.
 * This is a test-only helper to avoid race conditions in tests.
 */
void sentry__metrics_wait_for_thread_startup(void);
#endif

#endif
