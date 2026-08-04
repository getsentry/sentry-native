#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_http_transport.h"
#include "sentry_options.h"
#include "sentry_slice.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "sentry_utils.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifndef SENTRY_LINK_CURL
#    include <dlfcn.h>
#    ifndef RTLD_LOCAL
#        define RTLD_LOCAL 0
#    endif
#endif

#ifdef SENTRY_PLATFORM_NX
#    include "sentry_transport_curl_nx.h"
#endif

typedef struct {
#ifndef SENTRY_LINK_CURL
    void *handle;
#endif
    CURLcode (*global_init)(long flags);
    curl_version_info_data *(*version_info)(CURLversion version);
    void (*global_cleanup)(void);
    CURL *(*easy_init)(void);
    void (*easy_cleanup)(CURL *curl);
    void (*easy_reset)(CURL *curl);
    CURLcode (*easy_setopt)(CURL *curl, CURLoption option, ...);
    CURLcode (*easy_getinfo)(CURL *curl, CURLINFO info, ...);
    CURLcode (*easy_perform)(CURL *curl);
    const char *(*easy_strerror)(CURLcode errornum);
    struct curl_slist *(*slist_append)(
        struct curl_slist *list, const char *data);
    void (*slist_free_all)(struct curl_slist *list);
} curl_table_t;

static curl_table_t g_curl;

#ifndef SENTRY_LINK_CURL
static void
curl_unload(void)
{
    if (g_curl.handle) {
        dlclose(g_curl.handle);
    }
    memset(&g_curl, 0, sizeof(g_curl));
}

static bool
curl_resolve_symbol(const char *name, void **func)
{
    dlerror();
    void *symbol = dlsym(g_curl.handle, name);
    const char *error = dlerror();
    if (error || !symbol) {
        SENTRY_WARNF("failed to resolve libcurl symbol `%s`: %s", name,
            error ? error : "symbol not found");
        return false;
    }

    *func = symbol;
    return true;
}

