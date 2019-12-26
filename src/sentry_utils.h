#ifndef SENTRY_UTILS_H_INCLUDED
#define SENTRY_UTILS_H_INCLUDED

#include <sentry.h>
#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
#    include <winnt.h>
#else
#    include <sys/time.h>
#endif

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

int sentry__url_parse(sentry_url_t *url_out, const char *url);

void sentry__url_cleanup(sentry_url_t *url);

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

int sentry__dsn_parse(sentry_dsn_t *dsn_out, const char *dsn);

void sentry__dsn_cleanup(sentry_dsn_t *dsn);

#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
#    define EINTR_RETRY(X, Y)                                                  \
        do {                                                                   \
            int _tmp = (X);                                                    \
            if (Y) {                                                           \
                *(int *)Y = _tmp;                                              \
            }                                                                  \
        } while (false)
#else
#    define EINTR_RETRY(X, Y)                                                  \
        do {                                                                   \
            int _tmp;                                                          \
            do {                                                               \
                _tmp = (X);                                                    \
            } while (_tmp == -1 && errno == EINTR);                            \
            if (Y) {                                                           \
                *(int *)Y = _tmp;                                              \
            }                                                                  \
        } while (false)
#endif

/* returns the number of milliseconds since epoch. */
static inline uint64_t
sentry__msec_time()
{
#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
    SYSTEMTIME st;
    FILETIME file_time;
    GetSystemTime(&st);
    SystemTimeToFileTime(&system_time, &file_time);
    return (((uint64_t)file_time.dwLowDateTime
                    + (uint64_t)file_time.dwHighDateTime
                << 32)
               - 116444736000000000ULL)
        / 10000000ULL
        + system_time.wMilliseconds;
#else
    struct timeval tv;
    return (gettimeofday(&tv, NULL) == 0)
        ? (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000
        : 0;
#endif
}

#endif