#include "sentry_envelope.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_json.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_ratelimiter.h"
#include "sentry_scope.h"
#include "sentry_string.h"
#include "sentry_transport.h"
#include "sentry_value.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

#define SENTRY_TUS_UPLOAD_THRESHOLD (100 * 1024 * 1024)

struct sentry_envelope_item_s {
    sentry_value_t headers;
    sentry_value_t event;
    char *payload;
    size_t payload_len;
    sentry_envelope_item_t *next;
};

struct sentry_envelope_s {
    bool is_raw;
    union {
        struct {
            sentry_value_t headers;
            sentry_envelope_item_t *first_item;
            sentry_envelope_item_t *last_item;
            size_t item_count;
        } items;
        struct {
            char *payload;
            size_t payload_len;
        } raw;
    } contents;
};

static int envelope_deserialize_into(
    sentry_envelope_t *envelope, const char *buf, size_t buf_len);

int
sentry__envelope_materialize(sentry_envelope_t *envelope)
{
    if (!envelope || !envelope->is_raw) {
        return 0;
    }

    char *raw_payload = envelope->contents.raw.payload;
    size_t raw_payload_len = envelope->contents.raw.payload_len;

    envelope->is_raw = false;
    envelope->contents.items.headers = sentry_value_new_object();
    envelope->contents.items.first_item = NULL;
    envelope->contents.items.last_item = NULL;
    envelope->contents.items.item_count = 0;

    int rv = envelope_deserialize_into(envelope, raw_payload, raw_payload_len);
    sentry_free(raw_payload);
    return rv;
}

static sentry_envelope_item_t *
envelope_add_item(sentry_envelope_t *envelope)
{
    if (sentry__envelope_materialize(envelope) != 0) {
        return NULL;
    }

    // TODO: Envelopes may have at most one event item or one transaction item,
    // and not one of both. Some checking should be done here or in
    // `sentry__envelope_add_[transaction|event]` to ensure this can't happen.

    // Allocate new item
    sentry_envelope_item_t *item = SENTRY_MAKE(sentry_envelope_item_t);
    if (!item) {
        return NULL;
    }

    // Initialize item
    item->headers = sentry_value_new_object();
    item->event = sentry_value_new_null();
    item->payload = NULL;
    item->payload_len = 0;
    item->next = NULL;

    // Append to linked list
    if (envelope->contents.items.last_item) {
        envelope->contents.items.last_item->next = item;
    } else {
        envelope->contents.items.first_item = item;
    }
    envelope->contents.items.last_item = item;
    envelope->contents.items.item_count++;

    return item;
}

static void
envelope_item_cleanup(sentry_envelope_item_t *item)
{
    sentry_value_decref(item->headers);
    sentry_value_decref(item->event);
    sentry_free(item->payload);
}

sentry_value_t
sentry_envelope_get_header(const sentry_envelope_t *envelope, const char *key)
{
    return sentry_envelope_get_header_n(
        envelope, key, sentry__guarded_strlen(key));
}

sentry_value_t
sentry_envelope_get_header_n(
    const sentry_envelope_t *envelope, const char *key, size_t key_len)
{
    if (!envelope || envelope->is_raw) {
        return sentry_value_new_null();
    }
    return sentry_value_get_by_key_n(
        envelope->contents.items.headers, key, key_len);
}

void
sentry__envelope_item_set_header(
    sentry_envelope_item_t *item, const char *key, sentry_value_t value)
{
    sentry_value_set_by_key(item->headers, key, value);
}

static int
envelope_item_get_ratelimiter_category(const sentry_envelope_item_t *item)
{
    const char *ty = sentry_value_as_string(
        sentry_value_get_by_key(item->headers, "type"));
    if (sentry__string_eq(ty, "session")) {
        return SENTRY_RL_CATEGORY_SESSION;
    } else if (sentry__string_eq(ty, "transaction")) {
        return SENTRY_RL_CATEGORY_TRANSACTION;
    }
    // NOTE: the `type` here can be `event` or `attachment`.
    // Ideally, attachments should have their own RL_CATEGORY.
    return SENTRY_RL_CATEGORY_ERROR;
}

static sentry_envelope_item_t *
envelope_add_from_owned_buffer(
    sentry_envelope_t *envelope, char *buf, size_t buf_len, const char *type)
{
    if (!buf) {
        return NULL;
    }
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        sentry_free(buf);
        return NULL;
    }

    item->payload = buf;
    item->payload_len = buf_len;
    sentry_value_t length = sentry_value_new_int32((int32_t)buf_len);
    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string(type));
    sentry__envelope_item_set_header(item, "length", length);

    return item;
}

