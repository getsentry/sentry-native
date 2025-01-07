// sentry_sampling_context.c
#include "sentry_sampling_context.h"
#include <sentry_alloc.h>

sentry_sampling_context_t *
sentry_sampling_context_new(sentry_transaction_context_t *transaction_context,
    sentry_value_t custom_sampling_context)
{
    sentry_sampling_context_t *context = SENTRY_MAKE(sentry_sampling_context_t);
    if (context) {
        context->transaction_context = transaction_context; // todo incref?
        context->custom_sampling_context = custom_sampling_context;
    }
    return context;
}

sentry_value_t
sentry_sampling_context_get_custom_context(
    const sentry_sampling_context_t *sampling_ctx)
{
    return sampling_ctx->custom_sampling_context;
}
