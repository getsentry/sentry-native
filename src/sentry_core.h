#ifndef SENTRY_CORE_H_INCLUDED
#define SENTRY_CORE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_database.h"
#include "sentry_path.h"
#include "sentry_utils.h"

#define SENTRY_BREADCRUMBS_MAX 100

#ifdef SENTRY_PLATFORM_ANDROID
#    include <android/log.h>
#    define SENTRY_FPRINTF(fd, message, ...)                                   \
        __android_log_print(                                                   \
            ANDROID_LOG_INFO, "sentry-native", message, __VA_ARGS__)
#else
#    define SENTRY_FPRINTF(fd, message, ...)                                   \
        fprintf(fd, "[sentry] " message, __VA_ARGS__)
#endif
#define SENTRY_DEBUGF(message, ...)                                            \
    do {                                                                       \
        const sentry_options_t *_options = sentry_get_options();               \
        if (_options && sentry_options_get_debug(_options)) {                  \
            SENTRY_FPRINTF(stderr, message "\n", __VA_ARGS__);                 \
        }                                                                      \
    } while (0)

#define SENTRY_DEBUG(message) SENTRY_DEBUGF("%s", message "")

// TODO: we might want to have different log levels at some point
#define SENTRY_TRACEF(message, ...) SENTRY_DEBUGF(message, __VA_ARGS__)
#define SENTRY_TRACE(message) SENTRY_DEBUG(message)

#if defined(__GNUC__) && (__GNUC__ >= 4)
#    define MUST_USE __attribute__((warn_unused_result))
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
#    define MUST_USE _Check_return_
#else
#    define MUST_USE
#endif

#ifdef __GNUC__
#    define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#    define UNUSED(x) UNUSED_##x
#endif

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
struct sentry_options_s {
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
};

/**
 * This will free a previously allocated attachment.
 */
void sentry__attachment_free(sentry_attachment_t *attachment);

/**
 * This function will check the user consent, and return `true` if uploads
 * should *not* be sent to the sentry server, and be discarded instead.
 */
bool sentry__should_skip_upload(void);

/**
 * This function is essential to capture reports in the case of a hard crash.
 * It will set a special transport that will dump events to disk.
 * See `sentry__run_write_envelope`.
 */
void sentry__enforce_disk_transport(void);

/**
 * This function will submit the given `envelope` to the configured transport.
 */
void sentry__capture_envelope(sentry_envelope_t *envelope);

/**
 * Generates a new random UUID for events.
 */
sentry_uuid_t sentry__new_event_id(void);

/**
 * This will ensure that the given `event` has a UUID, generating a new one on
 * demand. It will return a serialized UUID as `sentry_value_t` and also write
 * it into the `uuid_out` parameter.
 */
sentry_value_t sentry__ensure_event_id(
    sentry_value_t event, sentry_uuid_t *uuid_out);

#endif