void
sentry_envelope_free(sentry_envelope_t *envelope)
{
    if (!envelope) {
        return;
    }
    if (envelope->is_raw) {
        sentry_free(envelope->contents.raw.payload);
        sentry_free(envelope);
        return;
    }
    sentry_value_decref(envelope->contents.items.headers);

    // Free all items in the linked list
    sentry_envelope_item_t *item = envelope->contents.items.first_item;
    while (item) {
        sentry_envelope_item_t *next = item->next;
        envelope_item_cleanup(item);
        sentry_free(item);
        item = next;
    }

    sentry_free(envelope);
}

void
sentry__envelope_set_header(
    sentry_envelope_t *envelope, const char *key, sentry_value_t value)
{
    if (sentry__envelope_materialize(envelope) != 0) {
        return;
    }
    sentry_value_set_by_key(envelope->contents.items.headers, key, value);
}

sentry_envelope_t *
sentry__envelope_new(void)
{
    sentry_dsn_t *dsn = NULL;
    SENTRY_WITH_OPTIONS (options) {
        dsn = sentry__dsn_incref(options->dsn);
    }
    sentry_envelope_t *rv = sentry__envelope_new_with_dsn(dsn);
    sentry__dsn_decref(dsn);
    return rv;
}

sentry_envelope_t *
sentry__envelope_new_with_dsn(const sentry_dsn_t *dsn)
{
    sentry_envelope_t *rv = SENTRY_MAKE(sentry_envelope_t);
    if (!rv) {
        return NULL;
    }

    rv->is_raw = false;
    rv->contents.items.first_item = NULL;
    rv->contents.items.last_item = NULL;
    rv->contents.items.item_count = 0;
    rv->contents.items.headers = sentry_value_new_object();

    if (dsn && dsn->is_valid) {
        sentry__envelope_set_header(
            rv, "dsn", sentry_value_new_string(dsn->raw));
    }

    return rv;
}

sentry_envelope_t *
sentry__envelope_from_path(const sentry_path_t *path)
{
    size_t buf_len;
    char *buf = sentry__path_read_to_buffer(path, &buf_len);
    if (!buf) {
        SENTRY_WARNF("failed to read raw envelope from \"%s\"", path->path);
        return NULL;
    }

    sentry_envelope_t *envelope = SENTRY_MAKE(sentry_envelope_t);
    if (!envelope) {
        sentry_free(buf);
        return NULL;
    }

    envelope->is_raw = true;
    envelope->contents.raw.payload = buf;
    envelope->contents.raw.payload_len = buf_len;

    return envelope;
}

sentry_uuid_t
sentry__envelope_get_event_id(const sentry_envelope_t *envelope)
{
    if (!envelope) {
        return sentry_uuid_nil();
    }
    if (envelope->is_raw) {
        const char *payload = envelope->contents.raw.payload;
        size_t payload_len = envelope->contents.raw.payload_len;
        const char *newline = memchr(payload, '\n', payload_len);
        size_t header_len = newline ? (size_t)(newline - payload) : payload_len;
        sentry_value_t header = sentry__value_from_json(payload, header_len);
        sentry_uuid_t event_id = sentry_uuid_from_string(sentry_value_as_string(
            sentry_value_get_by_key(header, "event_id")));
        sentry_value_decref(header);
        return event_id;
    }
    return sentry_uuid_from_string(sentry_value_as_string(
        sentry_value_get_by_key(envelope->contents.items.headers, "event_id")));
}

void
sentry__envelope_set_event_id(
    sentry_envelope_t *envelope, const sentry_uuid_t *event_id)
{
    sentry_value_t value = sentry__value_new_uuid(event_id);
    sentry__envelope_set_header(envelope, "event_id", value);
}

sentry_value_t
sentry_envelope_get_event(const sentry_envelope_t *envelope)
{
    if (!envelope || envelope->is_raw) {
        return sentry_value_new_null();
    }

    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        if (!sentry_value_is_null(item->event)
            && !sentry__event_is_transaction(item->event)) {
            return item->event;
        }
    }
    return sentry_value_new_null();
}

