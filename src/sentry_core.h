#ifndef SENTRY_CORE_H_INCLUDED
#define SENTRY_CORE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"
#include "sentry_utils.h"

#define SENTRY_SDK_NAME "sentry-native"
#define SENTRY_SDK_VERSION "0.2.0"
#define SENTRY_SDK_USER_AGENT (SENTRY_SDK_NAME "/" SENTRY_SDK_VERSION)
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

struct sentry_backend_s;

// We save the attachments as a linked list basically
typedef struct sentry_attachment_s sentry_attachment_t;
struct sentry_attachment_s {
    char *name;
    sentry_path_t *path;
    sentry_attachment_t *next;
};

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

    sentry_attachment_t *attachments;

    sentry_transport_t *transport;
    sentry_event_function_t before_send_func;
    void *before_send_data;

    /* everything from here on down are options which are stored here but
       not exposed through the options API */
    struct sentry_backend_s *backend;
    sentry_user_consent_t user_consent;
};

bool sentry__should_skip_upload(void);

void sentry__enforce_disk_transport(void);

sentry_uuid_t sentry__new_event_id(void);
sentry_value_t sentry__ensure_event_id(
    sentry_value_t event, sentry_uuid_t *uuid_out);

#endif
