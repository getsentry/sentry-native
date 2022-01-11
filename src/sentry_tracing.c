#include "sentry_tracing.h"
#include "sentry_alloc.h"
#include "sentry_value.h"

sentry_transaction_context_t *
sentry_transaction_context_new(const char *name, const char *operation)
{
    sentry_transaction_context_t *tx_cxt
        = SENTRY_MAKE(sentry_transaction_context_t);
    if (!tx_cxt) {
        return NULL;
    }
    memset(tx_cxt, 0, sizeof(sentry_transaction_context_t));

    sentry_value_t inner
        = sentry__value_transaction_context_new(name, operation);

    tx_cxt->inner = inner;

    return tx_cxt;
}

sentry_transaction_context_t *
sentry__transaction_context_incref(sentry_transaction_context_t *tx_cxt)
{
    sentry_value_incref(tx_cxt->inner);
}

void
sentry__transaction_context_decref(sentry_transaction_context_t *tx_cxt)
{
    if (!tx_cxt) {
        return;
    }
    if (sentry_value_refcount(tx_cxt->inner) == 1) {
        sentry_value_decref(tx_cxt->inner);
        sentry_free(tx_cxt);
    };
}

void
sentry_transaction_context_set_name(
    sentry_transaction_context_t *tx_cxt, const char *name)
{
    sentry__value_transaction_context_set_name(tx_cxt->inner, name);
}

void
sentry_transaction_context_set_operation(
    sentry_transaction_context_t *tx_cxt, const char *operation)
{
    sentry__value_transaction_context_set_operation(tx_cxt->inner, operation);
}

void
sentry_transaction_context_set_sampled(
    sentry_transaction_context_t *tx_cxt, int sampled)
{
    sentry__value_transaction_context_set_sampled(tx_cxt->inner, sampled);
}

void
sentry_transaction_context_remove_sampled(sentry_transaction_context_t *tx_cxt)
{
    sentry__value_transaction_context_remove_sampled(tx_cxt->inner);
}

sentry_transaction_t *
sentry__transaction_new(sentry_value_t inner)
{
    sentry_transaction_t *tx = SENTRY_MAKE(sentry_transaction_t);
    if (!tx) {
        return NULL;
    }
    memset(tx, 0, sizeof(sentry_transaction_t));

    tx->inner = inner;

    return tx;
}

sentry_transaction_t *
sentry__transaction_incref(sentry_transaction_t *tx)
{
    sentry_value_incref(tx->inner);
}

void
sentry__transaction_decref(sentry_transaction_t *tx)
{
    if (!tx) {
        return;
    }
    if (sentry_value_refcount(tx->inner) == 1) {
        sentry_value_decref(tx->inner);
        sentry_free(tx);
    };
}

sentry_span_t *
sentry__span_new(sentry_value_t inner)
{
    sentry_span_t *span = SENTRY_MAKE(sentry_span_t);
    if (!span) {
        return NULL;
    }
    memset(span, 0, sizeof(sentry_span_t));

    span->inner = inner;

    return span;
}

sentry_span_t *
sentry__span_incref(sentry_span_t *span)
{
    sentry_value_incref(span->inner);
}

void
sentry__span_decref(sentry_span_t *span)
{
    if (!span) {
        return;
    }
    if (sentry_value_refcount(span->inner) == 1) {
        sentry_value_decref(span->inner);
        sentry_free(span);
    };
}

sentry_value_t
sentry__span_get_trace_context(sentry_value_t span)
{
    if (sentry_value_is_null(span)
        || sentry_value_is_null(sentry_value_get_by_key(span, "trace_id"))
        || sentry_value_is_null(sentry_value_get_by_key(span, "span_id"))) {
        return sentry_value_new_null();
    }

    sentry_value_t trace_context = sentry_value_new_object();

#define PLACE_VALUE(Key, Source)                                               \
    do {                                                                       \
        sentry_value_t src = sentry_value_get_by_key(Source, Key);             \
        if (!sentry_value_is_null(src)) {                                      \
            sentry_value_incref(src);                                          \
            sentry_value_set_by_key(trace_context, Key, src);                  \
        }                                                                      \
    } while (0)

    PLACE_VALUE("trace_id", span);
    PLACE_VALUE("span_id", span);
    PLACE_VALUE("parent_span_id", span);
    PLACE_VALUE("op", span);
    PLACE_VALUE("description", span);
    PLACE_VALUE("status", span);

    // TODO: freeze this
    return trace_context;

#undef PLACE_VALUE
}
