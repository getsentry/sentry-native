#ifndef SENTRY_CORE_H_INCLUDED
#define SENTRY_CORE_H_INCLUDED

#include "sentry_path.h"
#include "sentry_utils.h"
#include <sentry.h>

struct sentry_options_s {
    char *raw_dsn;
    sentry_dsn_t dsn;
    char *release;
    char *environment;
    char *dist;
    char *http_proxy;
    char *ca_certs;
    sentry_path_t *database_path;
    sentry_path_t *handler_path;
    bool debug;
    bool require_user_consent;

    sentry_transport_t *transport;
    sentry_event_function_t before_send_func;
    void *before_send_data;

    /* everything from here on down are options which are stored here but
       not exposed through the options API */
    sentry_user_consent_t user_consent;
};

#endif