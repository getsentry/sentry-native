#include "sentry_envelope.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_string.h"
#include "sentry_value.h"
#include <sentry.h>
#include <string.h>

const size_t MAX_ENVELOPE_ITEMS = 10;
const size_t MAX_HTTP_HEADERS = 5;

typedef enum {
    ENDPOINT_TYPE_STORE,
    ENDPOINT_TYPE_MINIDUMP,
    ENDPOINT_TYPE_ATTACHMENT,
} endpoint_type_t;

typedef enum {
    ENVELOPE_ITEM_TYPE_EVENT,
    ENVELOPE_ITEM_TYPE_MINIDUMP,
    ENVELOPE_ITEM_TYPE_ATTACHMENT,
    ENVELOPE_ITEM_TYPE_UNKNOWN,
} envelope_item_type_t;

struct sentry_envelope_item_s {
    sentry_value_t headers;
    sentry_value_t event;
    char *payload;
    size_t payload_len;
};

struct sentry_envelope_s {
    sentry_value_t headers;
    sentry_envelope_item_t items[MAX_ENVELOPE_ITEMS];
    size_t item_count;
};

void
sentry__prepared_http_request_free(sentry_prepared_http_request_t *req)
{
    if (!req) {
        return;
    }
    sentry_free(req->url);
    for (size_t i = 0; i < req->headers_len; i++) {
        sentry_free(req->headers[i].value);
    }
    sentry_free(req->headers);
    sentry_free(req->payload);
    sentry_free(req);
}

static sentry_envelope_item_t *
envelope_add_item(sentry_envelope_t *envelope)
{
    if (envelope->item_count >= MAX_ENVELOPE_ITEMS) {
        return NULL;
    }

    sentry_envelope_item_t *rv = &envelope->items[envelope->item_count++];
    rv->headers = sentry_value_new_object();
    rv->event = sentry_value_new_null();
    rv->payload = NULL;
    rv->payload_len = 0;
    return rv;
}

static void
envelope_item_cleanup(sentry_envelope_item_t *item)
{
    sentry_value_decref(item->headers);
    sentry_value_decref(item->event);
    sentry_free(item->payload);
}

static envelope_item_type_t
envelope_item_get_type(const sentry_envelope_item_t *item)
{
    const char *ty = sentry_value_as_string(
        sentry_value_get_by_key(item->headers, "type"));
    if (strcmp(ty, "event") == 0) {
        return ENVELOPE_ITEM_TYPE_EVENT;
    } else if (strcmp(ty, "minidump") == 0) {
        return ENVELOPE_ITEM_TYPE_MINIDUMP;
    } else if (strcmp(ty, "attachment") == 0) {
        return ENVELOPE_ITEM_TYPE_ATTACHMENT;
    } else {
        return ENVELOPE_ITEM_TYPE_UNKNOWN;
    }
}

static void
envelope_item_set_header(
    sentry_envelope_item_t *item, const char *key, sentry_value_t value)
{
    sentry_value_set_by_key(item->headers, key, value);
}

void
sentry_envelope_free(sentry_envelope_t *envelope)
{
    if (!envelope) {
        return;
    }
    sentry_value_decref(envelope->headers);
    for (size_t i = 0; i < envelope->item_count; i++) {
        envelope_item_cleanup(&envelope->items[i]);
    }
    sentry_free(envelope);
}

sentry_envelope_t *
sentry__envelope_new(void)
{
    sentry_envelope_t *rv = SENTRY_MAKE(sentry_envelope_t);
    if (!rv) {
        return NULL;
    }

    rv->item_count = 0;
    rv->headers = sentry_value_new_object();

    /* TODO: add dsn as default header */

    return rv;
}

sentry_uuid_t
sentry__envelope_get_event_id(const sentry_envelope_t *envelope)
{
    return sentry_uuid_from_string(sentry_value_as_string(
        sentry_value_get_by_key(envelope->headers, "event_id")));
}

static void
sentry__envelope_set_header(
    sentry_envelope_t *envelope, const char *key, sentry_value_t value)
{
    sentry_value_set_by_key(envelope->headers, key, value);
}