static int
curl_load(void)
{
    if (g_curl.handle) {
        return 0;
    }

#    ifdef SENTRY_PLATFORM_MACOS
    const char *library_name = "libcurl.4.dylib";
#    else
    const char *library_name = "libcurl.so.4";
#    endif

    dlerror();
    g_curl.handle = dlopen(library_name, RTLD_LAZY | RTLD_LOCAL);
    if (!g_curl.handle) {
        const char *error = dlerror();
        SENTRY_WARNF("failed to load %s: %s", library_name,
            error ? error : "library not found");
        return 1;
    }

#    define RESOLVE_CURL_SYMBOL(symbol, field)                                 \
        do {                                                                   \
            if (!curl_resolve_symbol(#symbol, (void **)&g_curl.field)) {       \
                curl_unload();                                                 \
                return 1;                                                      \
            }                                                                  \
        } while (0)

    RESOLVE_CURL_SYMBOL(curl_global_init, global_init);
    RESOLVE_CURL_SYMBOL(curl_version_info, version_info);
    RESOLVE_CURL_SYMBOL(curl_global_cleanup, global_cleanup);
    RESOLVE_CURL_SYMBOL(curl_easy_init, easy_init);
    RESOLVE_CURL_SYMBOL(curl_easy_cleanup, easy_cleanup);
    RESOLVE_CURL_SYMBOL(curl_easy_reset, easy_reset);
    RESOLVE_CURL_SYMBOL(curl_easy_setopt, easy_setopt);
    RESOLVE_CURL_SYMBOL(curl_easy_getinfo, easy_getinfo);
    RESOLVE_CURL_SYMBOL(curl_easy_perform, easy_perform);
    RESOLVE_CURL_SYMBOL(curl_easy_strerror, easy_strerror);
    RESOLVE_CURL_SYMBOL(curl_slist_append, slist_append);
    RESOLVE_CURL_SYMBOL(curl_slist_free_all, slist_free_all);

#    undef RESOLVE_CURL_SYMBOL

    return 0;
}
#else
static int
curl_load(void)
{
    g_curl.global_init = curl_global_init;
    g_curl.version_info = curl_version_info;
    g_curl.global_cleanup = curl_global_cleanup;
    g_curl.easy_init = curl_easy_init;
    g_curl.easy_cleanup = curl_easy_cleanup;
    g_curl.easy_reset = curl_easy_reset;
    g_curl.easy_setopt = curl_easy_setopt;
    g_curl.easy_getinfo = curl_easy_getinfo;
    g_curl.easy_perform = curl_easy_perform;
    g_curl.easy_strerror = curl_easy_strerror;
    g_curl.slist_append = curl_slist_append;
    g_curl.slist_free_all = curl_slist_free_all;

    return 0;
}
#endif

typedef struct {
    CURL *curl_handle;
    char *proxy;
    char *ca_certs;
    bool debug;
    uint64_t transfer_timeout;
    long shutdown;
#ifdef SENTRY_PLATFORM_NX
    void *nx_state;
#endif
} curl_client_t;

typedef struct {
    FILE *file;
    const sentry_path_t *path;
} file_body_t;

static curl_client_t *
curl_client_new(void)
{
    curl_client_t *client = SENTRY_MAKE(curl_client_t);
    if (!client) {
        return NULL;
    }

#ifdef SENTRY_PLATFORM_NX
    client->nx_state = sentry_nx_curl_state_new();
#endif
    return client;
}

static void
curl_client_free(void *_client)
{
    curl_client_t *client = _client;
    if (client->curl_handle) {
        g_curl.easy_cleanup(client->curl_handle);
        g_curl.global_cleanup();
    }
    sentry_free(client->ca_certs);
    sentry_free(client->proxy);
#ifdef SENTRY_PLATFORM_NX
    sentry_nx_curl_state_free(client->nx_state);
#endif
    sentry_free(client);
}

static int
curl_client_start(void *_client, const sentry_options_t *options)
{
    curl_client_t *client = _client;

    if (curl_load() != 0) {
        return 1;
    }

    static bool curl_initialized = false;
    if (!curl_initialized) {
        CURLcode rv = g_curl.global_init(CURL_GLOBAL_ALL);
        if (rv != CURLE_OK) {
            SENTRY_WARNF("`curl_global_init` failed with code `%d`", (int)rv);
            return 1;
        }

        curl_version_info_data *version_data
            = g_curl.version_info(CURLVERSION_NOW);

        if (!version_data) {
            SENTRY_WARN("Failed to retrieve `curl_version_info`");
            return 1;
        }

        sentry_version_t curl_version = {
            .major = (version_data->version_num >> 16) & 0xff,
            .minor = (version_data->version_num >> 8) & 0xff,
            .patch = version_data->version_num & 0xff,
        };

        if (!sentry__check_min_version(
                curl_version, (sentry_version_t) { 7, 21, 7 })) {
            SENTRY_WARNF("`libcurl` is at unsupported version `%u.%u.%u`",
                curl_version.major, curl_version.minor, curl_version.patch);
            return 1;
        }

        if ((version_data->features & CURL_VERSION_ASYNCHDNS) == 0) {
            SENTRY_WARN("`libcurl` was not compiled with feature `AsynchDNS`");
            return 1;
        }
    }

    client->proxy = sentry__string_clone(options->proxy);
    client->ca_certs = sentry__string_clone(options->ca_certs);
    client->curl_handle = g_curl.easy_init();
    client->debug = options->debug;
    client->transfer_timeout = options->transfer_timeout;

    if (!client->curl_handle) {
        // In this case we don't start the worker at all, which means we can
        // still dump all unsent envelopes to disk on shutdown.
        SENTRY_WARN("`curl_easy_init` failed");
        return 1;
    }

#ifdef SENTRY_PLATFORM_NX
    if (!sentry_nx_transport_start(client->nx_state, options)) {
        return 1;
    }
#endif

    return 0;
}

static long
curl_timeout_ms(uint64_t timeout)
{
    return timeout > (uint64_t)LONG_MAX ? LONG_MAX : (long)timeout;
}

static void
curl_client_shutdown(void *_client)
{
    curl_client_t *client = _client;
    sentry__atomic_store(&client->shutdown, 1);
}

static int
progress_callback(void *clientp, curl_off_t UNUSED(dltotal),
    curl_off_t UNUSED(dlnow), curl_off_t UNUSED(ultotal),
    curl_off_t UNUSED(ulnow))
{
    curl_client_t *client = clientp;
    return sentry__atomic_fetch(&client->shutdown) ? 1 : 0;
}

static size_t
swallow_data(
    char *UNUSED(ptr), size_t size, size_t nmemb, void *UNUSED(userdata))
{
    return size * nmemb;
}

static int
debug_callback(CURL *UNUSED(handle), curl_infotype type, char *data,
    size_t size, void *UNUSED(userdata))
{
    const char *prefix;
    switch (type) {
    case CURLINFO_TEXT:
        prefix = "*";
        break;
    case CURLINFO_HEADER_OUT:
        prefix = ">";
        break;
    case CURLINFO_HEADER_IN:
        prefix = "<";
        break;
    default:
        return 0;
    }
    size_t start = 0;
    for (size_t i = 0; i <= size; i++) {
        if (i == size || data[i] == '\n') {
            size_t end = i;
            while (end > start && data[end - 1] == '\r') {
                end--;
            }
            if (end > start) {
                SENTRY_TRACEF(
                    "%s %.*s", prefix, (int)(end - start), data + start);
            }
            start = i + 1;
        }
    }
    return 0;
}

static size_t
write_callback(char *ptr, size_t size, size_t nmemb, void *UNUSED(userdata))
{
    size_t total = size * nmemb;
    size_t len = total;
    while (len > 0 && (ptr[len - 1] == '\n' || ptr[len - 1] == '\r')) {
        len--;
    }
    if (len > 0) {
        SENTRY_TRACEF("%.*s", (int)len, ptr);
    }
    return total;
}

static size_t
header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t bytes = size * nitems;
    sentry_http_response_t *info = userdata;
    char *header = sentry__string_clone_n(buffer, bytes);
    if (!header) {
        return bytes;
    }

    char *sep = strchr(header, ':');
    if (sep) {
        *sep = '\0';
        sentry__string_ascii_lower(header);
        sentry_slice_t value
            = sentry__slice_trim(sentry__slice_from_str(sep + 1));
        if (sentry__string_eq(header, "retry-after")) {
            sentry_free(info->retry_after);
            info->retry_after = sentry__slice_to_owned(value);
        } else if (sentry__string_eq(header, "x-sentry-rate-limits")) {
            sentry_free(info->x_sentry_rate_limits);
            info->x_sentry_rate_limits = sentry__slice_to_owned(value);
        } else if (sentry__string_eq(header, "location")) {
            sentry_free(info->location);
            info->location = sentry__slice_to_owned(value);
        }
    }

    sentry_free(header);
    return bytes;
}

