#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_ratelimiter.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_transport.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdlib.h>
#include <string.h>

typedef struct curl_transport_state_s {
    bool initialized;
    bool debug;
    CURL *curl_handle;
    const char *http_proxy;
    const char *ca_certs;
    sentry_rate_limiter_t *rl;
    sentry_bgworker_t *bgworker;
} curl_transport_state_t;

struct task_state {
    curl_transport_state_t *transport_state;
    sentry_envelope_t *envelope;
};

struct header_info {
    char *x_sentry_rate_limits;
    char *retry_after;
};

static curl_transport_state_t *
new_transport_state(void)
{
    static bool curl_initialized = false;

    curl_transport_state_t *state = SENTRY_MAKE(curl_transport_state_t);
    if (!state) {
        return NULL;
    }

    state->bgworker = sentry__bgworker_new();
    if (!state->bgworker) {
        sentry_free(state);
        return NULL;
    }

    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    state->initialized = false;
    state->curl_handle = curl_easy_init();
    state->rl = sentry__rate_limiter_new();

    return state;
}

static void
start_transport(const sentry_options_t *options, void *_state)
{
    curl_transport_state_t *state = _state;
    state->debug = options->debug;
    state->http_proxy = options->http_proxy;
    state->ca_certs = options->ca_certs;
    sentry__bgworker_start(state->bgworker);
}

static bool
shutdown_transport(uint64_t timeout, void *_state)
{
    curl_transport_state_t *state = _state;
    return !sentry__bgworker_shutdown(state->bgworker, timeout);
}

static void
free_transport(void *_state)
{
    curl_transport_state_t *state = _state;
    curl_easy_cleanup(state->curl_handle);
    sentry__bgworker_free(state->bgworker);
    sentry__rate_limiter_free(state->rl);
    sentry_free(state);
}

static size_t
swallow_data(
    char *UNUSED(ptr), size_t size, size_t nmemb, void *UNUSED(userdata))
{
    return size * nmemb;
}

static size_t
header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t bytes = size * nitems;
    struct header_info *info = userdata;
    char *header = sentry__string_clonen(buffer, bytes);
    if (!header) {
        return bytes;
    }

    char *sep = strchr(header, ':');
    if (sep) {
        *sep = '\0';
        sentry__string_ascii_lower(header);
        if (sentry__string_eq(header, "retry-after")) {
            info->retry_after = sentry__string_clone(sep + 1);
        } else if (sentry__string_eq(header, "x-sentry-rate-limits")) {
            info->x_sentry_rate_limits = sentry__string_clone(sep + 1);
        }
    }

    sentry_free(header);
    return bytes;
}

static void
task_exec_func(void *data)
{
    struct task_state *ts = data;
    curl_transport_state_t *state = ts->transport_state;

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(ts->envelope, state->rl);
    if (!req) {
        return;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "expect:");
    for (size_t i = 0; i < req->headers_len; i++) {
        char buf[255];
        snprintf(buf, sizeof(buf), "%s:%s", req->headers[i].key,
            req->headers[i].value);
        headers = curl_slist_append(headers, buf);
    }

    CURL *curl = state->curl_handle;
    curl_easy_reset(curl);
    if (state->debug) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stderr);
        // CURLOPT_WRITEFUNCTION will `fwrite` by default
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swallow_data);
    }
    curl_easy_setopt(curl, CURLOPT_URL, req->url);
    curl_easy_setopt(curl, CURLOPT_POST, (long)1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req->body_len);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, SENTRY_SDK_USER_AGENT);

    struct header_info info;
    info.retry_after = NULL;
    info.x_sentry_rate_limits = NULL;
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&info);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

    if (state->http_proxy) {
        curl_easy_setopt(curl, CURLOPT_PROXY, state->http_proxy);
    }
    if (state->ca_certs) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, state->ca_certs);
    }

    CURLcode rv = curl_easy_perform(curl);

    if (rv == CURLE_OK) {
        if (info.x_sentry_rate_limits) {
            sentry__rate_limiter_update_from_header(
                state->rl, info.x_sentry_rate_limits);
        } else if (info.retry_after) {
            sentry__rate_limiter_update_from_http_retry_after(
                state->rl, info.retry_after);
        }
    }

    curl_slist_free_all(headers);
    sentry_free(info.retry_after);
    sentry_free(info.x_sentry_rate_limits);
    sentry__prepared_http_request_free(req);
}

static void
task_cleanup_func(void *data)
{
    struct task_state *ts = data;
    sentry_envelope_free(ts->envelope);
    sentry_free(ts);
}

static void
send_envelope(sentry_envelope_t *envelope, void *_state)
{
    curl_transport_state_t *state = _state;
    struct task_state *ts = SENTRY_MAKE(struct task_state);
    if (!ts) {
        sentry_envelope_free(envelope);
        return;
    }
    ts->transport_state = state;
    ts->envelope = envelope;
    sentry__bgworker_submit(
        state->bgworker, task_exec_func, task_cleanup_func, ts);
}

static bool
sentry__curl_dump(void *task_data, void *run)
{
    struct task_state *ts = task_data;
    sentry__run_write_envelope((sentry_run_t *)run, ts->envelope);
    return true;
}

size_t
sentry__curl_dump_queue(void *state)
{
    sentry_bgworker_t *bgworker = ((curl_transport_state_t *)state)->bgworker;

    return sentry__bgworker_foreach_matching(
        bgworker, task_exec_func, sentry__curl_dump, sentry_get_options()->run);
}

sentry_transport_t *
sentry__transport_new_default(void)
{
    SENTRY_DEBUG("initializing curl transport");
    curl_transport_state_t *state = new_transport_state();
    if (!state) {
        return NULL;
    }
    sentry_transport_t *transport = sentry_transport_new(send_envelope);
    if (!transport) {
        free_transport(state);
        return NULL;
    }
    sentry_transport_set_state(transport, state);
    sentry_transport_set_free_func(transport, free_transport);
    sentry_transport_set_startup_func(transport, start_transport);
    sentry_transport_set_shutdown_func(transport, shutdown_transport);
    sentry__transport_set_dump_func(transport, sentry__curl_dump_queue);

    return transport;
}
