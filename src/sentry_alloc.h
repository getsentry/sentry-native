#ifndef SENTRY_ALLOC_H_INCLUDED
#define SENTRY_ALLOC_H_INCLUDED

#include <sentry.h>

#define SENTRY_MAKE(Type) (Type *)sentry_malloc(sizeof(Type))

#endif