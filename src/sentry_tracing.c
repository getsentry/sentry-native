#include "sentry_tracing.h"
#include "sentry_alloc.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include <stdlib.h>

sentry_transaction_t *
sentry__transaction_incref(sentry_transaction_t *tx)
{
    // if (options) {
    //     sentry__atomic_fetch_and_add(&options->refcount, 1);
    // }
    // return options;
}

void
sentry__transaction_decref(sentry_transaction_t *tx)
{
    // if (!dsn) {
    //     return;
    // }
    // if (sentry__atomic_fetch_and_add(&dsn->refcount, -1) == 1) {
    //     sentry_free(dsn->raw);
    //     sentry_free(dsn->host);
    //     sentry_free(dsn->path);
    //     sentry_free(dsn->public_key);
    //     sentry_free(dsn->secret_key);
    //     sentry_free(dsn);
    // }
}

sentry_span_t *
sentry__span_incref(sentry_span_t *tx)
{
    // if (options) {
    //     sentry__atomic_fetch_and_add(&options->refcount, 1);
    // }
    // return options;
}

void
sentry__span_decref(sentry_span_t *tx)
{
    // if (!dsn) {
    //     return;
    // }
    // if (sentry__atomic_fetch_and_add(&dsn->refcount, -1) == 1) {
    //     sentry_free(dsn->raw);
    //     sentry_free(dsn->host);
    //     sentry_free(dsn->path);
    //     sentry_free(dsn->public_key);
    //     sentry_free(dsn->secret_key);
    //     sentry_free(dsn);
    // }
}

sentry_span_context_t *
sentry__new_span_context(sentry_span_context_t *parent, const char *operation)
{
    sentry_span_context_t *span_ctx = SENTRY_MAKE(sentry_span_context_t);
    if (!span_ctx) {
        return NULL;
    }
    memset(span_ctx, 0, sizeof(sentry_span_context_t));

    span_ctx->op = sentry__string_clone(operation);
    span_ctx->span_id = sentry_uuid_new_v4();
    span_ctx->status = SENTRY_SPAN_STATUS_OK;
    span_ctx->trace_id = sentry_uuid_nil();
    span_ctx->span_id = sentry_uuid_nil();
    span_ctx->parent_span_id = sentry_uuid_nil(); // nullable
    span_ctx->tags = sentry_value_new_null();
    span_ctx->data = sentry_value_new_null();
    // Don't set timestamps. Make it the responsibility of the caller since this
    // isn't "starting" anything.

    // Span creation is currently aggressively pruned prior to this function so
    // once we're in here we definitely know that the span and its parent
    // transaction are sampled.
    // Sampling decisions inherited from traces created in other SDKs should be
    // taken care of `continue_from_headers`, spans don't need to worry about
    // (inheriting) forced sampling decisions, and transactions cannot be
    // children of other transactions, so no inheriting of the sampling field is
    // needed.
    if (parent) {
        // jank way of cloning uuids
        char trace_id_bytes[16];
        sentry_uuid_as_bytes(&parent->trace_id, trace_id_bytes);
        span_ctx->trace_id = sentry_uuid_from_bytes(trace_id_bytes);

        char span_id_bytes[16];
        sentry_uuid_as_bytes(&parent->span_id, span_id_bytes);
        span_ctx->span_id = sentry_uuid_from_bytes(span_id_bytes);
    } else {
        span_ctx->trace_id = sentry_uuid_new_v4();
    }

    return span_ctx;
}

sentry_value_t
sentry__span_get_trace_context(sentry_span_t *span)
{
    //     if (sentry_value_is_null(span)
    //         || sentry_value_is_null(sentry_value_get_by_key(span,
    //         "trace_id"))
    //         || sentry_value_is_null(sentry_value_get_by_key(span,
    //         "span_id"))) { return sentry_value_new_null();
    //     }

    //     sentry_value_t trace_context = sentry_value_new_object();

    // #define PLACE_VALUE(Key, Source)                                               \
//     do {                                                                       \
//         sentry_value_t src = sentry_value_get_by_key(Source, Key);             \
//         if (!sentry_value_is_null(src)) {                                      \
//             sentry_value_incref(src);                                          \
//             sentry_value_set_by_key(trace_context, Key, src);                  \
//         }                                                                      \
//     } while (0)

    //     PLACE_VALUE("trace_id", span);
    //     PLACE_VALUE("span_id", span);
    //     PLACE_VALUE("parent_span_id", span);
    //     PLACE_VALUE("op", span);
    //     PLACE_VALUE("description", span);
    //     PLACE_VALUE("status", span);

    //     // TODO: freeze this
    //     return trace_context;

    // #undef PLACE_VALUE
}
