#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_transport.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdlib.h>
#include <string.h>

struct transport_state {
    bool initialized;
    CURL *curl_handle;
    sentry_rate_limiter_t *rl;
    sentry_bgworker_t *bgworker;
};

struct task_state {
    struct transport_state *transport_state;
    sentry_envelope_t *envelope;
};

struct header_info {
    char *x_sentry_rate_limits;
    char *retry_after;
};

static struct transport_state *
new_transport_state(void)
{
    static bool curl_initialized = false;

    struct transport_state *state = SENTRY_MAKE(struct transport_state);
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
start_transport(sentry_transport_t *transport)
{
    struct transport_state *state = transport->data;
    sentry__bgworker_start(state->bgworker);
}

static void
shutdown_transport(sentry_transport_t *transport)
{
    struct transport_state *state = transport->data;
    sentry__bgworker_shutdown(state->bgworker, 5000);
}

static void
free_transport(sentry_transport_t *transport)
{
    struct transport_state *state = transport->data;
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

static bool
for_each_request_callback(sentry_prepared_http_request_t *req,
    const sentry_envelope_t *UNUSED(envelope), void *data)
{
    struct task_state *ts = data;
    const sentry_options_t *opts = sentry_get_options();

    if (!opts || opts->dsn.empty || sentry__should_skip_upload()) {
        SENTRY_DEBUG("skipping event upload");
        return false;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "expect:");
    for (size_t i = 0; i < req->headers_len; i++) {
        char buf[255];
        snprintf(buf, sizeof(buf), "%s:%s", req->headers[i].key,
            req->headers[i].value);
        headers = curl_slist_append(headers, buf);
    }

    CURL *curl = ts->transport_state->curl_handle;
    curl_easy_reset(curl);
    if (opts->debug) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stderr);
        // CURLOPT_WRITEFUNCTION will `fwrite` by default
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swallow_data);
    }
    curl_easy_setopt(curl, CURLOPT_URL, req->url);
    curl_easy_setopt(curl, CURLOPT_POST, (long)1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->payload);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req->payload_len);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, SENTRY_SDK_USER_AGENT);

    struct header_info info;
    info.retry_after = NULL;
    info.x_sentry_rate_limits = NULL;
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&info);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

    if (opts->http_proxy && *opts->http_proxy) {
        curl_easy_setopt(curl, CURLOPT_PROXY, opts->http_proxy);
    }
    if (opts->ca_certs && *opts->ca_certs) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, opts->ca_certs);
    }

    CURLcode rv = curl_easy_perform(curl);

    if (rv == CURLE_OK) {
        if (info.x_sentry_rate_limits) {
            sentry__rate_limiter_update_from_header(
                ts->transport_state->rl, info.x_sentry_rate_limits);
        } else if (info.retry_after) {
            sentry__rate_limiter_update_from_http_retry_after(
                ts->transport_state->rl, info.retry_after);
        }
    }

    curl_slist_free_all(headers);
    sentry_free(info.retry_after);
    sentry_free(info.x_sentry_rate_limits);
    sentry__prepared_http_request_free(req);

    return true;
}

static void
task_exec_func(void *data)
{
    struct task_state *ts = data;
    sentry__envelope_for_each_request(
        ts->envelope, for_each_request_callback, ts->transport_state->rl, data);
}

static void
task_cleanup_func(void *data)
{
    struct task_state *ts = data;
    sentry_envelope_free(ts->envelope);
    sentry_free(ts);
}

static void
send_envelope(struct sentry_transport_s *transport, sentry_envelope_t *envelope)
{
    struct transport_state *state = transport->data;
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

sentry_transport_t *
sentry__transport_new_default(void)
{
    SENTRY_DEBUG("initializing curl transport");
    sentry_transport_t *transport = SENTRY_MAKE(sentry_transport_t);
    if (!transport) {
        return NULL;
    }

    struct transport_state *state = new_transport_state();
    if (!state) {
        sentry_free(transport);
        return NULL;
    }

    transport->data = state;
    transport->free_func = free_transport;
    transport->send_envelope_func = send_envelope;
    transport->startup_func = start_transport;
    transport->shutdown_func = shutdown_transport;

    return transport;
}

static bool
sentry__curl_dump(void *task_data, void *UNUSED(data))
{
    struct task_state *ts = task_data;
    const sentry_options_t *opts = sentry_get_options();

    sentry__run_write_envelope(opts->run, ts->envelope);

    return true;
}

void
sentry__transport_dump_queue(sentry_transport_t *transport)
{
    // make sure to only dump when it is actually *this* transport which is in
    // use
    if (transport->send_envelope_func != send_envelope) {
        return;
    }

    sentry_bgworker_t *bgworker
        = ((struct transport_state *)transport->data)->bgworker;

    size_t dumped = sentry__bgworker_foreach_matching(
        bgworker, task_exec_func, sentry__curl_dump, NULL);
    SENTRY_TRACEF("dumped %zu in-flight envelopes to disk", dumped);
}
