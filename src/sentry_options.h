#ifndef SENTRY_OPTIONS_H_INCLUDED
#define SENTRY_OPTIONS_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_utils.h"

typedef struct sentry_path_s sentry_path_t;
typedef struct sentry_run_s sentry_run_t;
struct sentry_backend_s;

/**
 * This is a linked list of all the attachments registered via
 * `sentry_options_add_attachment`.
 */
typedef struct sentry_attachment_s sentry_attachment_t;
struct sentry_attachment_s {
    char *name;
    sentry_path_t *path;
    sentry_attachment_t *next;
};

/**
 * This is the main options struct, which is being accessed throughout all of
 * the sentry internals.
 */
typedef struct sentry_options_s {
    char *raw_dsn;
    sentry_dsn_t dsn;
    double sample_rate;
    char *release;
    char *environment;
    char *dist;
    char *http_proxy;
    char *ca_certs;
    sentry_path_t *database_path;
    sentry_path_t *handler_path;
    bool debug;
    bool require_user_consent;
    bool system_crash_reporter_enabled;

    sentry_attachment_t *attachments;
    sentry_run_t *run;

    sentry_transport_t *transport;
    sentry_event_function_t before_send_func;
    void *before_send_data;

    /* everything from here on down are options which are stored here but
       not exposed through the options API */
    struct sentry_backend_s *backend;
    sentry_user_consent_t user_consent;
} sentry_options_t;

#endif