sentry_value_t
sentry_envelope_get_transaction(const sentry_envelope_t *envelope)
{
    if (!envelope || envelope->is_raw) {
        return sentry_value_new_null();
    }

    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        if (!sentry_value_is_null(item->event)
            && sentry__event_is_transaction(item->event)) {
            return item->event;
        }
    }
    return sentry_value_new_null();
}

sentry_envelope_item_t *
sentry__envelope_add_event(sentry_envelope_t *envelope, sentry_value_t event)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry_uuid_t event_id;
    sentry__ensure_event_id(event, &event_id);

    sentry__jsonwriter_write_value(jw, event);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }
    item->event = event;

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("event"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    sentry__envelope_set_event_id(envelope, &event_id);

    double traces_sample_rate = 0.0;
    SENTRY_WITH_OPTIONS (options) {
        traces_sample_rate = options->traces_sample_rate;
    }
    sentry_value_t dsc = sentry_value_new_null();
    sentry_value_t sample_rand = sentry_value_new_null();
    SENTRY_WITH_SCOPE (scope) {
        dsc = sentry__value_clone(scope->dynamic_sampling_context);
        sample_rand = sentry_value_get_by_key(
            sentry_value_get_by_key(scope->propagation_context, "trace"),
            "sample_rand");
    }
    if (!sentry_value_is_null(dsc)) {
        sentry_value_t trace_id = sentry_value_get_by_key(
            sentry_value_get_by_key(
                sentry_value_get_by_key(event, "contexts"), "trace"),
            "trace_id");
        if (!sentry_value_is_null(trace_id)) {
            sentry_value_incref(trace_id);
            sentry_value_set_by_key(dsc, "trace_id", trace_id);
        } else {
            SENTRY_WARN("couldn't retrieve trace_id from scope to apply to the "
                        "dynamic sampling context");
        }
        if (!sentry_value_is_null(sample_rand)) {
            if (sentry_value_as_double(sample_rand) >= traces_sample_rate) {
                sentry_value_set_by_key(
                    dsc, "sampled", sentry_value_new_string("false"));
            } else {
                sentry_value_set_by_key(
                    dsc, "sampled", sentry_value_new_string("true"));
            }
        } else {
            // only for testing; in production, the SDK should always have a
            // non-null sample_rand. We don't set "sampled" to keep dsc empty
            SENTRY_WARN("couldn't retrieve sample_rand from scope to apply to "
                        "the dynamic sampling context");
        }
        // only add dsc if it has values
        if (sentry_value_is_true(dsc)) {
#ifdef SENTRY_UNITTEST
            // to make comparing the header feasible in unit tests
            sentry_value_set_by_key(dsc, "sample_rand",
                sentry_value_new_double(0.01006918276309107));
#endif
            sentry__envelope_set_header(envelope, "trace", dsc);
        } else {
            sentry_value_decref(dsc);
        }
    }

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_transaction(
    sentry_envelope_t *envelope, sentry_value_t transaction)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry_uuid_t event_id;
    sentry__ensure_event_id(transaction, &event_id);

    sentry__jsonwriter_write_value(jw, transaction);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }
    item->event = transaction;

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("transaction"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    sentry__envelope_set_event_id(envelope, &event_id);

    sentry_value_t dsc = sentry_value_new_null();

    SENTRY_WITH_SCOPE (scope) {
        dsc = sentry__value_clone(scope->dynamic_sampling_context);
    }

    if (!sentry_value_is_null(dsc)) {
        sentry_value_t trace_id = sentry_value_get_by_key(
            sentry_value_get_by_key(
                sentry_value_get_by_key(transaction, "contexts"), "trace"),
            "trace_id");
        if (!sentry_value_is_null(trace_id)) {
            sentry_value_incref(trace_id);
            sentry_value_set_by_key(dsc, "trace_id", trace_id);
            sentry_value_set_by_key(
                dsc, "sampled", sentry_value_new_string("true"));
        } else {
            SENTRY_WARN("couldn't retrieve trace_id in transaction's trace "
                        "context to apply to the dynamic sampling context");
        }
        sentry_value_t transaction_name
            = sentry_value_get_by_key(transaction, "transaction");
        if (!sentry_value_is_null(transaction_name)) {
            sentry_value_incref(transaction_name);
            sentry_value_set_by_key(dsc, "transaction", transaction_name);
        }
        // only add dsc if it has values
        if (sentry_value_is_true(dsc)) {
            sentry__envelope_set_header(envelope, "trace", dsc);
        } else {
            sentry_value_decref(dsc);
        }
    }

#ifdef SENTRY_UNITTEST
    sentry_value_t now = sentry_value_new_string("2021-12-16T05:53:59.343Z");
#else
    sentry_value_t now = sentry__value_new_string_owned(
        sentry__usec_time_to_iso8601(sentry__usec_time()));
#endif
    sentry__envelope_set_header(envelope, "sent_at", now);

    return item;
}