static size_t
file_read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    file_body_t *body = userdata;
    if (size && nitems > SIZE_MAX / size) {
        goto fail;
    }
    size_t capacity = size * nitems;
    size_t read = fread(buffer, 1, capacity, body->file);
    if (read < capacity && ferror(body->file)) {
        goto fail;
    }
    return read;

fail:
    SENTRY_WARNF("failed to read request body file \"%s\"",
        sentry__path_filename(body->path));
    return CURL_READFUNC_ABORT;
}

static bool
curl_send_task(void *_client, sentry_prepared_http_request_t *req,
    sentry_http_response_t *resp)
{
    curl_client_t *client = (curl_client_t *)_client;

#ifdef SENTRY_PLATFORM_NX
    if (!sentry_nx_curl_connect(client->nx_state)) {
        return false; // TODO should we dump the envelope to disk?
    }
#endif

    struct curl_slist *headers = NULL;
    headers = g_curl.slist_append(headers, "expect:");
    for (size_t i = 0; i < req->headers_len; i++) {
        char buf[512];
        size_t written = (size_t)snprintf(buf, sizeof(buf), "%s:%s",
            req->headers[i].key, req->headers[i].value);
        if (written >= sizeof(buf)) {
            continue;
        }
        buf[written] = '\0';
        headers = g_curl.slist_append(headers, buf);
    }

    CURL *curl = client->curl_handle;
    g_curl.easy_reset(curl);
    if (client->debug) {
        g_curl.easy_setopt(curl, CURLOPT_VERBOSE, 1);
        g_curl.easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
        g_curl.easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    } else {
        g_curl.easy_setopt(curl, CURLOPT_WRITEFUNCTION, swallow_data);
    }
    g_curl.easy_setopt(curl, CURLOPT_URL, req->url);
    g_curl.easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    g_curl.easy_setopt(curl, CURLOPT_USERAGENT, SENTRY_SDK_USER_AGENT);
    g_curl.easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 15000L);
    g_curl.easy_setopt(
        curl, CURLOPT_TIMEOUT_MS, curl_timeout_ms(client->transfer_timeout));
    g_curl.easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    g_curl.easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    g_curl.easy_setopt(curl, CURLOPT_XFERINFODATA, client);

    FILE *body_file = NULL;
    file_body_t file_body = { 0 };
    if (req->body_path) {
#ifdef SENTRY_PLATFORM_WINDOWS
        body_file = _wfopen(req->body_path->path_w, L"rb");
#else
        body_file = fopen(req->body_path->path, "rb");
#endif
        if (!body_file) {
            SENTRY_WARNF("failed to open request body file \"%s\"",
                sentry__path_filename(req->body_path));
            g_curl.slist_free_all(headers);
            return false;
        }
        file_body.file = body_file;
        file_body.path = req->body_path;
        g_curl.easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        g_curl.easy_setopt(curl, CURLOPT_CUSTOMREQUEST, req->method);
        g_curl.easy_setopt(curl, CURLOPT_READFUNCTION, file_read_callback);
        g_curl.easy_setopt(curl, CURLOPT_READDATA, &file_body);
        g_curl.easy_setopt(
            curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)req->body_len);
    } else if (req->body) {
        g_curl.easy_setopt(curl, CURLOPT_POST, (long)1);
        g_curl.easy_setopt(curl, CURLOPT_POSTFIELDS, req->body);
        g_curl.easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req->body_len);
    } else {
        g_curl.easy_setopt(curl, CURLOPT_CUSTOMREQUEST, req->method);
    }

    char error_buf[CURL_ERROR_SIZE];
    error_buf[0] = 0;
    g_curl.easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buf);

    g_curl.easy_setopt(curl, CURLOPT_HEADERDATA, (void *)resp);
    g_curl.easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

    if (client->proxy) {
        g_curl.easy_setopt(curl, CURLOPT_PROXY, client->proxy);
    }
    if (client->ca_certs) {
        g_curl.easy_setopt(curl, CURLOPT_CAINFO, client->ca_certs);
    }

