// sentry_sampling_context.c
#include "sentry_sampling_context.h"
#include <sentry_alloc.h>
#include <stdlib.h>

sentry_sampling_context_t *sentry_sampling_context_new(sentry_transaction_context_t *transaction_context) {
    sentry_sampling_context_t *context = SENTRY_MAKE(sentry_sampling_context_t);
    if (context) {
        context->transaction_context = transaction_context; // todo incref?
        context->custom_sampling_context = NULL;
    }
    return context;
}

sentry_sampling_context_t *sentry_sampling_context_new_with_custom(sentry_transaction_context_t *transaction_context, void *custom_sampling_context) {
    sentry_sampling_context_t *context = sentry_sampling_context_new(transaction_context);
    if (context) {
        context->custom_sampling_context = custom_sampling_context;
    }
    return context;
}