static sentry_envelope_item_t *
add_telemetry(sentry_envelope_t *envelope, sentry_value_t telemetry,
    const char *type, const char *content_type)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry__jsonwriter_write_value(jw, telemetry);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string(type));
    sentry__envelope_item_set_header(item, "item_count",
        sentry_value_new_int32((int32_t)sentry_value_get_length(
            sentry_value_get_by_key(telemetry, "items"))));
    sentry__envelope_item_set_header(
        item, "content_type", sentry_value_new_string(content_type));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_logs(sentry_envelope_t *envelope, sentry_value_t logs)
{
    return add_telemetry(
        envelope, logs, "log", "application/vnd.sentry.items.log+json");
}

sentry_envelope_item_t *
sentry__envelope_add_metrics(
    sentry_envelope_t *envelope, sentry_value_t metrics)
{
    return add_telemetry(envelope, metrics, "trace_metric",
        "application/vnd.sentry.items.trace-metric+json");
}

sentry_envelope_item_t *
sentry__envelope_add_user_report(
    sentry_envelope_t *envelope, sentry_value_t user_report)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }

    sentry_uuid_t event_id;
    sentry__ensure_event_id(user_report, &event_id);

    sentry__jsonwriter_write_value(jw, user_report);
    item->payload = sentry__jsonwriter_into_string(jw, &item->payload_len);
    if (!item->payload) {
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("user_report"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    sentry__envelope_item_set_header(item, "length", length);

    sentry__envelope_set_event_id(envelope, &event_id);

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_user_feedback(
    sentry_envelope_t *envelope, sentry_value_t user_feedback)
{
    sentry_value_t event = sentry_value_new_event();
    sentry_value_t contexts = sentry_value_get_by_key(event, "contexts");
    if (sentry_value_is_null(contexts)) {
        contexts = sentry_value_new_object();
    }
    sentry_value_set_by_key(contexts, "feedback", user_feedback);
    sentry_value_set_by_key(event, "contexts", contexts);

    sentry_envelope_item_t *item = sentry__envelope_add_event(envelope, event);
    if (!item) {
        sentry_value_decref(event);
        return NULL;
    }

    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("feedback"));

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_session(
    sentry_envelope_t *envelope, const sentry_session_t *session)
{
    if (!envelope || !session) {
        return NULL;
    }
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    if (!jw) {
        return NULL;
    }
    sentry__session_to_json(session, jw);
    size_t payload_len = 0;
    char *payload = sentry__jsonwriter_into_string(jw, &payload_len);

    // NOTE: function will check for `payload` internally and free it on error
    return envelope_add_from_owned_buffer(
        envelope, payload, payload_len, "session");
}

static const char *
str_from_attachment_type(sentry_attachment_type_t attachment_type)
{
    switch (attachment_type) {
    case ATTACHMENT:
        return "event.attachment";
    case MINIDUMP:
        return "event.minidump";
    case VIEW_HIERARCHY:
        return "event.view_hierarchy";
    default:
        UNREACHABLE("Unknown attachment type");
        return "event.attachment";
    }
}

static sentry_envelope_item_t *
envelope_add_attachment_ref(sentry_envelope_t *envelope,
    const sentry_path_t *path, size_t file_size, const char *content_type)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }
    sentry__envelope_item_set_header(
        item, "type", sentry_value_new_string("attachment"));
    sentry__envelope_item_set_header(item, "content_type",
        sentry_value_new_string("application/vnd.sentry.attachment-ref"));
    sentry__envelope_item_set_header(item, "attachment_length",
        sentry_value_new_uint64((uint64_t)file_size));

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(&sb);
    sentry__jsonwriter_write_object_start(jw);
    sentry__jsonwriter_write_key(jw, "path");
#ifdef SENTRY_PLATFORM_WINDOWS
    char *path_str = sentry__string_from_wstr(path->path_w);
    sentry__jsonwriter_write_str(jw, path_str);
    sentry_free(path_str);
#else
    sentry__jsonwriter_write_str(jw, path->path);
#endif
    if (content_type) {
        sentry__jsonwriter_write_key(jw, "content_type");
        sentry__jsonwriter_write_str(jw, content_type);
    }
    sentry__jsonwriter_write_object_end(jw);
    sentry__jsonwriter_free(jw);

    size_t payload_len = sentry__stringbuilder_len(&sb);
    char *payload = sentry__stringbuilder_into_string(&sb);
    item->payload = payload;
    item->payload_len = payload_len;
    sentry__envelope_item_set_header(
        item, "length", sentry_value_new_int32((int32_t)payload_len));

    return item;
}