#ifdef SENTRY_PLATFORM_NX
    CURLcode rv = sentry_nx_curl_easy_setopt(client->nx_state, curl, req);
#else
    CURLcode rv = CURLE_OK;
#endif

    if (rv == CURLE_OK) {
        rv = g_curl.easy_perform(curl);
    }

    if (rv == CURLE_OK) {
        long response_code;
        g_curl.easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        resp->status_code = (int)response_code;
    } else {
        resp->shutdown = sentry__atomic_fetch(&client->shutdown) != 0;
        size_t len = strlen(error_buf);
        if (len) {
            if (error_buf[len - 1] == '\n') {
                error_buf[len - 1] = 0;
            }
            SENTRY_WARNF("`curl_easy_perform` failed with code `%d`: %s",
                (int)rv, error_buf);
        } else {
            SENTRY_WARNF("`curl_easy_perform` failed with code `%d`: %s",
                (int)rv, g_curl.easy_strerror(rv));
        }
    }

    if (body_file) {
        fclose(body_file);
    }
    g_curl.slist_free_all(headers);
    return rv == CURLE_OK;
}

sentry_transport_t *
sentry__transport_new_default(void)
{
    SENTRY_INFO("initializing curl transport");
    curl_client_t *client = curl_client_new();
    if (!client) {
        return NULL;
    }

    sentry_transport_t *transport
        = sentry__http_transport_new(client, curl_send_task);
    if (!transport) {
        curl_client_free(client);
        return NULL;
    }
    sentry__http_transport_set_free_client(transport, curl_client_free);
    sentry__http_transport_set_start_client(transport, curl_client_start);
    sentry__http_transport_set_shutdown_client(transport, curl_client_shutdown);
    return transport;
}
