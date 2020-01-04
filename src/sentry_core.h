#ifndef SENTRY_CORE_H_INCLUDED
#define SENTRY_CORE_H_INCLUDED

#include "sentry_path.h"
#include "sentry_utils.h"
#include <sentry.h>

#define SENTRY_SDK_NAME "sentry-native"
#define SENTRY_SDK_VERSION "0.2.0"
#define SENTRY_SDK_USER_AGENT (SENTRY_SDK_NAME "/" SENTRY_SDK_VERSION)
#define SENTRY_BREADCRUMBS_MAX 100

struct sentry_backend_s;

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
    struct sentry_backend_s *backend;
    sentry_user_consent_t user_consent;
};

bool sentry__should_skip_upload(void);

sentry_uuid_t sentry__new_event_id(void);
sentry_value_t sentry__ensure_event_id(
    sentry_value_t event, sentry_uuid_t *uuid_out);

#endif