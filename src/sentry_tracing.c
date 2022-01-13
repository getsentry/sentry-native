#include "sentry_boot.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_string.h"
#include "sentry_tracing.h"
#include "sentry_utils.h"
#include "sentry_value.h"
#include <string.h>

sentry_value_t
sentry__value_new_span(sentry_value_t parent, const char *operation)
{
    sentry_value_t span = sentry_value_new_object();

    sentry_value_set_by_key(span, "op", sentry_value_new_string(operation));

    sentry_uuid_t span_id = sentry_uuid_new_v4();
    sentry_value_set_by_key(
        span, "span_id", sentry__value_new_span_uuid(&span_id));

    sentry_value_set_by_key(span, "status", sentry_value_new_string("ok"));

    // Span creation is currently aggressively pruned prior to this function so
    // once we're in here we definitely know that the span and its parent
    // transaction are sampled.
    // Sampling decisions inherited from traces created in other SDKs should be
    // taken care of `continue_from_headers`, spans don't need to worry about
    // (inheriting) forced sampling decisions, and transactions cannot be
    // children of other transactions, so no inheriting of the sampling field is
    // needed.
    if (!sentry_value_is_null(parent)) {
        sentry_value_set_by_key(span, "trace_id",
            sentry_value_get_by_key_owned(parent, "trace_id"));
        sentry_value_set_by_key(span, "parent_span_id",
            sentry_value_get_by_key_owned(parent, "span_id"));
    }

    return span;
}

sentry_value_t
sentry__value_transaction_context_new(const char *name, const char *operation)
{
    sentry_value_t transaction_context
        = sentry__value_new_span(sentry_value_new_null(), operation);

    sentry_uuid_t trace_id = sentry_uuid_new_v4();
    sentry_value_set_by_key(transaction_context, "trace_id",
        sentry__value_new_internal_uuid(&trace_id));

    sentry_value_set_by_key(
        transaction_context, "transaction", sentry_value_new_string(name));

    return transaction_context;
}

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

    if (sentry_value_is_null(inner)) {
        return NULL;
    }

    tx_cxt->inner = inner;

    return tx_cxt;
}

void
sentry__transaction_context_free(sentry_transaction_context_t *tx_cxt)
{
    if (!tx_cxt) {
        return;
    }
    if (sentry_value_refcount(tx_cxt->inner) <= 1) {
        sentry_value_decref(tx_cxt->inner);
        sentry_free(tx_cxt);
    } else {
        sentry_value_decref(tx_cxt->inner);
    };
}

void
sentry_transaction_context_set_name(
    sentry_transaction_context_t *tx_cxt, const char *name)
{
    sentry_value_set_by_key(
        tx_cxt->inner, "transaction", sentry_value_new_string(name));
}

void
sentry_transaction_context_set_operation(
    sentry_transaction_context_t *tx_cxt, const char *operation)
{
    sentry_value_set_by_key(
        tx_cxt->inner, "op", sentry_value_new_string(operation));
}

void
sentry_transaction_context_set_sampled(
    sentry_transaction_context_t *tx_cxt, int sampled)
{
    sentry_value_set_by_key(
        tx_cxt->inner, "sampled", sentry_value_new_bool(sampled));
}

void
sentry_transaction_context_remove_sampled(sentry_transaction_context_t *tx_cxt)
{
    sentry_value_remove_by_key(tx_cxt->inner, "sampled");
}

sentry_transaction_t *
sentry__transaction_new(sentry_value_t inner)
{
    if (sentry_value_is_null(inner)) {
        return NULL;
    }

    sentry_transaction_t *tx = SENTRY_MAKE(sentry_transaction_t);
    if (!tx) {
        return NULL;
    }
    memset(tx, 0, sizeof(sentry_transaction_t));

    tx->inner = inner;

    return tx;
}

void
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

    if (sentry_value_refcount(tx->inner) <= 1) {
        sentry_value_decref(tx->inner);
        sentry_free(tx);
    } else {
        sentry_value_decref(tx->inner);
    };
}

sentry_span_t *
sentry__span_new(sentry_value_t inner)
{
    if (sentry_value_is_null(inner)) {
        return NULL;
    }

    sentry_span_t *span = SENTRY_MAKE(sentry_span_t);
    if (!span) {
        return NULL;
    }
    memset(span, 0, sizeof(sentry_span_t));

    span->inner = inner;

    return span;
}

