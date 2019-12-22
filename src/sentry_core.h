#ifndef SENTRY_CORE_H_INCLUDED
#define SENTRY_CORE_H_INCLUDED

#include "sentry_utils.h"
#include <sentry.h>

struct sentry_options_s {
    char *raw_dsn;
    sentry__dsn_t dsn;
    char *release;
    char *environment;
    char *dist;
    char *http_proxy;
    char *ca_certs;
    bool debug;

    /* everything from here on down are options which are stored here but
       not exposed through the options API */
    sentry_user_consent_t user_consent;
};

#endif