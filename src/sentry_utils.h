#ifndef SENTRY_UTILS_H_INCLUDED
#define SENTRY_UTILS_H_INCLUDED

#include "sentry_boot.h"

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <winnt.h>
#else
#    include <sys/time.h>
#endif

/**
 * This represents a URL parsed into its different parts.
 */
typedef struct {
    char *scheme;
    char *host;
    int port;
    char *path;
    char *query;
    char *fragment;
    char *username;
    char *password;
} sentry_url_t;

/**
 * Parse the given `url` into the pre-allocated `url_out` parameter.
 * Returns 0 on success.
 */
int sentry__url_parse(sentry_url_t *url_out, const char *url);

/**
 * This will free all the internal members of `url`, but not `url` itself, as
 * that might have been stack allocated.
 */
void sentry__url_cleanup(sentry_url_t *url);

/**
 * This is the internal representation of a parsed DSN.
 */
typedef struct {
    bool is_secure;
    char *host;
    int port;
    char *secret_key;
    char *public_key;
    uint64_t project_id;
    char *path;
    bool empty;
} sentry_dsn_t;

/**
 * This will parse the DSN URL given in `dsn` into the pre-allocated `dsn_out`.
 * Returns 0 on success.
 */
int sentry__dsn_parse(sentry_dsn_t *dsn_out, const char *dsn);

/**
 * This will free all the internal members of `dsn`, but not `dsn` itself, as
 * that might have been stack allocated.
 */
void sentry__dsn_cleanup(sentry_dsn_t *dsn);

/**
 * This will create a new string, with the contents of the `X-Sentry-Auth`, as
 * described here:
 * https://docs.sentry.io/development/sdk-dev/overview/#authentication
 */
char *sentry__dsn_get_auth_header(const sentry_dsn_t *dsn);

/**
 * This returns a new string, with the URL for normal event and envelope
 * uploads.
 */
char *sentry__dsn_get_store_url(const sentry_dsn_t *dsn);

/**
 * This returns a new string, with the URL for minidump uploads.
 */
char *sentry__dsn_get_minidump_url(const sentry_dsn_t *dsn);

/**
 * This returns a new string, with the URL for attachment uploads.
 */
char *sentry__dsn_get_attachment_url(
    const sentry_dsn_t *dsn, const sentry_uuid_t *event_id);

/**
 * Returns the number of milliseconds since the unix epoch.
 */
static inline uint64_t
sentry__msec_time(void)
{
#ifdef SENTRY_PLATFORM_WINDOWS
    // Contains a 64-bit value representing the number of 100-nanosecond
    // intervals since January 1, 1601 (UTC).
    FILETIME file_time;
    SYSTEMTIME system_time;
    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);

    uint64_t timestamp = (uint64_t)file_time.dwLowDateTime
        + ((uint64_t)file_time.dwHighDateTime << 32);
    timestamp -= 116444736000000000LL; // convert to unix epoch
    timestamp /= 10000LL; // 100ns -> 1ms

    return timestamp;
#else
    struct timeval tv;
    return (gettimeofday(&tv, NULL) == 0)
        ? (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000
        : 0;
#endif
}

/**
 * Formats a timestamp (milliseconds since epoch) into ISO8601 format.
 */
char *sentry__msec_time_to_iso8601(uint64_t time);

#endif
