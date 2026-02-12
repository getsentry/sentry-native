#include "sentry_http_transport.h"
#include "sentry_alloc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_ratelimiter.h"
#include "sentry_retry.h"
#include "sentry_string.h"
#include "sentry_transport.h"
#include "sentry_utils.h"

#ifdef SENTRY_TRANSPORT_COMPRESSION
#    include "zlib.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ENVELOPE_MIME "application/x-sentry-envelope"
#ifdef SENTRY_TRANSPORT_COMPRESSION
#    define MAX_HTTP_HEADERS 4
#else
#    define MAX_HTTP_HEADERS 3
#endif

typedef struct {
    sentry_dsn_t *dsn;
    char *user_agent;
    sentry_rate_limiter_t *ratelimiter;
    void *client;
    void (*free_client)(void *);
    int (*start_client)(void *, const sentry_options_t *);
    sentry_http_send_func_t send_func;
    void (*shutdown_client)(void *client);
    sentry_bgworker_t *bgworker;
    sentry_retry_t *retry;
} http_transport_state_t;

#ifdef SENTRY_TRANSPORT_COMPRESSION
static bool
gzipped_with_compression(const char *body, const size_t body_len,
    char **compressed_body, size_t *compressed_body_len)
{
    if (!body || body_len == 0) {
        return false;
    }

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = (unsigned char *)body;
    stream.avail_in = (unsigned int)body_len;

    int err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
        MAX_WBITS + 16, 9, Z_DEFAULT_STRATEGY);
    if (err != Z_OK) {
        SENTRY_WARNF("deflateInit2 failed: %d", err);
        return false;
    }

    size_t len = compressBound((unsigned long)body_len);
    char *buffer = sentry_malloc(len);
    if (!buffer) {
        deflateEnd(&stream);
        return false;
    }

    while (err == Z_OK) {
        stream.next_out = (unsigned char *)(buffer + stream.total_out);
        stream.avail_out = (unsigned int)(len - stream.total_out);
        err = deflate(&stream, Z_FINISH);
    }

    if (err != Z_STREAM_END) {
        SENTRY_WARNF("deflate failed: %d", err);
        sentry_free(buffer);
        buffer = NULL;
        deflateEnd(&stream);
        return false;
    }

    *compressed_body_len = stream.total_out;
    *compressed_body = buffer;

    deflateEnd(&stream);
    return true;
}
#endif

sentry_prepared_http_request_t *
sentry__prepare_http_request(sentry_envelope_t *envelope,
    const sentry_dsn_t *dsn, const sentry_rate_limiter_t *rl,
    const char *user_agent)
{
    if (!dsn || !dsn->is_valid) {
        return NULL;
    }

    size_t body_len = 0;
    bool body_owned = true;
    char *body = sentry_envelope_serialize_ratelimited(
        envelope, rl, &body_len, &body_owned);
    if (!body) {
        return NULL;
    }

#ifdef SENTRY_TRANSPORT_COMPRESSION
    bool compressed = false;
    char *compressed_body = NULL;
    size_t compressed_body_len = 0;
    compressed = gzipped_with_compression(
        body, body_len, &compressed_body, &compressed_body_len);
    if (compressed) {
        if (body_owned) {
            sentry_free(body);
            body_owned = false;
        }
        body = compressed_body;
        body_len = compressed_body_len;
        body_owned = true;
    }
#endif

    sentry_prepared_http_request_t *req
        = SENTRY_MAKE(sentry_prepared_http_request_t);
    if (!req) {
        if (body_owned) {
            sentry_free(body);
        }
        return NULL;
    }
    req->headers = sentry_malloc(
        sizeof(sentry_prepared_http_header_t) * MAX_HTTP_HEADERS);
    if (!req->headers) {
        sentry_free(req);
        if (body_owned) {
            sentry_free(body);
        }
        return NULL;
    }
    req->headers_len = 0;

    req->method = "POST";
    req->url = sentry__dsn_get_envelope_url(dsn);

    sentry_prepared_http_header_t *h;
    h = &req->headers[req->headers_len++];
    h->key = "x-sentry-auth";
    h->value = sentry__dsn_get_auth_header(dsn, user_agent);

    h = &req->headers[req->headers_len++];
    h->key = "content-type";
    h->value = sentry__string_clone(ENVELOPE_MIME);

#ifdef SENTRY_TRANSPORT_COMPRESSION
    if (compressed) {
        h = &req->headers[req->headers_len++];
        h->key = "content-encoding";
        h->value = sentry__string_clone("gzip");
    }
#endif

    h = &req->headers[req->headers_len++];
    h->key = "content-length";
    h->value = sentry__int64_to_string((int64_t)body_len);

    req->body = body;
    req->body_len = body_len;
    req->body_owned = body_owned;

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
    if (req->body_owned) {
        sentry_free(req->body);
    }
    sentry_free(req);
}

