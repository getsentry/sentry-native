#ifndef SENTRY_ENVELOPE_H_INCLUDED
#define SENTRY_ENVELOPE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"
#include "sentry_string.h"

struct sentry_envelope_item_s;
typedef struct sentry_envelope_item_s sentry_envelope_item_t;

typedef struct sentry_prepared_http_header_s {
    const char *key;
    char *value;
} sentry_prepared_http_header_t;

typedef struct sentry_prepared_http_request_s {
    char *url;
    const char *method;
    sentry_prepared_http_header_t *headers;
    size_t headers_len;
    char *payload;
    size_t payload_len;
    bool payload_owned;
} sentry_prepared_http_request_t;

void sentry__prepared_http_request_free(sentry_prepared_http_request_t *req);

sentry_envelope_t *sentry__envelope_new(void);
sentry_uuid_t sentry__envelope_get_event_id(const sentry_envelope_t *envelope);
sentry_envelope_item_t *sentry__envelope_add_event(
    sentry_envelope_t *envelope, sentry_value_t event);
sentry_envelope_item_t *sentry__envelope_add_from_path(
    sentry_envelope_t *envelope, const sentry_path_t *path, const char *type);
sentry_envelope_item_t *sentry__envelope_add_from_buffer(
    sentry_envelope_t *envelope, const char *buf, size_t buf_len,
    const char *type);
void sentry__envelope_for_each_request(const sentry_envelope_t *envelope,
    bool (*callback)(sentry_prepared_http_request_t *,
        const sentry_envelope_t *, void *data),
    void *data);

void sentry__envelope_serialize_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb);

int sentry_envelope_write_to_path(
    const sentry_envelope_t *envelope, const sentry_path_t *path);

#endif