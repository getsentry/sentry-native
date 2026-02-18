#ifndef SENTRY_ALLOC_H_INCLUDED
#define SENTRY_ALLOC_H_INCLUDED

#include "sentry_boot.h"

/**
 * This is a shortcut for a typed `malloc`.
 */
#define SENTRY_MAKE(Type) (Type *)sentry_malloc(sizeof(Type))

/**
 * This is a typed `calloc` that zero-initializes the allocation.
 */
void *sentry__calloc(size_t count, size_t size);
#define SENTRY_MAKE_0(Type) (Type *)sentry__calloc(1, sizeof(Type))

#endif
