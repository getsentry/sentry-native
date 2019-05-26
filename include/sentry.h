#ifndef SENTRY_H
#define SENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SENTRY_API
#ifdef _WIN32
#if defined(SENTRY_BUILD_SHARED) /* build dll */
#define SENTRY_API __declspec(dllexport)
#elif !defined(SENTRY_BUILD_STATIC) /* use dll */
#define SENTRY_API __declspec(dllimport)
#else /* static library */
#define SENTRY_API
#endif
#else
#if __GNUC__ >= 4
#define SENTRY_API __attribute__((visibility("default")))
#else
#define SENTRY_API
#endif
#endif
#endif

// Char types
#ifdef _WIN32
#include <wchar.h>
typedef wchar_t xchar_t;
#define XSTR(X) L##X
#else
typedef char xchar_t;
#define XSTR(X) X
#endif

/*
 * Possible error codes.
 */
enum sentry_error_t {
    SENTRY_ERROR_NULL_ARGUMENT = 1,
    SENTRY_ERROR_HANDLER_STARTUP_FAIL = 2,
    SENTRY_ERROR_NO_DSN = 3,
    SENTRY_ERROR_NO_MINIDUMP_URL = 4,
    SENTRY_ERROR_INVALID_URL_SCHEME = 5,
    SENTRY_ERROR_INVALID_URL_MISSING_HOST = 6,
    SENTRY_ERROR_BREADCRUMB_SERIALIZATION = 7,
    SENTRY_ERROR_SERIALIZING_SENTRY_RUN_INFO = 8,
    SENTRY_ERROR_DESERIALIZING_SENTRY_RUN_INFO = 9,
    SENTRY_ERROR_FAILED_READING_DATABASE_DIRECTORY = 10,
};

/*
 * Sentry levels for events and breadcrumbs.
 */
enum sentry_level_t {
    SENTRY_LEVEL_DEBUG = -1,
    SENTRY_LEVEL_INFO = 0, /* defaults to info */
    SENTRY_LEVEL_WARNING = 1,
    SENTRY_LEVEL_ERROR = 2,
    SENTRY_LEVEL_FATAL = 3,
};

typedef struct sentry_options_s {
    /* Unified API */
    const char *dsn;
    const char *release;
    const char *environment;
    const char *dist;
    int debug;
    const xchar_t **attachments;
    /* Crashpad */
    const xchar_t *handler_path;
    /* The base working directory */
    const xchar_t *database_path;
} sentry_options_t;

/*
 * A breadcrumb sent as part of an event.
 */
typedef struct sentry_breadcrumb_s {
    const char *message;
    const char *type;
    const char *category;
    const enum sentry_level_t level;
} sentry_breadcrumb_t;

/*
 * The user affected by the event.
 */
typedef struct sentry_user_s {
    const char *username;
    const char *email;
    const char *id;
    const char *ip_address;
} sentry_user_t;

/* Unified API */

/*
 * Initializes the Sentry SDK with the specified options.
 */
SENTRY_API int sentry_init(const sentry_options_t *options);
/*
 * Adds the breadcrumb to be sent in case of an event.
 */
SENTRY_API int sentry_add_breadcrumb(sentry_breadcrumb_t *breadcrumb);
/*
 * Sets the specified user.
 */
SENTRY_API int sentry_set_user(const sentry_user_t *user);
/*
 * Removes a user.
 */
SENTRY_API int sentry_remove_user();
/*
 * Sets a tag.
 */
SENTRY_API int sentry_set_tag(const char *key, const char *value);
/*
 * Removes the tag with the specified key.
 */
SENTRY_API int sentry_remove_tag(const char *key);
/*
 * Sets extra information.
 */
SENTRY_API int sentry_set_extra(const char *key, const char *value);
/*
 * Removes the extra with the specified key.
 */
SENTRY_API int sentry_remove_extra(const char *key);
/*
 * Sets the release.
 */
SENTRY_API int sentry_set_release(const char *release);
/*
 * Removes the release.
 */
SENTRY_API int sentry_remove_release();
/*
 * Sets the event fingerprint.
 */
SENTRY_API int sentry_set_fingerprint(const char *fingerprint, ...);
/*
 * Removes the fingerprint.
 */
SENTRY_API int sentry_remove_fingerprint();
/*
 * Sets the transaction.
 */
SENTRY_API int sentry_set_transaction(const char *transaction);
/*
 * Removes the transaction.
 */
SENTRY_API int sentry_remove_transaction();
/*
 * Sets the event level.
 */
SENTRY_API int sentry_set_level(enum sentry_level_t level);

/* Sentrypad custom API */

/*
 * Clears the values of the specified user.
 */
SENTRY_API void sentry_user_clear(sentry_user_t *user);
/*
 * Initializes the Sentry options.
 */
SENTRY_API void sentry_options_init(sentry_options_t *options);
/*
 * Closes the SDK.
 */
SENTRY_API int sentry_shutdown(void);
/*
 * Captures a minidump.
 */
SENTRY_API int sentry_capture_minidump(const char *optional_message);
/*
 * Pushes a new scope.
 */
SENTRY_API int sentry_push_scope();
/*
 * Pops the current scope.
 */
SENTRY_API int sentry_pop_scope();

#ifdef __cplusplus
}
#endif
#endif
