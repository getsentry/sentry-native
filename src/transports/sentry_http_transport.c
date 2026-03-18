#include "sentry_http_transport.h"
#include "sentry_alloc.h"
#include "sentry_attachment.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_ratelimiter.h"
#include "sentry_retry.h"
#include "sentry_string.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_value.h"

#ifdef SENTRY_TRANSPORT_COMPRESSION
#    include "zlib.h"
#endif

#include <string.h>

#define ENVELOPE_MIME "application/x-sentry-envelope"
#define TUS_MIME "application/offset+octet-stream"
#define TUS_MAX_HTTP_HEADERS 4
#ifdef SENTRY_TRANSPORT_COMPRESSION
#    define MAX_HTTP_HEADERS 4
#else
#    define MAX_HTTP_HEADERS 3
#endif

typedef struct {
    sentry_dsn_t *dsn;
    char *user_agent;
    sentry_run_t *run;
    sentry_rate_limiter_t *ratelimiter;
    void *client;
    void (*free_client)(void *);
    int (*start_client)(void *, const sentry_options_t *);
    sentry_http_send_func_t send_func;
    void (*shutdown_client)(void *client);
    sentry_retry_t *retry;
    bool cache_keep;
    bool has_tus;
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
    req->body_path = NULL;

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
    sentry__path_free(req->body_path);
    sentry_free(req);
}

static sentry_prepared_http_request_t *
prepare_tus_request_common(
    size_t upload_size, const sentry_dsn_t *dsn, const char *user_agent)
{
    if (!dsn || !dsn->is_valid) {
        return NULL;
    }

    sentry_prepared_http_request_t *req
        = SENTRY_MAKE(sentry_prepared_http_request_t);
    if (!req) {
        return NULL;
    }
    memset(req, 0, sizeof(*req));

    req->headers = sentry_malloc(
        sizeof(sentry_prepared_http_header_t) * TUS_MAX_HTTP_HEADERS);
    if (!req->headers) {
        sentry_free(req);
        return NULL;
    }
    req->headers_len = 0;

    req->method = "POST";
    req->url = sentry__dsn_get_upload_url(dsn);

    sentry_prepared_http_header_t *h;
    h = &req->headers[req->headers_len++];
    h->key = "x-sentry-auth";
    h->value = sentry__dsn_get_auth_header(dsn, user_agent);

    h = &req->headers[req->headers_len++];
    h->key = "content-type";
    h->value = sentry__string_clone(TUS_MIME);

    h = &req->headers[req->headers_len++];
    h->key = "tus-resumable";
    h->value = sentry__string_clone("1.0.0");

    h = &req->headers[req->headers_len++];
    h->key = "upload-length";
    h->value = sentry__uint64_to_string((uint64_t)upload_size);

    return req;
}

static sentry_prepared_http_request_t *
prepare_tus_request(const sentry_path_t *path, size_t file_size,
    const sentry_dsn_t *dsn, const char *user_agent)
{
    if (!path) {
        return NULL;
    }
    sentry_prepared_http_request_t *req
        = prepare_tus_request_common(file_size, dsn, user_agent);
    if (req) {
        req->body_path = sentry__path_clone(path);
        req->body_len = file_size;
    }
    return req;
}

static void
http_response_cleanup(sentry_http_response_t *resp)
{
    sentry_free(resp->retry_after);
    sentry_free(resp->x_sentry_rate_limits);
    sentry_free(resp->location);
}

