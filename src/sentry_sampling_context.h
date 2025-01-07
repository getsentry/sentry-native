// sentry_sampling_context.h
#ifndef SENTRY_SAMPLING_CONTEXT_H_INCLUDED
#define SENTRY_SAMPLING_CONTEXT_H_INCLUDED

#include "sentry_tracing.h"

typedef struct sentry_sampling_context_s {
    sentry_transaction_context_t *transaction_context;
    sentry_value_t custom_sampling_context;
} sentry_sampling_context_t;

// TODO add refcounting for this?
//  we only construct this internally for one sampling decision,
//  and dont reuse it
sentry_sampling_context_t *sentry_sampling_context_new(
    sentry_transaction_context_t *transaction_context,
    sentry_value_t custom_sampling_context);

#endif // SENTRY_SAMPLING_CONTEXT_H_INCLUDED
