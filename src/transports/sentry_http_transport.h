#ifndef SENTRY_HTTP_TRANSPORT_H_INCLUDED
#define SENTRY_HTTP_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"
#include "sentry_ratelimiter.h"
#include "sentry_sync.h"
#include "sentry_transport.h"

typedef struct sentry_prepared_http_header_s {
    const char *key;
    char *value;
} sentry_prepared_http_header_t;

struct sentry_http_request_s {
    const char *method;
    char *url;
    sentry_prepared_http_header_t *headers;
    size_t headers_len;
    char *body;
    size_t body_len;
    bool body_owned;
    sentry_path_t *body_path;
};

typedef sentry_http_request_t sentry_prepared_http_request_t;

sentry_prepared_http_request_t *sentry__prepare_http_request(
    sentry_envelope_t *envelope, const sentry_dsn_t *dsn,
    const sentry_rate_limiter_t *rl, const char *user_agent);
sentry_prepared_http_request_t *sentry__prepare_tus_create_request(
    size_t file_size, const char *attachment_type, const sentry_dsn_t *dsn,
    const char *user_agent);
sentry_prepared_http_request_t *sentry__prepare_tus_upload_request(
    const char *location, const sentry_path_t *path, size_t file_size,
    const sentry_dsn_t *dsn, const char *user_agent);

void sentry__prepared_http_request_free(sentry_prepared_http_request_t *req);

#ifdef SENTRY_UNITTEST
void *sentry__http_transport_get_bgworker(sentry_transport_t *transport);
#endif

#endif