sentry_span_t *
sentry__start_child(
    size_t max_spans, sentry_value_t parent, char *operation, char *description)
{
    if (!sentry_value_is_null(sentry_value_get_by_key(parent, "timestamp"))) {
        SENTRY_DEBUG("span's parent is already finished, not creating span");
        goto fail;
    }

    // Aggressively discard spans if a transaction is unsampled to avoid
    // wasting memory
    sentry_value_t sampled = sentry_value_get_by_key(parent, "sampled");
    if (!sentry_value_is_true(sampled)) {
        SENTRY_DEBUG("span's parent is unsampled, not creating span");
        goto fail;
    }
    sentry_value_t spans = sentry_value_get_by_key(parent, "spans");
    // This only checks that the number of _completed_ spans matches the number
    // of max spans. This means that the number of in-flight spans can exceed
    // the max number of spans.
    if (sentry_value_get_length(spans) >= max_spans) {
        SENTRY_DEBUG("reached maximum number of spans for transaction, not "
                     "creating span");
        goto fail;
    }

    sentry_value_t child = sentry__value_new_span(parent, operation);
    sentry_value_set_by_key(
        child, "description", sentry_value_new_string(description));
    sentry_value_set_by_key(child, "start_timestamp",
        sentry__value_new_string_owned(
            sentry__msec_time_to_iso8601(sentry__msec_time())));
    sentry_value_set_by_key(child, "sampled", sentry_value_new_bool(1));

    return sentry__span_new(child);
fail:
    return NULL;
}

// TODO: for now, don't allow multiple references to spans. this should be
// revisited when sentry_transaction_t stores a list of sentry_span_t's instead
// of a list of sentry_value_t's.
void
sentry__span_free(sentry_span_t *span)
{
    if (!span) {
        return;
    }
    sentry_value_decref(span->inner);
    sentry_free(span);
}

sentry_value_t
sentry__transaction_get_trace_context(sentry_transaction_t *opaque_tx)
{
    if (!opaque_tx || sentry_value_is_null(opaque_tx->inner)) {
        return sentry_value_new_null();
    }

    sentry_value_t tx = opaque_tx->inner;
    if (sentry_value_is_null(sentry_value_get_by_key(tx, "trace_id"))
        || sentry_value_is_null(sentry_value_get_by_key(tx, "span_id"))) {
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

    PLACE_VALUE("trace_id", tx);
    PLACE_VALUE("span_id", tx);
    PLACE_VALUE("parent_span_id", tx);
    PLACE_VALUE("op", tx);
    PLACE_VALUE("description", tx);
    PLACE_VALUE("status", tx);

    // TODO: freeze this
    return trace_context;

#undef PLACE_VALUE
}

void
sentry_transaction_set_name(sentry_transaction_t *tx, const char *name)
{
    sentry_value_set_by_key(
        tx->inner, "transaction", sentry_value_new_string(name));
}

static void
set_tag(sentry_value_t item, const char *tag, const char *value)
{
    sentry_value_t tags = sentry_value_get_by_key(item, "tags");
    if (sentry_value_is_null(tags)) {
        tags = sentry_value_new_object();
        sentry_value_set_by_key(item, "tags", tags);
    }

    char *s = sentry__string_clonen(value, 200);
    if (s) {
        sentry_value_set_by_key(tags, tag, sentry__value_new_string_owned(s));
    } else {
        sentry_value_set_by_key(tags, tag, sentry_value_new_null());
    }
}

void
sentry_transaction_set_tag(
    sentry_transaction_t *tx, const char *tag, const char *value)
{
    set_tag(tx->inner, tag, value);
}

void
sentry_span_set_tag(sentry_span_t *span, const char *tag, const char *value)
{
    set_tag(span->inner, tag, value);
}

static void
remove_tag(sentry_value_t item, const char *tag)
{
    sentry_value_t tags = sentry_value_get_by_key(item, "tags");
    if (!sentry_value_is_null(tags)) {
        sentry_value_remove_by_key(tags, tag);
    }
}

void
sentry_transaction_remove_tag(sentry_transaction_t *tx, const char *tag)
{
    remove_tag(tx->inner, tag);
}

void
sentry_span_remove_tag(sentry_span_t *span, const char *tag)
{
    remove_tag(span->inner, tag);
}

static void
set_data(sentry_value_t item, const char *key, sentry_value_t value)
{
    sentry_value_t data = sentry_value_get_by_key(item, "data");
    if (sentry_value_is_null(data)) {
        data = sentry_value_new_object();
        sentry_value_set_by_key(item, "data", data);
    }
    sentry_value_set_by_key(data, key, value);
}

void
sentry_transaction_set_data(
    sentry_transaction_t *tx, const char *key, sentry_value_t value)
{
    set_data(tx->inner, key, value);
}

void
sentry_span_set_data(sentry_span_t *span, const char *key, sentry_value_t value)
{
    set_data(span->inner, key, value);
}

static void
remove_data(sentry_value_t item, const char *key)
{
    sentry_value_t data = sentry_value_get_by_key(item, "data");
    if (!sentry_value_is_null(data)) {
        sentry_value_remove_by_key(data, key);
    }
}

void
sentry_transaction_remove_data(sentry_transaction_t *tx, const char *key)
{
    remove_data(tx->inner, key);
}

void
sentry_span_remove_data(sentry_span_t *span, const char *key)
{
    remove_data(span->inner, key);
}