sentry_value_t
sentry_envelope_get_event(const sentry_envelope_t *envelope)
{
    for (size_t i = 0; i < envelope->item_count; i++) {
        if (!sentry_value_is_null(envelope->items[i].event)) {
            return envelope->items[i].event;
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

    sentry_value_t event_id = sentry_value_get_by_key(event, "event_id");
    const char *uuid_str = sentry_value_as_string(event_id);
    sentry_uuid_t uuid = sentry_uuid_from_string(uuid_str);
    if (sentry_uuid_is_nil(&uuid)) {
        uuid = sentry_uuid_new_v4();
        event_id = sentry__value_new_uuid(&uuid);
        sentry_value_set_by_key(event, "event_id", event_id);
    }

    item->event = event;
    item->payload = sentry_value_to_json(event);
    item->payload_len = strlen(item->payload);
    envelope_item_set_header(item, "type", sentry_value_new_string("event"));
    sentry_value_t length = sentry_value_new_int32((int32_t)item->payload_len);
    envelope_item_set_header(item, "length", length);

    sentry_value_incref(event_id);
    sentry__envelope_set_header(envelope, "event_id", event_id);

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_from_disk(
    sentry_envelope_t *envelope, const sentry_path_t *path, const char *type)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item) {
        return NULL;
    }

    size_t size_out;
    char *buf = sentry__path_read_to_buffer(path, &size_out);
    if (!buf) {
        return NULL;
    }

    item->payload = buf;
    item->payload_len = size_out;
    sentry_value_t length = sentry_value_new_int32((int32_t)size_out);
    envelope_item_set_header(item, "type", sentry_value_new_string(type));
    envelope_item_set_header(item, "length", length);

    return item;
}

static sentry_prepared_http_request_t *
prepare_http_request(const sentry_uuid_t *event_id,
    endpoint_type_t endpoint_type, const char *content_type,
    const char *payload, size_t payload_len)
{
    const sentry_options_t *options = sentry_get_options();
    if (!options) {
        return NULL;
    }

    sentry_prepared_http_request_t *rv
        = SENTRY_MAKE(sentry_prepared_http_request_t);
    if (!rv) {
        return NULL;
    }

    rv->headers = sentry_malloc(
        sizeof(sentry_prepared_http_header_t) * MAX_HTTP_HEADERS);
    if (!rv->headers) {
        sentry_free(rv);
        return NULL;
    }

    sentry_prepared_http_header_t *h;
    rv->headers_len = 0;
    if (!options->dsn.empty) {
        h = &rv->headers[rv->headers_len++];
        h->key = "x-sentry-auth";
        h->value = sentry__dsn_get_auth_header(&options->dsn);
    }

    h = &rv->headers[rv->headers_len++];
    h->key = "content-type";
    h->value = sentry__string_dup(content_type);

    h = &rv->headers[rv->headers_len++];
    h->key = "content-length";
    h->value = sentry__int64_to_string((int64_t)payload_len);

    switch (endpoint_type) {
    case ENDPOINT_TYPE_STORE:
        rv->url = sentry__dsn_get_store_url(&options->dsn);
        break;
    case ENDPOINT_TYPE_MINIDUMP:
        rv->url = sentry__dsn_get_minidump_url(&options->dsn);
        break;
    case ENDPOINT_TYPE_ATTACHMENT: {
        rv->url = sentry__dsn_get_attachment_url(&options->dsn, event_id);
        break;
    }
    }

    return rv;
}

void
sentry__envelope_for_each_request(const sentry_envelope_t *envelope,
    bool (*callback)(sentry_prepared_http_request_t *,
        const sentry_envelope_t *, void *data),
    void *data)
{
    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    const sentry_envelope_item_t *attachments[MAX_ENVELOPE_ITEMS];
    const sentry_envelope_item_t *minidump = NULL;
    size_t attachment_count = 0;

    for (size_t i = 0; i < envelope->item_count; i++) {
        const sentry_envelope_item_t *item = &envelope->items[i];
        switch (envelope_item_get_type(item)) {
        case ENVELOPE_ITEM_TYPE_EVENT: {
            sentry_prepared_http_request_t *req;
            req = prepare_http_request(&event_id, ENDPOINT_TYPE_STORE,
                "application/json", item->payload, item->payload_len);
            if (!req || !callback(req, envelope, data)) {
                return;
            }
            break;
        }
        case ENVELOPE_ITEM_TYPE_ATTACHMENT: {
            attachments[attachment_count++] = item;
            break;
        }
        case ENVELOPE_ITEM_TYPE_MINIDUMP: {
            minidump = item;
            break;
        }
        default:
            continue;
        }
    }
}