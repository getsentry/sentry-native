#ifndef SENTRY_UNIX_PAGEALLOCATOR_H_INCLUDED
#define SENTRY_UNIX_PAGEALLOCATOR_H_INCLUDED

#include "../sentry_boot.h"

bool sentry__page_allocator_enabled(void);
void sentry__page_allocator_enable(void);
void *sentry__page_allocator_alloc(size_t size);

#if SENTRY_UNITTEST
void sentry__page_allocator_disable(void);
#endif

#endif