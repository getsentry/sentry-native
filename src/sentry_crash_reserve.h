#ifndef SENTRY_CRASH_RESERVE_H_INCLUDED
#define SENTRY_CRASH_RESERVE_H_INCLUDED

#include "sentry_boot.h"

/**
 * Reserves `size` bytes of committed memory for the crash handler, replacing
 * any previous reserve.
 */
void sentry__crash_reserve_alloc(size_t size);

/**
 * Releases the reserve. Safe to call from a crash handler and more than once.
 */
void sentry__crash_reserve_release(void);

/**
 * Returns the size of the memory that is currently reserved, or `0`.
 */
size_t sentry__crash_reserve_size(void);

#endif
