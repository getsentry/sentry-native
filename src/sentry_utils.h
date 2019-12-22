#ifndef SENTRY_UTILS_H_INCLUDED
#define SENTRY_UTILS_H_INCLUDED

#include <sentry.h>
#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
#    include <winnt.h>
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

static inline int
sentry__atomic_fetch_and_add(volatile int *val, int diff)
{
#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
    return ::InterlockedExchangeAdd(ptr, value);
#else
    return __sync_fetch_and_add(val, diff);
#endif
}

static inline int
sentry__atomic_fetch(volatile int *val)
{
    return sentry__atomic_fetch_and_add(val, 0);
}

#endif