static int
http_send_request(
    http_transport_state_t *state, sentry_prepared_http_request_t *req)
{
    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    if (!state->send_func(state->client, req, &resp)) {
        http_response_cleanup(&resp);
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

    http_response_cleanup(&resp);
    return resp.status_code;
}

static bool
tus_upload_ref(http_transport_state_t *state, const sentry_path_t *att_dir,
    sentry_value_t entry, sentry_envelope_t *tus_envelope)
{
    const char *filename
        = sentry_value_as_string(sentry_value_get_by_key(entry, "filename"));
    if (!filename || *filename == '\0') {
        return false;
    }

    sentry_path_t *att_file = sentry__path_join_str(att_dir, filename);
    size_t file_size = att_file ? sentry__path_get_size(att_file) : 0;
    if (!att_file || file_size == 0) {
        sentry__path_free(att_file);
        return false;
    }

    sentry_prepared_http_request_t *req = prepare_tus_request(
        att_file, file_size, state->dsn, state->user_agent);
    if (!req) {
        sentry__path_free(att_file);
        return false;
    }

    sentry_http_response_t resp;
    memset(&resp, 0, sizeof(resp));
    bool ok = state->send_func(state->client, req, &resp);
    sentry__prepared_http_request_free(req);

    if (!ok) {
        sentry__path_free(att_file);
        http_response_cleanup(&resp);
        return false;
    }

    if (resp.status_code == 404) {
        state->has_tus = false;
        SENTRY_WARN("TUS upload returned 404, disabling TUS");
        sentry__path_free(att_file);
        http_response_cleanup(&resp);
        return false;
    }

    if (resp.status_code != 201 || !resp.location) {
        sentry__path_free(att_file);
        http_response_cleanup(&resp);
        return false;
    }

    const char *ref_ct = sentry_value_as_string(
        sentry_value_get_by_key(entry, "content_type"));
    const char *att_type = sentry_value_as_string(
        sentry_value_get_by_key(entry, "attachment_type"));
    sentry_value_t att_length
        = sentry_value_get_by_key(entry, "attachment_length");
    sentry_value_incref(att_length);
    sentry__envelope_add_attachment_ref(tus_envelope, resp.location, filename,
        *ref_ct ? ref_ct : NULL, *att_type ? att_type : NULL, att_length);

    sentry__path_remove(att_file);
    sentry__path_free(att_file);
    http_response_cleanup(&resp);
    return true;
}

static bool
inline_cached_attachment(const sentry_path_t *att_dir, sentry_value_t entry,
    sentry_envelope_t *envelope)
{
    const char *filename
        = sentry_value_as_string(sentry_value_get_by_key(entry, "filename"));
    if (!filename || *filename == '\0') {
        return false;
    }

    const char *att_type = sentry_value_as_string(
        sentry_value_get_by_key(entry, "attachment_type"));
    const char *ref_ct = sentry_value_as_string(
        sentry_value_get_by_key(entry, "content_type"));

    sentry_path_t *att_file = sentry__path_join_str(att_dir, filename);
    if (!att_file) {
        return false;
    }

    // Try structured envelope first, fall back to raw append
    sentry_envelope_item_t *item
        = sentry__envelope_add_from_path(envelope, att_file, "attachment");
    if (item) {
        if (*att_type) {
            sentry__envelope_item_set_header(
                item, "attachment_type", sentry_value_new_string(att_type));
        }
        if (*ref_ct) {
            sentry__envelope_item_set_header(
                item, "content_type", sentry_value_new_string(ref_ct));
        }
        sentry__envelope_item_set_header(
            item, "filename", sentry_value_new_string(filename));
    } else if (!sentry__envelope_append_raw_attachment(envelope, att_file,
                   filename, *att_type ? att_type : NULL,
                   *ref_ct ? ref_ct : NULL)) {
        sentry__path_free(att_file);
        return false;
    }

    sentry__path_free(att_file);
    return true;
}

static void
resolve_and_send_external_attachments(http_transport_state_t *state,
    const char *uuid, sentry_envelope_t *envelope)
{
    if (!uuid || !state->run) {
        return;
    }

    sentry_path_t *att_dir
        = sentry__path_join_str(state->run->cache_path, uuid);
    if (!att_dir || !sentry__path_is_dir(att_dir)) {
        sentry__path_free(att_dir);
        return;
    }

    sentry_path_t *refs_path
        = sentry__path_join_str(att_dir, "__sentry-attachments.json");
    if (!refs_path) {
        sentry__path_free(att_dir);
        return;
    }
    if (!sentry__path_is_file(refs_path)) {
        sentry__path_free(refs_path);
        sentry__path_free(att_dir);
        return;
    }

    size_t buf_len = 0;
    char *buf = sentry__path_read_to_buffer(refs_path, &buf_len);
    if (!buf) {
        sentry__path_free(refs_path);
        sentry__path_free(att_dir);
        return;
    }
    sentry_value_t refs = sentry__value_from_json(buf, buf_len);
    sentry_free(buf);
    if (sentry_value_get_type(refs) != SENTRY_VALUE_TYPE_LIST) {
        sentry_value_decref(refs);
        sentry__path_free(refs_path);
        sentry__path_free(att_dir);
        return;
    }

    sentry_uuid_t event_id = sentry_uuid_from_string(uuid);
    sentry_envelope_t *tus_envelope = NULL;

    size_t count = sentry_value_get_length(refs);
    for (size_t i = 0; i < count; i++) {
        sentry_value_t entry = sentry_value_get_by_index(refs, i);

        const char *filename = sentry_value_as_string(
            sentry_value_get_by_key(entry, "filename"));
        if (!filename || *filename == '\0') {
            continue;
        }
        sentry_path_t *att_file = sentry__path_join_str(att_dir, filename);
        size_t file_size = att_file ? sentry__path_get_size(att_file) : 0;
        sentry__path_free(att_file);

        if (file_size >= SENTRY_LARGE_ATTACHMENT_SIZE && state->has_tus) {
            if (!tus_envelope) {
                tus_envelope = sentry__envelope_new_with_dsn(state->dsn);
                if (tus_envelope) {
                    sentry__envelope_set_event_id(tus_envelope, &event_id);
                }
            }
            if (tus_envelope) {
                tus_upload_ref(state, att_dir, entry, tus_envelope);
            }
            if (!state->has_tus) {
                break;
            }
        } else {
            inline_cached_attachment(att_dir, entry, envelope);
        }
    }

    sentry_value_decref(refs);

    if (tus_envelope) {
        sentry_prepared_http_request_t *req = sentry__prepare_http_request(
            tus_envelope, state->dsn, state->ratelimiter, state->user_agent);
        if (req) {
            http_send_request(state, req);
            sentry__prepared_http_request_free(req);
        }
        sentry_envelope_free(tus_envelope);
    }

    sentry__path_free(refs_path);
    sentry__path_free(att_dir);
}

static void
cleanup_external_attachments(const sentry_run_t *run, const char *uuid)
{
    if (!uuid || !run) {
        return;
    }
    sentry_path_t *att_dir = sentry__path_join_str(run->cache_path, uuid);
    if (att_dir) {
        sentry__path_remove_all(att_dir);
        sentry__path_free(att_dir);
    }
}

static int
http_send_envelope(http_transport_state_t *state, sentry_envelope_t *envelope,
    const char *uuid)
{
    char uuid_buf[37];
    if (!uuid) {
        sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
        if (!sentry_uuid_is_nil(&event_id)) {
            sentry_uuid_as_string(&event_id, uuid_buf);
            uuid = uuid_buf;
        }
    }

    resolve_and_send_external_attachments(state, uuid, envelope);

    sentry_prepared_http_request_t *req = sentry__prepare_http_request(
        envelope, state->dsn, state->ratelimiter, state->user_agent);
    if (!req) {
        return 0;
    }
    int status_code = http_send_request(state, req);
    sentry__prepared_http_request_free(req);

    if (status_code >= 0) {
        cleanup_external_attachments(state->run, uuid);
    }

    return status_code;
}

static int
retry_send_cb(sentry_envelope_t *envelope, const char *uuid, void *_state)
{
    http_transport_state_t *state = _state;
    return http_send_envelope(state, envelope, uuid);
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
    sentry__run_free(state->run);
    sentry__rate_limiter_free(state->ratelimiter);
    sentry__retry_free(state->retry);
    sentry_free(state);
}

static void
http_send_task(void *_envelope, void *_state)
{
    sentry_envelope_t *envelope = _envelope;
    http_transport_state_t *state = _state;

    int status_code = http_send_envelope(state, envelope, NULL);
    if (status_code < 0 && state->retry) {
        sentry__retry_enqueue(state->retry, envelope);
    } else if (status_code < 0 && state->cache_keep) {
        sentry__run_write_cache(state->run, envelope, -1);
    }
}

static void
http_transport_shutdown_timeout(void *_state)
{
    http_transport_state_t *state = _state;
    if (state->shutdown_client) {
        state->shutdown_client(state->client);
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
    state->cache_keep = options->cache_keep;
    state->run = sentry__run_incref(options->run);

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

    if (options->http_retry) {
        state->retry = sentry__retry_new(options);
        if (state->retry) {
            sentry__retry_start(state->retry, bgworker, retry_send_cb, state);
        } else {
            // cannot retry, clear retry_func so envelopes get cached instead of
            // dropped
            sentry__transport_set_retry_func(options->transport, NULL);
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

    sentry__retry_shutdown(state->retry);

    int rv = sentry__bgworker_shutdown_cb(
        bgworker, timeout, http_transport_shutdown_timeout, state);
    if (rv != 0) {
        sentry__retry_dump_queue(state->retry, http_send_task);
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

static void
http_transport_retry(void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    http_transport_state_t *state = sentry__bgworker_get_state(bgworker);
    if (state->retry) {
        sentry__retry_trigger(state->retry);
    }
}

static void
http_cleanup_cache_task(void *task_data, void *_state)
{
    (void)_state;
    sentry_options_t *options = task_data;
    sentry__cleanup_cache(options);
}

static void
http_transport_submit_cleanup(
    const sentry_options_t *options, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    sentry__bgworker_submit(bgworker, http_cleanup_cache_task,
        (void (*)(void *))sentry_options_free,
        sentry__options_incref((sentry_options_t *)options));
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
    state->has_tus = true;
    state->client = client;
    state->send_func = send_func;

    sentry_bgworker_t *bgworker
        = sentry__bgworker_new(state, http_transport_state_free);
    if (!bgworker) {
        http_transport_state_free(state);
        return NULL;
    }

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
    sentry__transport_set_retry_func(transport, http_transport_retry);
    sentry__transport_set_cleanup_func(
        transport, http_transport_submit_cleanup);

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

sentry_prepared_http_request_t *
sentry__prepare_tus_request(const sentry_path_t *path, size_t file_size,
    const sentry_dsn_t *dsn, const char *user_agent)
{
    return prepare_tus_request(path, file_size, dsn, user_agent);
}
#endif
