#ifndef SENTRY_INPROC_BACKEND_H_INCLUDED
#define SENTRY_INPROC_BACKEND_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_backend.h"

sentry_backend_t *sentry__new_inproc_backend(void);

#endif