static void retry_process_task(void *_check_backoff, void *_state);

static int
http_send_request(
    http_transport_state_t *state, sentry_prepared_http_request_t *req)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    if (!state->send_func(state->client, req, &resp)) {
        sentry_free(resp.retry_after);
        sentry_free(resp.x_sentry_rate_limits);
        return -1;
    }

    if (resp.x_sentry_rate_limits) {
        sentry__rate_limiter_update_from_header(
            state->ratelimiter, resp.x_sentry_rate_limits);
    } else if (resp.retry_after) {
        sentry__rate_limiter_update_from_http_retry_after(
            state->ratelimiter, resp.retry_after);
    } else if (resp.status_code == 429) {
        sentry__rate_limiter_update_from_429(state->ratelimiter);
    }

    sentry_free(resp.retry_after);
    sentry_free(resp.x_sentry_rate_limits);
    return resp.status_code;
}

static void
retry_process_task(void *_startup, void *_state)
{
    int startup = (int)(intptr_t)_startup;
    http_transport_state_t *state = _state;

    if (!state->retry) {
        return;
    }

    size_t count = 0;
    sentry_path_t **paths = sentry__retry_scan(state->retry, startup, &count);

    for (size_t i = 0; i < count; i++) {
        sentry_envelope_t *envelope = sentry__envelope_from_path(paths[i]);
        if (!envelope) {
            sentry__path_remove(paths[i]);
            continue;
        }

        sentry_prepared_http_request_t *req = sentry__prepare_http_request(
            envelope, state->dsn, state->ratelimiter, state->user_agent);
        int status_code;
        if (!req) {
            status_code = 0;
        } else {
            status_code = http_send_request(state, req);
            sentry__prepared_http_request_free(req);
        }
        sentry_envelope_free(envelope);

        sentry__retry_handle_result(state->retry, paths[i], status_code);
    }

    sentry__retry_free_paths(paths, count);

    if (sentry__retry_has_files(state->retry)) {
        sentry__bgworker_submit_delayed(state->bgworker, retry_process_task,
            NULL, (void *)(intptr_t)0, SENTRY_RETRY_BACKOFF_BASE_S * 1000);
    }
}

static void
http_transport_state_free(void *_state)
{
    http_transport_state_t *state = _state;
    if (state->free_client) {
        state->free_client(state->client);
    }
    sentry__dsn_decref(state->dsn);
    sentry_free(state->user_agent);
    sentry__rate_limiter_free(state->ratelimiter);
    sentry__retry_free(state->retry);
    sentry_free(state);
}

static void
http_send_task(void *_envelope, void *_state)
{
    sentry_envelope_t *envelope = _envelope;
    http_transport_state_t *state = _state;

    sentry_prepared_http_request_t *req = sentry__prepare_http_request(
        envelope, state->dsn, state->ratelimiter, state->user_agent);
    if (!req) {
        return;
    }

    int status_code = http_send_request(state, req);
    sentry__prepared_http_request_free(req);

    if (status_code < 0 && state->retry) {
        sentry__retry_write_envelope(state->retry, envelope);
        sentry__bgworker_submit_delayed(state->bgworker, retry_process_task,
            NULL, (void *)(intptr_t)0, SENTRY_RETRY_BACKOFF_BASE_S * 1000);
    }
}

