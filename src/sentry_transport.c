#include "sentry_transport.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include "sentry_ratelimiter.h"

#define ENVELOPE_MIME "application/x-sentry-envelope"
// The headers we use are: `x-sentry-auth`, `content-type`, `content-length`
#define MAX_HTTP_HEADERS 3

sentry_prepared_http_request_t *
sentry__prepare_http_request(
    sentry_envelope_t *envelope, const sentry_rate_limiter_t *rl)
{
    if (rl) {
        envelope = sentry__envelope_ratelimit_items(envelope, rl);
    }
    if (!envelope) {
        return NULL;
    }
    const sentry_options_t *options = sentry_get_options();
    if (!options) {
        sentry_envelope_free(envelope);
        return NULL;
    }

    size_t body_len = 0;
    char *body = sentry_envelope_serialize_consume(envelope, &body_len);

    sentry_prepared_http_request_t *req
        = SENTRY_MAKE(sentry_prepared_http_request_t);
    if (!req) {
        sentry_free(body);
        return NULL;
    }
    req->headers = sentry_malloc(
        sizeof(sentry_prepared_http_header_t) * MAX_HTTP_HEADERS);
    if (!req->headers) {
        sentry_free(req);
        return NULL;
    }
    req->headers_len = 0;

    req->method = "POST";
    req->url = sentry__dsn_get_envelope_url(&options->dsn);

    sentry_prepared_http_header_t *h;
    if (!options->dsn.empty) {
        h = &req->headers[req->headers_len++];
        h->key = "x-sentry-auth";
        h->value = sentry__dsn_get_auth_header(&options->dsn);
    }

    h = &req->headers[req->headers_len++];
    h->key = "content-type";
    h->value = sentry__string_clone(ENVELOPE_MIME);

    h = &req->headers[req->headers_len++];
    h->key = "content-length";
    h->value = sentry__int64_to_string((int64_t)body_len);

    req->body = body;
    req->body_len = body_len;

    return req;
}

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
    sentry_free(req->body);
    sentry_free(req);
}
