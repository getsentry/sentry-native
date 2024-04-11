#include "sentry_alloc.h"
#include "sentry_sync.h"
#include <stdlib.h>
#include <string.h>

/* on unix platforms we add support for a simplistic page allocator that can
   be enabled to make code async safe */
#ifdef SENTRY_ENABLE_PAGE_ALLOCATOR
#    include "sentry_unix_pageallocator.h"
#endif

void *
sentry_malloc(size_t size)
{
#ifdef SENTRY_ENABLE_PAGE_ALLOCATOR
    if (sentry__page_allocator_enabled()) {
        return sentry__page_allocator_alloc(size);
    }
#endif
    return malloc(size);
}

void
sentry_free(void *ptr)
{
#ifdef SENTRY_ENABLE_PAGE_ALLOCATOR
    /* page allocator can't free */
    if (sentry__page_allocator_enabled()) {
        return;
    }
#endif
    free(ptr);
}
