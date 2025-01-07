// sentry_sampling_context.c
#include "sentry_sampling_context.h"
#include <sentry_alloc.h>

sentry_value_t
sentry_sampling_context_get_custom_context(
    const sentry_sampling_context_t *sampling_ctx)
{
    return sampling_ctx->custom_sampling_context;
}