void
sentry__envelope_item_set_attachment_ref(
    sentry_envelope_item_t *item, const sentry_path_t *path)
{
    size_t old_len = 0;
    const char *old_payload = sentry__envelope_item_get_payload(item, &old_len);
    sentry_value_t obj = (old_payload && old_len > 0)
        ? sentry__value_from_json(old_payload, old_len)
        : sentry_value_new_object();

#ifdef SENTRY_PLATFORM_WINDOWS
    char *path_str = sentry__string_from_wstr(path->path_w);
    sentry_value_set_by_key(obj, "path", sentry_value_new_string(path_str));
    sentry_free(path_str);
#else
    sentry_value_set_by_key(obj, "path", sentry_value_new_string(path->path));
#endif

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(NULL);
    sentry__jsonwriter_write_value(jw, obj);
    sentry_value_decref(obj);
    size_t json_len = 0;
    char *json = sentry__jsonwriter_into_string(jw, &json_len);
    sentry__envelope_item_set_payload(item, json, json_len);
}

bool
sentry__envelope_item_is_attachment_ref(const sentry_envelope_item_t *item)
{
    const char *ct = sentry_value_as_string(
        sentry__envelope_item_get_header(item, "content_type"));
    return ct && strcmp(ct, "application/vnd.sentry.attachment-ref") == 0;
}

sentry_path_t *
sentry__envelope_item_get_attachment_ref_path(
    const sentry_envelope_item_t *item)
{
    size_t payload_len = 0;
    const char *payload = sentry__envelope_item_get_payload(item, &payload_len);
    if (!payload || payload_len == 0) {
        return NULL;
    }
    sentry_value_t pj = sentry__value_from_json(payload, payload_len);
    const char *ps
        = sentry_value_as_string(sentry_value_get_by_key(pj, "path"));
    sentry_path_t *path
        = (ps && *ps != '\0') ? sentry__path_from_str(ps) : NULL;
    sentry_value_decref(pj);
    return path;
}

sentry_envelope_item_t *
sentry__envelope_add_attachment(
    sentry_envelope_t *envelope, const sentry_attachment_t *attachment)
{
    if (!envelope || !attachment) {
        return NULL;
    }

    size_t file_size = attachment->buf
        ? attachment->buf_len
        : sentry__path_get_size(attachment->path);

    sentry_envelope_item_t *item = NULL;
    if (file_size >= SENTRY_TUS_UPLOAD_THRESHOLD) {
        if (attachment->buf) {
            item = envelope_add_item(envelope);
            if (item) {
                sentry__envelope_item_set_header(
                    item, "type", sentry_value_new_string("attachment"));
                sentry__envelope_item_set_header(item, "content_type",
                    sentry_value_new_string(
                        "application/vnd.sentry.attachment-ref"));
                sentry__envelope_item_set_header(item, "attachment_length",
                    sentry_value_new_uint64((uint64_t)file_size));
                sentry__envelope_item_set_header(
                    item, "inline", sentry_value_new_bool(true));
                item->payload = sentry__string_clone_n(
                    attachment->buf, attachment->buf_len);
                item->payload_len = attachment->buf_len;
                sentry__envelope_item_set_header(item, "length",
                    sentry_value_new_int32((int32_t)item->payload_len));
            }
        } else {
            item = envelope_add_attachment_ref(envelope, attachment->path,
                file_size, attachment->content_type);
        }
    } else if (attachment->buf) {
        item = sentry__envelope_add_from_buffer(
            envelope, attachment->buf, attachment->buf_len, "attachment");
    } else {
        item = sentry__envelope_add_from_path(
            envelope, attachment->path, "attachment");
    }
    if (!item) {
        return NULL;
    }
    if (attachment->type != ATTACHMENT) { // don't need to set the default
        sentry__envelope_item_set_header(item, "attachment_type",
            sentry_value_new_string(
                str_from_attachment_type(attachment->type)));
    }
    if (attachment->content_type
        && sentry_value_is_null(
            sentry_value_get_by_key(item->headers, "content_type"))) {
        sentry__envelope_item_set_header(item, "content_type",
            sentry_value_new_string(attachment->content_type));
    }
    sentry__envelope_item_set_header(item, "filename",
        sentry_value_new_string(sentry__path_filename(
            attachment->filename ? attachment->filename : attachment->path)));

    return item;
}

