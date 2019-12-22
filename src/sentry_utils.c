#include "sentry_utils.h"
#include "sentry_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
is_scheme_valid(const char *scheme_name)
{
    char c;
    while ((c = *scheme_name++) != 0) {
        if (!isalpha(c) && c != '+' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
}

int
sentry__url_parse(sentry_url_t *url_out, const char *url)
{
    bool has_username;
    int result = 0;
    char *scratch = sentry__string_dup(url);
    char *ptr = scratch;
    char *tmp;
    char *aux_buf = 0;
    memset(url_out, 0, sizeof(sentry_url_t));

#define SKIP_WHILE_NOT(ptr, c)                                                 \
    do {                                                                       \
        while (*ptr && *ptr != c)                                              \
            ptr++;                                                             \
    } while (false)
#define SKIP_WHILE_NOT2(ptr, c1, c2)                                           \
    do {                                                                       \
        while (*ptr && *ptr != c1 && *ptr != c2)                               \
            ptr++;                                                             \
    } while (false)

    if (!scratch) {
        goto error;
    }

    /* scheme */
    tmp = strchr(ptr, ':');
    if (!tmp) {
        goto error;
    }
    url_out->scheme = sentry__string_dupn(ptr, tmp - ptr);

    if (!is_scheme_valid(url_out->scheme)) {
        goto error;
    }
    sentry__string_ascii_lower(url_out->scheme);

    ptr = tmp + 1;

    /* scheme trailer */
    if (*ptr++ != '/') {
        goto error;
    }
    if (*ptr++ != '/') {
        goto error;
    }

    // auth
    has_username = false;
    tmp = ptr;
    while (*tmp) {
        if (*tmp == '@') {
            has_username = true;
            break;
        } else if (*tmp == '/') {
            has_username = false;
            break;
        }
        tmp++;
    }
    tmp = ptr;
    if (has_username) {
        SKIP_WHILE_NOT2(tmp, '@', ':');
        url_out->username = sentry__string_dupn(ptr, tmp - ptr);
        ptr = tmp;
        if (*ptr == ':') {
            ptr++;
            tmp = ptr;
            SKIP_WHILE_NOT(tmp, '@');
            url_out->password = sentry__string_dupn(ptr, tmp - ptr);
            ptr = tmp;
        }
        if (*ptr != '@') {
            goto error;
        }
        ptr++;
    }

    /* host */
    bool is_ipv6 = *ptr == '[';
    tmp = ptr;
    while (*tmp) {
        if (is_ipv6 && *tmp == ']') {
            tmp++;
            break;
        } else if (!is_ipv6 && (*tmp == ':' || *tmp == '/')) {
            break;
        }

        tmp++;
    }

    url_out->host = sentry__string_dupn(ptr, tmp - ptr);

    /* port */
    ptr = tmp;
    if (*ptr == ':') {
        ptr++;
        tmp = ptr;
        SKIP_WHILE_NOT(tmp, '/');
        aux_buf = sentry__string_dupn(ptr, tmp - ptr);
        char *end;
        url_out->port = strtol(aux_buf, &end, 10);
        if (end != aux_buf + strlen(aux_buf)) {
            goto error;
        }
        sentry_free(aux_buf);
        aux_buf = 0;
        ptr = tmp;
    }

    if (!*ptr) {
        goto error;
    }

    /* end of netloc */
    if (*ptr != '/') {
        goto error;
    }

    /* path */
    tmp = ptr;
    SKIP_WHILE_NOT2(tmp, '#', '?');
    url_out->path = sentry__string_dupn(ptr, tmp - ptr);
    ptr = tmp;

    /* query */
    if (*ptr == '?') {
        ptr++;
        tmp = ptr;
        SKIP_WHILE_NOT(tmp, '#');
        url_out->query = sentry__string_dupn(ptr, tmp - ptr);
        ptr = tmp;
    }

    /* fragment */
    if (*ptr == '#') {
        ptr++;
        tmp = ptr;
        SKIP_WHILE_NOT(tmp, 0);
        url_out->fragment = sentry__string_dupn(ptr, tmp - ptr);
        ptr = tmp;
    }

    if (url_out->port == 0) {
        if (strcmp(url_out->scheme, "https") == 0) {
            url_out->port = 443;
        } else if (strcmp(url_out->scheme, "http") == 0) {
            url_out->port = 80;
        }
    }

    result = 0;
    goto cleanup;

error:
    result = 1;
    sentry__url_cleanup(url_out);
    goto cleanup;

cleanup:
    sentry_free(aux_buf);
    sentry_free(scratch);
    return result;
}

void
sentry__url_cleanup(sentry_url_t *url)
{
    sentry_free(url->scheme);
    sentry_free(url->host);
    sentry_free(url->path);
    sentry_free(url->query);
    sentry_free(url->fragment);
    sentry_free(url->username);
    sentry_free(url->password);
}

int
sentry__dsn_parse(sentry_dsn_t *dsn_out, const char *dsn)
{
    sentry_url_t url;
    size_t path_len;
    char *tmp;
    char *end;

    memset(dsn_out, 0, sizeof(sentry_dsn_t));

    if (!dsn || !dsn[0]) {
        dsn_out->empty = true;
        return 0;
    }

    if (sentry__url_parse(&url, dsn) != 0) {
        return 1;
    }

    if (strcmp(url.scheme, "https") == 0) {
        dsn_out->is_secure = 1;
    } else if (strcmp(url.scheme, "http") == 0) {
        dsn_out->is_secure = 0;
    } else {
        goto error;
    }

    dsn_out->host = url.host;
    url.host = NULL;
    dsn_out->public_key = url.username;
    url.username = NULL;
    dsn_out->secret_key = url.password;
    url.password = NULL;
    dsn_out->port = url.port;

    path_len = strlen(url.path);
    while (path_len > 0 && url.path[path_len - 1] == '/') {
        url.path[path_len - 1] = '\0';
        path_len--;
    }

    tmp = strrchr(url.path, '/');
    if (!tmp) {
        goto error;
    }

    dsn_out->project_id = (uint64_t)strtoll(tmp + 1, &end, 10);
    if (end != tmp + strlen(tmp)) {
        goto error;
    }
    *tmp = 0;
    dsn_out->path = url.path;
    url.path = NULL;
    dsn_out->empty = false;

    sentry__url_cleanup(&url);
    return 0;

error:
    sentry__url_cleanup(&url);
    sentry__dsn_cleanup(dsn_out);
    return 1;
}

void
sentry__dsn_cleanup(sentry_dsn_t *dsn)
{
    sentry_free(dsn->host);
    sentry_free(dsn->path);
    sentry_free(dsn->public_key);
    sentry_free(dsn->secret_key);
}