#ifndef SENTRY_CRASHPAD_BACKEND_H_INCLUDED
#define SENTRY_CRASHPAD_BACKEND_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../sentry_boot.h"

#include "../sentry_backend.h"

sentry_backend_t *sentry__new_crashpad_backend(void);

#ifdef __cplusplus
}
#endif
#endif