void
sentry__envelope_add_attachments(
    sentry_envelope_t *envelope, const sentry_attachment_t *attachments)
{
    if (!envelope || !attachments) {
        return;
    }

    SENTRY_DEBUG("adding attachments to envelope");
    for (const sentry_attachment_t *attachment = attachments; attachment;
        attachment = attachment->next) {
        sentry__envelope_add_attachment(envelope, attachment);
    }
}

sentry_envelope_item_t *
sentry__envelope_add_from_buffer(sentry_envelope_t *envelope, const char *buf,
    size_t buf_len, const char *type)
{
    // NOTE: function will check for the clone of `buf` internally and free it
    // on error
    return envelope_add_from_owned_buffer(
        envelope, sentry__string_clone_n(buf, buf_len), buf_len, type);
}

sentry_envelope_item_t *
sentry__envelope_add_from_path(
    sentry_envelope_t *envelope, const sentry_path_t *path, const char *type)
{
    if (!envelope) {
        return NULL;
    }
    size_t buf_len;
    char *buf = sentry__path_read_to_buffer(path, &buf_len);
    if (!buf) {
        SENTRY_WARNF("failed to read envelope item from \"%s\"", path->path);
        return NULL;
    }
    // NOTE: function will free `buf` on error
    return envelope_add_from_owned_buffer(envelope, buf, buf_len, type);
}

static void
sentry__envelope_serialize_headers_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb)
{
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(sb);
    if (jw) {
        sentry__jsonwriter_write_value(jw, envelope->contents.items.headers);
        sentry__jsonwriter_free(jw);
    }
}

static void
sentry__envelope_serialize_item_into_stringbuilder(
    const sentry_envelope_item_t *item, sentry_stringbuilder_t *sb)
{
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_sb(sb);
    if (!jw) {
        return;
    }
    sentry__stringbuilder_append_char(sb, '\n');

    sentry__jsonwriter_write_value(jw, item->headers);
    sentry__jsonwriter_free(jw);

    sentry__stringbuilder_append_char(sb, '\n');

    sentry__stringbuilder_append_buf(sb, item->payload, item->payload_len);
}

void
sentry__envelope_serialize_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb)
{
    if (envelope->is_raw) {
        sentry__stringbuilder_append_buf(sb, envelope->contents.raw.payload,
            envelope->contents.raw.payload_len);
        return;
    }

    SENTRY_DEBUG("serializing envelope into buffer");
    sentry__envelope_serialize_headers_into_stringbuilder(envelope, sb);

    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        sentry__envelope_serialize_item_into_stringbuilder(item, sb);
    }
}

char *
sentry_envelope_serialize_ratelimited(const sentry_envelope_t *envelope,
    const sentry_rate_limiter_t *rl, size_t *size_out, bool *owned_out)
{
    if (envelope->is_raw) {
        *size_out = envelope->contents.raw.payload_len;
        *owned_out = false;
        return envelope->contents.raw.payload;
    }
    *owned_out = true;

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    sentry__envelope_serialize_headers_into_stringbuilder(envelope, &sb);

    size_t serialized_items = 0;
    for (const sentry_envelope_item_t *item
        = envelope->contents.items.first_item;
        item; item = item->next) {
        if (rl) {
            int category = envelope_item_get_ratelimiter_category(item);
            if (sentry__rate_limiter_is_disabled(rl, category)) {
                continue;
            }
        }
        sentry__envelope_serialize_item_into_stringbuilder(item, &sb);
        serialized_items += 1;
    }

    if (!serialized_items) {
        sentry__stringbuilder_cleanup(&sb);
        *size_out = 0;
        return NULL;
    }

    *size_out = sentry__stringbuilder_len(&sb);
    return sentry__stringbuilder_into_string(&sb);
}

char *
sentry_envelope_serialize(const sentry_envelope_t *envelope, size_t *size_out)
{
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    sentry__envelope_serialize_into_stringbuilder(envelope, &sb);

    if (size_out) {
        *size_out = sentry__stringbuilder_len(&sb);
    }
    return sentry__stringbuilder_into_string(&sb);
}

