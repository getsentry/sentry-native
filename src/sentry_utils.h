#ifndef SENTRY_UTILS_H_INCLUDED
#define SENTRY_UTILS_H_INCLUDED

#include <sentry.h>

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

int sentry_url_parse(sentry_url_t *url_out, const char *url);

void sentry_url_cleanup(sentry_url_t *url);

#endif