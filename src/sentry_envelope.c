#include "sentry_envelope.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_json.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_value.h"
#include <string.h>

#define MAX_ENVELOPE_ITEMS 10
#define MAX_HTTP_HEADERS 5

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
    if (req->payload_owned) {
        sentry_free(req->payload);
    }
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

const char *
envelope_item_get_content_type(const sentry_envelope_item_t *item)
{
    sentry_value_t content_type
        = sentry_value_get_by_key(item->headers, "content_type");
    if (sentry_value_is_null(content_type)) {
        switch (envelope_item_get_type(item)) {
        case ENVELOPE_ITEM_TYPE_MINIDUMP:
            return "application/x-minidump";
        case ENVELOPE_ITEM_TYPE_EVENT:
            return "application/json";
        default:
            return "application/octet-stream";
        }
    }
    return sentry_value_as_string(content_type);
}

const char *
envelope_item_get_name(const sentry_envelope_item_t *item)
{
    sentry_value_t name = sentry_value_get_by_key(item->headers, "name");
    if (sentry_value_is_null(name)) {
        switch (envelope_item_get_type(item)) {
        case ENVELOPE_ITEM_TYPE_MINIDUMP:
            return "uploaded_file_minidump";
        case ENVELOPE_ITEM_TYPE_EVENT:
            return "event";
        default:
            return "attachment";
        }
    }
    return sentry_value_as_string(name);
}

const char *
envelope_item_get_filename(const sentry_envelope_item_t *item)
{
    sentry_value_t filename
        = sentry_value_get_by_key(item->headers, "filename");
    if (sentry_value_is_null(filename)) {
        switch (envelope_item_get_type(item)) {
        case ENVELOPE_ITEM_TYPE_MINIDUMP:
            return "minidump.dmp";
        case ENVELOPE_ITEM_TYPE_EVENT:
            return "event.json";
        default:
            return "attachment.bin";
        }
    }
    return sentry_value_as_string(filename);
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

static void
sentry__envelope_set_header(
    sentry_envelope_t *envelope, const char *key, sentry_value_t value)
{
    sentry_value_set_by_key(envelope->headers, key, value);
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

    const sentry_options_t *options = sentry_get_options();
    if (options && !options->dsn.empty) {
        sentry__envelope_set_header(rv, "dsn",
            sentry_value_new_string(sentry_options_get_dsn(options)));
    }

    return rv;
}

sentry_uuid_t
sentry__envelope_get_event_id(const sentry_envelope_t *envelope)
{
    return sentry_uuid_from_string(sentry_value_as_string(
        sentry_value_get_by_key(envelope->headers, "event_id")));
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

    sentry_value_t event_id = sentry__ensure_event_id(event, NULL);

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

static sentry_envelope_item_t *
envelope_add_from_owned_buffer(
    sentry_envelope_t *envelope, char *buf, size_t buf_len, const char *type)
{
    sentry_envelope_item_t *item = envelope_add_item(envelope);
    if (!item || !buf) {
        return NULL;
    }

    item->payload = buf;
    item->payload_len = buf_len;
    sentry_value_t length = sentry_value_new_int32((int32_t)buf_len);
    envelope_item_set_header(item, "type", sentry_value_new_string(type));
    envelope_item_set_header(item, "length", length);

    return item;
}

sentry_envelope_item_t *
sentry__envelope_add_from_buffer(sentry_envelope_t *envelope, const char *buf,
    size_t buf_len, const char *type)
{
    return envelope_add_from_owned_buffer(
        envelope, sentry__string_clonen(buf, buf_len), buf_len, type);
}

sentry_envelope_item_t *
sentry__envelope_add_from_path(
    sentry_envelope_t *envelope, const sentry_path_t *path, const char *type)
{
    size_t buf_len;
    char *buf = sentry__path_read_to_buffer(path, &buf_len);
    if (!buf) {
        return NULL;
    }
    sentry_envelope_item_t *rv
        = envelope_add_from_owned_buffer(envelope, buf, buf_len, type);
    if (!rv) {
        sentry_free(buf);
        return NULL;
    }
    return rv;
}

static sentry_prepared_http_request_t *
prepare_http_request(const sentry_uuid_t *event_id,
    endpoint_type_t endpoint_type, const char *content_type, char *payload,
    size_t payload_len, bool payload_owned)
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

    rv->method = "POST";
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
    h->value = sentry__string_clone(content_type);

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
    default:
        rv->url = NULL;
    }

    rv->payload = payload;
    rv->payload_len = payload_len;
    rv->payload_owned = payload_owned;

    return rv;
}