MUST_USE int
sentry_envelope_write_to_path(
    const sentry_envelope_t *envelope, const sentry_path_t *path)
{
    sentry_filewriter_t *fw = sentry__filewriter_new(path);
    if (!fw) {
        return 1;
    }

    if (envelope->is_raw) {
        size_t rv = sentry__filewriter_write(fw, envelope->contents.raw.payload,
            envelope->contents.raw.payload_len);
        sentry__filewriter_free(fw);
        return rv != 0;
    }

    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_fw(fw);
    if (jw) {
        sentry__jsonwriter_write_value(jw, envelope->contents.items.headers);
        sentry__jsonwriter_reset(jw);

        for (const sentry_envelope_item_t *item
            = envelope->contents.items.first_item;
            item; item = item->next) {
            const char newline = '\n';
            sentry__filewriter_write(fw, &newline, sizeof(char));

            sentry__jsonwriter_write_value(jw, item->headers);
            sentry__jsonwriter_reset(jw);

            sentry__filewriter_write(fw, &newline, sizeof(char));

            sentry__filewriter_write(fw, item->payload, item->payload_len);
        }
        sentry__jsonwriter_free(jw);
    }

    size_t rv = sentry__filewriter_byte_count(fw);
    sentry__filewriter_free(fw);

    return rv == 0;
}

int
sentry_envelope_write_to_file_n(
    const sentry_envelope_t *envelope, const char *path, size_t path_len)
{
    if (!envelope || !path) {
        return 1;
    }
    sentry_path_t *path_obj = sentry__path_from_str_n(path, path_len);

    int rv = sentry_envelope_write_to_path(envelope, path_obj);

    sentry__path_free(path_obj);

    return rv;
}

int
sentry_envelope_write_to_file(
    const sentry_envelope_t *envelope, const char *path)
{
    if (!envelope || !path) {
        return 1;
    }

    return sentry_envelope_write_to_file_n(envelope, path, strlen(path));
}

// https://develop.sentry.dev/sdk/data-model/envelopes/
static int
envelope_deserialize_into(
    sentry_envelope_t *envelope, const char *buf, size_t buf_len)
{
    const char *ptr = buf;
    const char *end = buf + buf_len;

    // headers
    const char *headers_end = memchr(ptr, '\n', (size_t)(end - ptr));
    if (!headers_end) {
        headers_end = end;
    }
    size_t headers_len = (size_t)(headers_end - ptr);
    sentry_value_decref(envelope->contents.items.headers);
    envelope->contents.items.headers
        = sentry__value_from_json(ptr, headers_len);
    if (sentry_value_get_type(envelope->contents.items.headers)
        != SENTRY_VALUE_TYPE_OBJECT) {
        return 1;
    }

    ptr = headers_end;
    if (ptr < end) {
        ptr++; // skip newline
    }

    // items
    while (ptr < end) {
        sentry_envelope_item_t *item = envelope_add_item(envelope);
        if (!item) {
            return 1;
        }

        // item headers
        const char *item_headers_end = memchr(ptr, '\n', (size_t)(end - ptr));
        if (!item_headers_end) {
            item_headers_end = end;
        }
        size_t item_headers_len = (size_t)(item_headers_end - ptr);
        sentry_value_decref(item->headers);
        item->headers = sentry__value_from_json(ptr, item_headers_len);
        if (sentry_value_get_type(item->headers) != SENTRY_VALUE_TYPE_OBJECT) {
            return 1;
        }
        ptr = item_headers_end + 1; // skip newline

        if (ptr > end) {
            return 1;
        }

        // item payload
        sentry_value_t length
            = sentry_value_get_by_key(item->headers, "length");
        if (sentry_value_is_null(length)) {
            // length omitted -> find newline or end of buffer
            const char *payload_end = memchr(ptr, '\n', (size_t)(end - ptr));
            if (!payload_end) {
                payload_end = end;
            }
            item->payload_len = (size_t)(payload_end - ptr);
        } else if (sentry_value_get_type(length) == SENTRY_VALUE_TYPE_UINT64) {
            uint64_t payload_len = sentry_value_as_uint64(length);
            if (payload_len >= SIZE_MAX) {
                return 1;
            }
            item->payload_len = (size_t)payload_len;
        } else {
            int64_t payload_len = sentry_value_as_int64(length);
            if (payload_len < 0 || (uint64_t)payload_len >= SIZE_MAX) {
                return 1;
            }
            item->payload_len = (size_t)payload_len;
        }
        if (item->payload_len > 0) {
            if (ptr + item->payload_len > end
                || item->payload_len >= SIZE_MAX) {
                return 1;
            }
            item->payload = sentry_malloc(item->payload_len + 1);
            if (!item->payload) {
                return 1;
            }
            memcpy(item->payload, ptr, item->payload_len);
            item->payload[item->payload_len] = '\0';

            // item event/transaction
            const char *type = sentry_value_as_string(
                sentry_value_get_by_key(item->headers, "type"));
            if (type
                && (sentry__string_eq(type, "event")
                    || sentry__string_eq(type, "transaction"))) {
                item->event
                    = sentry__value_from_json(item->payload, item->payload_len);
            }

            ptr += item->payload_len;
        }

        while (ptr < end && *ptr == '\n') {
            ptr++;
        }
    }

    return 0;
}