static int
http_transport_start(const sentry_options_t *options, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    http_transport_state_t *state = sentry__bgworker_get_state(bgworker);

    sentry__bgworker_setname(bgworker, options->transport_thread_name);

    state->dsn = sentry__dsn_incref(options->dsn);
    state->user_agent = sentry__string_clone(options->user_agent);

    if (state->start_client) {
        int rv = state->start_client(state->client, options);
        if (rv != 0) {
            return rv;
        }
    }

    int rv = sentry__bgworker_start(bgworker);
    if (rv != 0) {
        return rv;
    }

    if (options->http_retries > 0) {
        sentry_path_t *retry_dir
            = sentry__path_join_str(options->database_path, "retry");
        if (retry_dir) {
            sentry__path_create_dir_all(retry_dir);
            sentry_path_t *cache_dir = NULL;
            if (options->cache_keep) {
                cache_dir
                    = sentry__path_join_str(options->database_path, "cache");
                if (cache_dir) {
                    sentry__path_create_dir_all(cache_dir);
                }
            }
            state->retry = sentry__retry_new(
                retry_dir, cache_dir, options->http_retries);
            sentry__path_free(cache_dir);
            sentry__path_free(retry_dir);
        }
        if (state->retry) {
            sentry__retry_set_startup_time(state->retry, (uint64_t)time(NULL));
            sentry__bgworker_submit_delayed(bgworker, retry_process_task, NULL,
                (void *)(intptr_t)1, SENTRY_RETRY_STARTUP_DELAY_MS);
        }
    }

    return 0;
}

static int
http_transport_flush(uint64_t timeout, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    return sentry__bgworker_flush(bgworker, timeout);
}

static int
http_transport_shutdown(uint64_t timeout, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    http_transport_state_t *state = sentry__bgworker_get_state(bgworker);

    int rv = sentry__bgworker_shutdown(bgworker, timeout);
    if (rv != 0 && state->shutdown_client) {
        state->shutdown_client(state->client);
    }
    return rv;
}

static void
http_transport_send_envelope(sentry_envelope_t *envelope, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    sentry__bgworker_submit(bgworker, http_send_task,
        (void (*)(void *))sentry_envelope_free, envelope);
}

static bool
http_dump_task_cb(void *envelope, void *run)
{
    sentry__run_write_envelope(
        (sentry_run_t *)run, (sentry_envelope_t *)envelope);
    return true;
}

static size_t
http_dump_queue(sentry_run_t *run, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    return sentry__bgworker_foreach_matching(
        bgworker, http_send_task, http_dump_task_cb, run);
}

static http_transport_state_t *
http_transport_get_state(sentry_transport_t *transport)
{
    sentry_bgworker_t *bgworker = sentry__transport_get_state(transport);
    return sentry__bgworker_get_state(bgworker);
}

sentry_transport_t *
sentry__http_transport_new(void *client, sentry_http_send_func_t send_func)
{
    http_transport_state_t *state = SENTRY_MAKE(http_transport_state_t);
    if (!state) {
        return NULL;
    }
    memset(state, 0, sizeof(http_transport_state_t));
    state->ratelimiter = sentry__rate_limiter_new();
    state->client = client;
    state->send_func = send_func;

    sentry_bgworker_t *bgworker
        = sentry__bgworker_new(state, http_transport_state_free);
    if (!bgworker) {
        http_transport_state_free(state);
        return NULL;
    }
    state->bgworker = bgworker;

    sentry_transport_t *transport
        = sentry_transport_new(http_transport_send_envelope);
    if (!transport) {
        sentry__bgworker_decref(bgworker);
        return NULL;
    }

    sentry_transport_set_state(transport, bgworker);
    sentry_transport_set_free_func(
        transport, (void (*)(void *))sentry__bgworker_decref);
    sentry_transport_set_startup_func(transport, http_transport_start);
    sentry_transport_set_flush_func(transport, http_transport_flush);
    sentry_transport_set_shutdown_func(transport, http_transport_shutdown);
    sentry__transport_set_dump_func(transport, http_dump_queue);

    return transport;
}

void
sentry__http_transport_set_free_client(
    sentry_transport_t *transport, void (*free_client)(void *))
{
    http_transport_get_state(transport)->free_client = free_client;
}

void
sentry__http_transport_set_start_client(sentry_transport_t *transport,
    int (*start_client)(void *, const sentry_options_t *))
{
    http_transport_get_state(transport)->start_client = start_client;
}

void
sentry__http_transport_set_shutdown_client(
    sentry_transport_t *transport, void (*shutdown_client)(void *))
{
    http_transport_get_state(transport)->shutdown_client = shutdown_client;
}

#ifdef SENTRY_UNITTEST
void *
sentry__http_transport_get_bgworker(sentry_transport_t *transport)
{
    return sentry__transport_get_state(transport);
}
#endif