static void
gen_boundary(char *boundary)
{
#if SENTRY_UNITTEST
    sentry_uuid_t boundary_id
        = sentry_uuid_from_string("0220b54a-d050-42ef-954a-ac481dc924db");
#else
    sentry_uuid_t boundary_id = sentry_uuid_new_v4();
#endif
    sentry_uuid_as_string(&boundary_id, boundary);
    strcat(boundary, "-boundary-");
}

void
sentry__envelope_for_each_request(const sentry_envelope_t *envelope,
    bool (*callback)(sentry_prepared_http_request_t *,
        const sentry_envelope_t *, void *data),
    void *data)
{
    sentry_prepared_http_request_t *req;
    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    const sentry_envelope_item_t *attachments[MAX_ENVELOPE_ITEMS];
    const sentry_envelope_item_t *minidump = NULL;
    size_t attachment_count = 0;

    for (size_t i = 0; i < envelope->item_count; i++) {
        const sentry_envelope_item_t *item = &envelope->items[i];
        switch (envelope_item_get_type(item)) {
        case ENVELOPE_ITEM_TYPE_EVENT: {
            req = prepare_http_request(&event_id, ENDPOINT_TYPE_STORE,
                "application/json", item->payload, item->payload_len, false);
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

    endpoint_type_t endpoint_type = ENDPOINT_TYPE_ATTACHMENT;
    if (minidump) {
        attachments[attachment_count++] = minidump;
        endpoint_type = ENDPOINT_TYPE_MINIDUMP;
    }

    if (attachment_count == 0) {
        return;
    }

    char boundary[50];
    gen_boundary(boundary);

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    for (size_t i = 0; i < attachment_count; i++) {
        const sentry_envelope_item_t *item = attachments[i];
        sentry__stringbuilder_append(&sb, "--");
        sentry__stringbuilder_append(&sb, boundary);
        sentry__stringbuilder_append(&sb, "\r\ncontent-type:");
        sentry__stringbuilder_append(&sb, envelope_item_get_content_type(item));
        sentry__stringbuilder_append(
            &sb, "\r\ncontent-disposition:form-data;name=\"");
        sentry__stringbuilder_append(&sb, envelope_item_get_name(item));
        sentry__stringbuilder_append(&sb, "\";filename=\"");
        sentry__stringbuilder_append(&sb, envelope_item_get_filename(item));
        sentry__stringbuilder_append(&sb, "\"\r\n\r\n");
        sentry__stringbuilder_append_buf(&sb, item->payload, item->payload_len);
        sentry__stringbuilder_append(&sb, "\r\n");
    }
    sentry__stringbuilder_append(&sb, "--");
    sentry__stringbuilder_append(&sb, boundary);
    sentry__stringbuilder_append(&sb, "--");

    size_t body_len = sentry__stringbuilder_len(&sb);
    char *body = sentry__stringbuilder_into_string(&sb);

    sentry__stringbuilder_init(&sb);
    sentry__stringbuilder_append(&sb, "multipart/form-data;boundary=\"");
    sentry__stringbuilder_append(&sb, boundary);
    sentry__stringbuilder_append(&sb, "\"");
    char *content_type = sentry__stringbuilder_into_string(&sb);

    req = prepare_http_request(
        &event_id, endpoint_type, content_type, body, body_len, true);
    sentry_free(content_type);

    callback(req, envelope, data);
}

void
sentry__envelope_serialize_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb)
{
    char *buf = sentry_value_to_json(envelope->headers);
    sentry__stringbuilder_append(sb, buf);
    sentry_free(buf);

    for (size_t i = 0; i < envelope->item_count; i++) {
        const sentry_envelope_item_t *item = &envelope->items[i];

        sentry__stringbuilder_append_char(sb, '\n');
        buf = sentry_value_to_json(item->headers);
        sentry__stringbuilder_append(sb, buf);
        sentry__stringbuilder_append_char(sb, '\n');
        sentry_free(buf);

        sentry__stringbuilder_append_buf(sb, item->payload, item->payload_len);
    }
}

int
sentry_envelope_write_to_path(
    const sentry_envelope_t *envelope, const sentry_path_t *path)
{
    // TODO: This currently builds the whole buffer in-memory.
    // It would be nice to actually stream this to a file.
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    sentry__envelope_serialize_into_stringbuilder(envelope, &sb);

    size_t buf_len = sentry__stringbuilder_len(&sb);
    char *buf = sentry__stringbuilder_into_string(&sb);

    int rv = sentry__path_write_buffer(path, buf, buf_len);

    sentry_free(buf);

    return rv;
}

int
sentry_envelope_write_to_file(
    const sentry_envelope_t *envelope, const char *path)
{
    sentry_path_t *path_obj = sentry__path_from_str(path);

    int rv = sentry_envelope_write_to_path(envelope, path_obj);

    sentry__path_free(path_obj);

    return rv;
}