sentry_envelope_t *
sentry_envelope_deserialize(const char *buf, size_t buf_len)
{
    if (!buf || buf_len == 0) {
        return NULL;
    }

    sentry_envelope_t *envelope = sentry__envelope_new();
    if (!envelope) {
        return NULL;
    }

    if (envelope_deserialize_into(envelope, buf, buf_len) != 0) {
        sentry_envelope_free(envelope);
        return NULL;
    }

    return envelope;
}

static sentry_envelope_t *
parse_envelope_from_file(sentry_path_t *path)
{
    if (!path) {
        return NULL;
    }

    size_t buf_len = 0;
    char *buf = sentry__path_read_to_buffer(path, &buf_len);
    sentry_envelope_t *envelope = sentry_envelope_deserialize(buf, buf_len);
    sentry_free(buf);
    sentry__path_free(path);
    return envelope;
}

sentry_envelope_t *
sentry_envelope_read_from_file(const char *path)
{
    if (!path) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_str(path));
}

sentry_envelope_t *
sentry_envelope_read_from_file_n(const char *path, size_t path_len)
{
    if (!path || path_len == 0) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_str_n(path, path_len));
}

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_envelope_t *
sentry_envelope_read_from_filew(const wchar_t *path)
{
    if (!path) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_wstr(path));
}

sentry_envelope_t *
sentry_envelope_read_from_filew_n(const wchar_t *path, size_t path_len)
{
    if (!path || path_len == 0) {
        return NULL;
    }
    return parse_envelope_from_file(sentry__path_from_wstr_n(path, path_len));
}
#endif

size_t
sentry__envelope_get_item_count(const sentry_envelope_t *envelope)
{
    if (!envelope || envelope->is_raw) {
        return 0;
    }
    return envelope->contents.items.item_count;
}

static sentry_envelope_item_t *
envelope_get_item(sentry_envelope_t *envelope, size_t idx)
{
    if (sentry__envelope_materialize(envelope) != 0) {
        return NULL;
    }

    // Traverse linked list to find item at index
    size_t current_idx = 0;
    for (sentry_envelope_item_t *item = envelope->contents.items.first_item;
        item; item = item->next) {
        if (current_idx == idx) {
            return item;
        }
        current_idx++;
    }

    return NULL;
}

const sentry_envelope_item_t *
sentry__envelope_get_item(const sentry_envelope_t *envelope, size_t idx)
{
    return envelope_get_item((sentry_envelope_t *)envelope, idx);
}

sentry_envelope_item_t *
sentry__envelope_get_item_mut(sentry_envelope_t *envelope, size_t idx)
{
    return envelope_get_item(envelope, idx);
}

sentry_value_t
sentry__envelope_item_get_header(
    const sentry_envelope_item_t *item, const char *key)
{
    return sentry_value_get_by_key(item->headers, key);
}

const char *
sentry__envelope_item_get_payload(
    const sentry_envelope_item_t *item, size_t *payload_len_out)
{
    if (payload_len_out) {
        *payload_len_out = item->payload_len;
    }
    return item->payload;
}

void
sentry__envelope_item_set_payload(
    sentry_envelope_item_t *item, char *payload, size_t payload_len)
{
    sentry_free(item->payload);
    item->payload = payload;
    item->payload_len = payload_len;
    sentry__envelope_item_set_header(
        item, "length", sentry_value_new_int32((int32_t)payload_len));
}
