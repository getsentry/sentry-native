#include "sentry_value.h"

sentry_value_t
sentry__span_get_trace_context(sentry_value_t span)
{
    if (sentry_value_is_null(span)
        || sentry_value_is_null(sentry_value_get_by_key(span, "trace_id"))
        || sentry_value_is_null(sentry_value_get_by_key(span, "span_id"))) {
        return sentry_value_new_null();
    }

    sentry_value_t trace_context = sentry_value_new_object();

#define PLACE_CLONED_VALUE(Key, Source)                                        \
    do {                                                                       \
        sentry_value_t src = sentry_value_get_by_key(Source, Key);             \
        if (!sentry_value_is_null(src)) {                                      \
            sentry_value_set_by_key(                                           \
                trace_context, Key, sentry__value_clone(src));                 \
        }                                                                      \
    } while (0)

    PLACE_CLONED_VALUE("trace_id", span);
    PLACE_CLONED_VALUE("span_id", span);
    PLACE_CLONED_VALUE("parent_span_id", span);
    PLACE_CLONED_VALUE("op", span);
    PLACE_CLONED_VALUE("description", span);
    PLACE_CLONED_VALUE("status", span);

    // TODO: freeze this
    return trace_context;

#undef PLACE_CLONED_VALUE
}

sentry_value_t
sentry__span_get_span_context(sentry_value_t span)
{
    if (sentry_value_is_null(span)
        || sentry_value_is_null(sentry_value_get_by_key(span, "trace_id"))
        || sentry_value_is_null(sentry_value_get_by_key(span, "span_id"))) {
        return sentry_value_new_null();
    }

    sentry_value_t span_context = sentry_value_new_object();

#define PLACE_CLONED_VALUE(Key, Source)                                        \
    do {                                                                       \
        sentry_value_t src = sentry_value_get_by_key(Source, Key);             \
        if (!sentry_value_is_null(src)) {                                      \
            sentry_value_set_by_key(                                           \
                span_context, Key, sentry__value_clone(src));                  \
        }                                                                      \
    } while (0)

    PLACE_CLONED_VALUE("trace_id", span);
    PLACE_CLONED_VALUE("span_id", span);
    PLACE_CLONED_VALUE("status", span);

    return span_context;

#undef PLACE_CLONED_VALUE
}
