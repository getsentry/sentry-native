#include "sentry_client_report.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
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
    case SENTRY_DATA_CATEGORY_SPAN:
        return "span";
    case SENTRY_DATA_CATEGORY_ATTACHMENT:
        return "attachment";
    case SENTRY_DATA_CATEGORY_LOG_ITEM:
        return "log_item";
    case SENTRY_DATA_CATEGORY_FEEDBACK:
        return "feedback";
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
    for (int reason = 0; reason < SENTRY_DISCARD_REASON_MAX; reason++) {
        for (int category = 0; category < SENTRY_DATA_CATEGORY_MAX;
            category++) {
            if (sentry__atomic_fetch(
                    (long *)&g_discard_counts[reason][category])
                > 0) {
                return true;
            }
        }
    }
    return false;
}

sentry_envelope_item_t *
sentry__client_report_into_envelope(sentry_envelope_t *envelope)
{
    if (!envelope) {
        return NULL;
    }

    long counts[SENTRY_DISCARD_REASON_MAX][SENTRY_DATA_CATEGORY_MAX]
        = { { 0 } };
    bool has_data = false;

    for (int reason = 0; reason < SENTRY_DISCARD_REASON_MAX; reason++) {
        for (int category = 0; category < SENTRY_DATA_CATEGORY_MAX;
            category++) {
            long count = sentry__atomic_store(
                (long *)&g_discard_counts[reason][category], 0);
            counts[reason][category] = count;
            if (count > 0) {
                has_data = true;
            }
        }
    }

    if (!has_data) {
        return NULL;
    }

    sentry_value_t client_report = sentry_value_new_object();

    sentry_value_set_by_key(client_report, "timestamp",
        sentry__value_new_string_owned(
            sentry__usec_time_to_iso8601(sentry__usec_time())));

    sentry_value_t discarded_events = sentry_value_new_list();

    for (int reason = 0; reason < SENTRY_DISCARD_REASON_MAX; reason++) {
        for (int category = 0; category < SENTRY_DATA_CATEGORY_MAX;
            category++) {
            long count = counts[reason][category];
            if (count > 0) {
                sentry_value_t entry = sentry_value_new_object();
                sentry_value_set_by_key(entry, "reason",
                    sentry_value_new_string(discard_reason_to_string(
                        (sentry_discard_reason_t)reason)));
                sentry_value_set_by_key(entry, "category",
                    sentry_value_new_string(data_category_to_string(
                        (sentry_data_category_t)category)));
                sentry_value_set_by_key(
                    entry, "quantity", sentry_value_new_int32((int32_t)count));
                sentry_value_append(discarded_events, entry);
            }
        }
    }

    sentry_value_set_by_key(
        client_report, "discarded_events", discarded_events);

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        sentry_value_decref(client_report);
        return NULL;
    }

    sentry__jsonwriter_write_value(jw, client_report);
    size_t payload_len = 0;
    char *payload = sentry__jsonwriter_into_string(jw, &payload_len);
    sentry_value_decref(client_report);

    if (!payload) {
        return NULL;
    }

    sentry_envelope_item_t *item = sentry__envelope_add_from_buffer(
        envelope, payload, payload_len, "client_report");
    sentry_free(payload);

    return item;
}

static sentry_data_category_t
item_type_to_data_category(const char *ty)
{
    if (sentry__string_eq(ty, "session")) {
        return SENTRY_DATA_CATEGORY_SESSION;
    } else if (sentry__string_eq(ty, "transaction")) {
        return SENTRY_DATA_CATEGORY_TRANSACTION;
    }
    return SENTRY_DATA_CATEGORY_ERROR;
}

void
sentry__client_report_discard_envelope(
    const sentry_envelope_t *envelope, sentry_discard_reason_t reason)
{
    size_t count = sentry__envelope_get_item_count(envelope);
    for (size_t i = 0; i < count; i++) {
        const sentry_envelope_item_t *item
            = sentry__envelope_get_item(envelope, i);
        const char *ty = sentry_value_as_string(
            sentry__envelope_item_get_header(item, "type"));
        if (sentry__string_eq(ty, "client_report")) {
            continue;
        }
        sentry__client_report_discard(
            reason, item_type_to_data_category(ty), 1);
    }
}

void
sentry__client_report_reset(void)
{
    for (int reason = 0; reason < SENTRY_DISCARD_REASON_MAX; reason++) {
        for (int category = 0; category < SENTRY_DATA_CATEGORY_MAX;
            category++) {
            sentry__atomic_store(
                (long *)&g_discard_counts[reason][category], 0);
        }
    }
}
