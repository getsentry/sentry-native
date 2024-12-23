// sentry_sampling_context.h
#ifndef SENTRY_SAMPLING_CONTEXT_H_INCLUDED
#define SENTRY_SAMPLING_CONTEXT_H_INCLUDED

#include "sentry_tracing.h"

typedef struct sentry_sampling_context_s {
    sentry_transaction_context_t *transaction_context;
    void *custom_sampling_context; // TODO what type should this be?
} sentry_sampling_context_t;

// TODO add refcounting for this; users might want to reuse this
sentry_sampling_context_t *sentry_sampling_context_new(
    sentry_transaction_context_t *transaction_context);
sentry_sampling_context_t *sentry_sampling_context_new_with_custom(
    sentry_transaction_context_t *transaction_context,
    void *custom_sampling_context);

#endif // SENTRY_SAMPLING_CONTEXT_H_INCLUDED