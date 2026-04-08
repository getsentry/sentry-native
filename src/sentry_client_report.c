#include "sentry_client_report.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_ratelimiter.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_utils.h"
#include "sentry_value.h"

// Counters for discarded events, indexed by [reason][category]
static volatile long g_discard_counts[SENTRY_DISCARD_REASON_MAX]
                                     [SENTRY_DATA_CATEGORY_MAX]
    = { { 0 } };

static const char *
discard_reason_to_string(sentry_discard_reason_t reason)
{
    switch (reason) {
    case SENTRY_DISCARD_REASON_QUEUE_OVERFLOW:
        return "queue_overflow";
    case SENTRY_DISCARD_REASON_RATELIMIT_BACKOFF:
        return "ratelimit_backoff";
    case SENTRY_DISCARD_REASON_NETWORK_ERROR:
        return "network_error";
    case SENTRY_DISCARD_REASON_SAMPLE_RATE:
        return "sample_rate";
    case SENTRY_DISCARD_REASON_BEFORE_SEND:
        return "before_send";
    case SENTRY_DISCARD_REASON_EVENT_PROCESSOR:
        return "event_processor";
    case SENTRY_DISCARD_REASON_SEND_ERROR:
        return "send_error";
    case SENTRY_DISCARD_REASON_MAX:
    default:
        return "unknown";
    }
}

static const char *
data_category_to_string(sentry_data_category_t category)
{
    switch (category) {
    case SENTRY_DATA_CATEGORY_ERROR:
        return "error";
    case SENTRY_DATA_CATEGORY_SESSION:
        return "session";
    case SENTRY_DATA_CATEGORY_TRANSACTION:
        return "transaction";
    case SENTRY_DATA_CATEGORY_ATTACHMENT:
        return "attachment";
    case SENTRY_DATA_CATEGORY_LOG_ITEM:
        return "log_item";
    case SENTRY_DATA_CATEGORY_FEEDBACK:
        return "feedback";
    case SENTRY_DATA_CATEGORY_TRACE_METRIC:
        return "trace_metric";
    case SENTRY_DATA_CATEGORY_MAX:
    default:
        return "unknown";
    }
}

void
sentry__client_report_discard(sentry_discard_reason_t reason,
    sentry_data_category_t category, long quantity)
{
    if (reason >= SENTRY_DISCARD_REASON_MAX
        || category >= SENTRY_DATA_CATEGORY_MAX || quantity <= 0) {
        return;
    }

    sentry__atomic_fetch_and_add(
        (long *)&g_discard_counts[reason][category], quantity);
}

bool
sentry__client_report_has_pending(void)
{
    for (int r = 0; r < SENTRY_DISCARD_REASON_MAX; r++) {
        for (int c = 0; c < SENTRY_DATA_CATEGORY_MAX; c++) {
            if (sentry__atomic_fetch((long *)&g_discard_counts[r][c]) > 0) {
                return true;
            }
        }
    }
    return false;
}

sentry_envelope_item_t *
sentry__client_report_into_envelope(sentry_envelope_t *envelope)
{
    if (!envelope || !sentry__client_report_has_pending()) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry__jsonwriter_write_object_start(jw);
    sentry__jsonwriter_write_key(jw, "timestamp");
    char *timestamp = sentry__usec_time_to_iso8601(sentry__usec_time());
    sentry__jsonwriter_write_str(jw, timestamp);
    sentry_free(timestamp);

    sentry__jsonwriter_write_key(jw, "discarded_events");
    sentry__jsonwriter_write_list_start(jw);
    for (int r = 0; r < SENTRY_DISCARD_REASON_MAX; r++) {
        for (int c = 0; c < SENTRY_DATA_CATEGORY_MAX; c++) {
            long count
                = sentry__atomic_store((long *)&g_discard_counts[r][c], 0);
            if (count > 0) {
                sentry__jsonwriter_write_object_start(jw);
                sentry__jsonwriter_write_key(jw, "reason");
                sentry__jsonwriter_write_str(jw, discard_reason_to_string(r));
                sentry__jsonwriter_write_key(jw, "category");
                sentry__jsonwriter_write_str(jw, data_category_to_string(c));
                sentry__jsonwriter_write_key(jw, "quantity");
                sentry__jsonwriter_write_int64(jw, count);
                sentry__jsonwriter_write_object_end(jw);
            }
        }
    }
    sentry__jsonwriter_write_list_end(jw);
    sentry__jsonwriter_write_object_end(jw);

    size_t payload_len = 0;
    char *payload = sentry__jsonwriter_into_string(jw, &payload_len);
    if (!payload) {
        return NULL;
    }

    sentry_envelope_item_t *item = sentry__envelope_add_from_buffer(
        envelope, payload, payload_len, "client_report");
    sentry_free(payload);

    return item;
}

sentry_data_category_t
sentry__item_type_to_data_category(const char *ty)
{
    if (sentry__string_eq(ty, "session")) {
        return SENTRY_DATA_CATEGORY_SESSION;
    } else if (sentry__string_eq(ty, "transaction")) {
        return SENTRY_DATA_CATEGORY_TRANSACTION;
    } else if (sentry__string_eq(ty, "attachment")) {
        return SENTRY_DATA_CATEGORY_ATTACHMENT;
    } else if (sentry__string_eq(ty, "log")) {
        return SENTRY_DATA_CATEGORY_LOG_ITEM;
    } else if (sentry__string_eq(ty, "feedback")) {
        return SENTRY_DATA_CATEGORY_FEEDBACK;
    } else if (sentry__string_eq(ty, "trace_metric")) {
        return SENTRY_DATA_CATEGORY_TRACE_METRIC;
    }
    return SENTRY_DATA_CATEGORY_ERROR;
}

void
sentry__client_report_discard_envelope(const sentry_envelope_t *envelope,
    sentry_discard_reason_t reason, const sentry_rate_limiter_t *rl)
{
    size_t count = sentry__envelope_get_item_count(envelope);
    for (size_t i = 0; i < count; i++) {
        const sentry_envelope_item_t *item
            = sentry__envelope_get_item(envelope, i);
        const char *ty = sentry_value_as_string(
            sentry__envelope_item_get_header(item, "type"));
        int rl_category = sentry__envelope_item_type_to_rl_category(ty);
        // internal items (e.g. client_report) bypass rate limiting
        if (rl_category < 0) {
            continue;
        }
        // already recorded as ratelimit_backoff
        if (rl && sentry__rate_limiter_is_disabled(rl, rl_category)) {
            continue;
        }
        sentry__client_report_discard(
            reason, sentry__item_type_to_data_category(ty), 1);
    }
}

void
sentry__client_report_reset(void)
{
    for (int r = 0; r < SENTRY_DISCARD_REASON_MAX; r++) {
        for (int c = 0; c < SENTRY_DATA_CATEGORY_MAX; c++) {
            sentry__atomic_store((long *)&g_discard_counts[r][c], 0);
        }
    }
}
