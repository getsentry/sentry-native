#ifndef SENTRY_INTEGRATION_H_INCLUDED
#define SENTRY_INTEGRATION_H_INCLUDED

#include "sentry_boot.h"

typedef struct sentry_integration_s {
    void *data;

    void (*register_func)(
        void *data, sentry_scope_t *scope, const sentry_options_t *options);
    void (*unregister_func)(
        void *data, sentry_scope_t *scope, const sentry_options_t *options);
    void (*free_func)(void *data);
} sentry_integration_t;

#endif
