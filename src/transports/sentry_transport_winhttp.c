#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_ratelimiter.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_transport.h"

#include <stdlib.h>
#include <string.h>
#include <winhttp.h>

typedef struct {
    sentry_bgworker_t *bgworker;
    wchar_t *user_agent;
    wchar_t *proxy;
    sentry_rate_limiter_t *rl;
    HINTERNET session;
    HINTERNET connect;

    bool initialized;
    bool debug;
} winhttp_transport_state_t;

struct task_state {
    winhttp_transport_state_t *transport_state;
    sentry_envelope_t *envelope;
};

static winhttp_transport_state_t *
new_transport_state(void)
{
    winhttp_transport_state_t *state = SENTRY_MAKE(winhttp_transport_state_t);
    if (!state) {
        return NULL;
    }

    state->bgworker = sentry__bgworker_new();
    if (!state->bgworker) {
        sentry_free(state);
        return NULL;
    }

    state->connect = NULL;
    state->session = NULL;
    state->user_agent = NULL;
    state->proxy = NULL;
    state->rl = sentry__rate_limiter_new();

    return state;
}

static void
winhttp_transport_start(const sentry_options_t *opts, void *_state)
{
    winhttp_transport_state_t *state = _state;

    state->user_agent = sentry__string_to_wstr(SENTRY_SDK_USER_AGENT);
    state->debug = opts->debug;

    if (opts->http_proxy && strstr(opts->http_proxy, "http://") == 0) {
        const char *ptr = opts->http_proxy + 7;
        const char *slash = strchr(ptr, '/');
        if (slash) {
            char *copy = sentry__string_clonen(ptr, slash - ptr);
            state->proxy = sentry__string_to_wstr(copy);
            sentry_free(copy);
        } else {
            state->proxy = sentry__string_to_wstr(ptr);
        }
    }

    if (state->proxy) {
        state->session
            = WinHttpOpen(state->user_agent, WINHTTP_ACCESS_TYPE_NAMED_PROXY,
                state->proxy, WINHTTP_NO_PROXY_BYPASS, 0);
    } else {
#if _WIN32_WINNT >= 0x0603
        state->session = WinHttpOpen(state->user_agent,
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
#endif
        // On windows 8.0 or lower, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY does
        // not work on error we fallback to WINHTTP_ACCESS_TYPE_DEFAULT_PROXY
        if (!state->session) {
            state->session = WinHttpOpen(state->user_agent,
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS, 0);
        }
    }

    sentry__bgworker_start(state->bgworker);
}

static bool
winhttp_transport_shutdown(uint64_t timeout, void *_state)
{
    winhttp_transport_state_t *state = _state;
    return !sentry__bgworker_shutdown(state->bgworker, timeout);
}

static void
winhttp_transport_free(void *_state)
{
    winhttp_transport_state_t *state = _state;
    sentry__bgworker_free(state->bgworker);
    sentry__rate_limiter_free(state->rl);
    WinHttpCloseHandle(state->connect);
    WinHttpCloseHandle(state->session);
    sentry_free(state->user_agent);
    sentry_free(state->proxy);
    sentry_free(state);
}

static void
task_exec_func(void *data)
{
    struct task_state *ts = data;
    winhttp_transport_state_t *state = ts->transport_state;

    sentry_prepared_http_request_t *req
        = sentry__prepare_http_request(ts->envelope, state->rl);
    if (!req) {
        return;
    }

    wchar_t *url = sentry__string_to_wstr(req->url);

    URL_COMPONENTS url_components;
    wchar_t hostname[128];
    wchar_t url_path[4096];
    memset(&url_components, 0, sizeof(URL_COMPONENTS));
    url_components.dwStructSize = sizeof(URL_COMPONENTS);
    url_components.lpszHostName = hostname;
    url_components.dwHostNameLength = 128;
    url_components.lpszUrlPath = url_path;
    url_components.dwUrlPathLength = 1024;

    WinHttpCrackUrl(url, 0, 0, &url_components);
    if (!state->connect) {
        state->connect = WinHttpConnect(state->session,
            url_components.lpszHostName, url_components.nPort, 0);
    }

    bool is_secure = strstr(req->url, "https") == req->url;
    HINTERNET request = WinHttpOpenRequest(state->connect, L"POST",
        url_components.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, is_secure ? WINHTTP_FLAG_SECURE : 0);

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    for (size_t i = 0; i < req->headers_len; i++) {
        sentry__stringbuilder_append(&sb, req->headers[i].key);
        sentry__stringbuilder_append_char(&sb, ':');
        sentry__stringbuilder_append(&sb, req->headers[i].value);
        sentry__stringbuilder_append(&sb, "\r\n");
    }

    char *buf = sentry__stringbuilder_into_string(&sb);
    wchar_t *headers = sentry__string_to_wstr(buf);
    sentry_free(buf);

    SENTRY_TRACEF(
        "sending request using winhttp to \"%s\":\n%S", req->url, headers);

    if (WinHttpSendRequest(request, headers, -1, (LPVOID)req->body,
            (DWORD)req->body_len, (DWORD)req->body_len, 0)) {
        WinHttpReceiveResponse(request, NULL);

        if (state->debug) {
            // this is basically the example from:
            // https://docs.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpqueryheaders#examples
            DWORD dwSize = 0;
            LPVOID lpOutBuffer = NULL;
            WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize,
                WINHTTP_NO_HEADER_INDEX);

            // Allocate memory for the buffer.
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                lpOutBuffer = sentry_malloc(dwSize);

                // Now, use WinHttpQueryHeaders to retrieve the header.
                if (lpOutBuffer
                    && WinHttpQueryHeaders(request,
                        WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX, lpOutBuffer, &dwSize,
                        WINHTTP_NO_HEADER_INDEX)) {
                    SENTRY_TRACEF(
                        "received response:\n%S", (wchar_t *)lpOutBuffer);
                }
                sentry_free(lpOutBuffer);
            }
        }

        // lets just assume we won’t have headers > 2k
        wchar_t buf[2048];
        DWORD buf_size = sizeof(buf);
        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM,
                L"x-sentry-rate-limits", buf, &buf_size,
                WINHTTP_NO_HEADER_INDEX)) {
            char *h = sentry__string_from_wstr(buf);
            if (h) {
                sentry__rate_limiter_update_from_header(
                    ts->transport_state->rl, h);
                sentry_free(h);
            }
        } else if (WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM,
                       L"retry-after", buf, &buf_size,
                       WINHTTP_NO_HEADER_INDEX)) {
            char *h = sentry__string_from_wstr(buf);
            if (h) {
                sentry__rate_limiter_update_from_http_retry_after(
                    ts->transport_state->rl, h);
                sentry_free(h);
            }
        }
    } else {
        SENTRY_DEBUGF("http request failed with error: %d", GetLastError());
    }

    WinHttpCloseHandle(request);
    sentry_free(url);
    sentry_free(headers);
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
winhttp_transport_send_envelope(sentry_envelope_t *envelope, void *_state)
{
    winhttp_transport_state_t *state = _state;
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
sentry__winhttp_dump(void *task_data, void *run)
{
    struct task_state *ts = task_data;
    sentry__run_write_envelope((sentry_run_t *)run, ts->envelope);
    return true;
}

size_t
sentry__winhttp_dump_queue(void *state)
{
    sentry_bgworker_t *bgworker
        = ((winhttp_transport_state_t *)state)->bgworker;

    return sentry__bgworker_foreach_matching(bgworker, task_exec_func,
        sentry__winhttp_dump, sentry_get_options()->run);
}

sentry_transport_t *
sentry__transport_new_default(void)
{
    SENTRY_DEBUG("initializing winhttp transport");
    winhttp_transport_state_t *state = new_transport_state();
    if (!state) {
        return NULL;
    }

    sentry_transport_t *transport
        = sentry_transport_new(winhttp_transport_send_envelope);

    if (!transport) {
        winhttp_transport_free(state);
        return NULL;
    }
    sentry_transport_set_state(transport, state);
    sentry_transport_set_free_func(transport, winhttp_transport_free);
    sentry_transport_set_startup_func(transport, winhttp_transport_start);
    sentry_transport_set_shutdown_func(transport, winhttp_transport_shutdown);
    sentry__transport_set_dump_func(transport, sentry__winhttp_dump_queue);

    return transport;
}
