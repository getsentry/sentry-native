#ifndef SENTRY_ENVELOPE_H_INCLUDED
#define SENTRY_ENVELOPE_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_core.h"

#include "sentry_path.h"
#include "sentry_session.h"
#include "sentry_string.h"

#define SENTRY_MAX_ENVELOPE_ITEMS 10

struct sentry_rate_limiter_s;
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
sentry_envelope_t *sentry__envelope_from_path(const sentry_path_t *path);
sentry_uuid_t sentry__envelope_get_event_id(const sentry_envelope_t *envelope);
sentry_envelope_item_t *sentry__envelope_add_event(
    sentry_envelope_t *envelope, sentry_value_t event);
sentry_envelope_item_t *sentry__envelope_add_session(
    sentry_envelope_t *envelope, const sentry_session_t *session);
sentry_envelope_item_t *sentry__envelope_add_from_path(
    sentry_envelope_t *envelope, const sentry_path_t *path, const char *type);
sentry_envelope_item_t *sentry__envelope_add_from_buffer(
    sentry_envelope_t *envelope, const char *buf, size_t buf_len,
    const char *type);
void sentry__envelope_item_set_header(
    sentry_envelope_item_t *item, const char *key, sentry_value_t value);
void sentry__envelope_for_each_request(const sentry_envelope_t *envelope,
    bool (*callback)(sentry_prepared_http_request_t *,
        const sentry_envelope_t *, void *data),
    const struct sentry_rate_limiter_s *rl, void *data);

void sentry__envelope_serialize_headers_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb);
void sentry__envelope_serialize_item_into_stringbuilder(
    const sentry_envelope_item_t *item, sentry_stringbuilder_t *sb);
void sentry__envelope_serialize_into_stringbuilder(
    const sentry_envelope_t *envelope, sentry_stringbuilder_t *sb);

MUST_USE int sentry_envelope_write_to_path(
    const sentry_envelope_t *envelope, const sentry_path_t *path);

// these for now are only needed for tests
#if SENTRY_UNITTEST
size_t sentry__envelope_get_item_count(const sentry_envelope_t *envelope);
const sentry_envelope_item_t *sentry__envelope_get_item(
    const sentry_envelope_t *envelope, size_t idx);
sentry_value_t sentry__envelope_item_get_header(
    const sentry_envelope_item_t *item, const char *key);
const char *sentry__envelope_item_get_payload(
    const sentry_envelope_item_t *item, size_t *payload_len_out);
#endif

#endif
