#ifndef SENTRY_H
#define SENTRY_H

#include <stddef.h>
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

enum sentry_error_t {
    SENTRY_ERROR_NULL_ARGUMENT = 1,
    SENTRY_ERROR_HANDLER_STARTUP_FAIL = 2,
    SENTRY_ERROR_NO_DSN = 3,
    SENTRY_ERROR_NO_MINIDUMP_URL = 4,
    SENTRY_ERROR_INVALID_URL_SCHEME = 5,
    SENTRY_ERROR_INVALID_URL_MISSING_HOST = 6,
};

typedef struct sentry_options_s {
    // Unified API
    const char *dsn;
    const char *release;
    const char *environment;
    const char *dist;
    int debug;
    // Crashpad
    const char *handler_path;
    const char *database_path;
    // TODO hook/callback to crashpad configuration object.
    // Breakpad
    // TODO: whatever breakpad needs
} sentry_options_t;

typedef struct sentry_breadcrumb_s {
} sentry_breadcrumb_t;

typedef struct sentry_user_s {
    const char *username;
    const char *email;
    const char *id;
    const char *ip_address;
} sentry_user_t;

enum sentry_level_t {
    SENTRY_LEVEL_DEBUG = 0,
    SENTRY_LEVEL_INFO = 1,
    SENTRY_LEVEL_WARNING = 2,
    SENTRY_LEVEL_ERROR = 3
};

// Unified API
SENTRY_API int sentry_init(const sentry_options_t *options);
SENTRY_API int sentry_add_breadcrumb(sentry_breadcrumb_t *breadcrumb);
SENTRY_API int sentry_push_scope();
SENTRY_API int sentry_pop_scope();
SENTRY_API int sentry_set_user(const sentry_user_t *user);
SENTRY_API int sentry_remove_user();
SENTRY_API int sentry_set_tag(const char *key, const char *value);
SENTRY_API int sentry_remove_tag(const char *key);
SENTRY_API int sentry_set_extra(const char *key, const char *value);
SENTRY_API int sentry_remove_extra(const char *key);
SENTRY_API int sentry_set_release(const char *release);
SENTRY_API int sentry_remove_release();
SENTRY_API int sentry_set_fingerprint(const char **fingerprint, size_t len);
SENTRY_API int sentry_remove_fingerprint();
SENTRY_API int sentry_set_transaction(const char *transaction);
SENTRY_API int sentry_remove_transaction();
SENTRY_API int sentry_set_level(enum sentry_level_t level);

/* helpers */
SENTRY_API void sentry_user_clear(sentry_user_t *user);

/* Sentrypad custom API */
SENTRY_API int sentry_shutdown(void);
SENTRY_API int sentry_attach_file_by_path(const char *path);
SENTRY_API int sentry_attach_file_with_contents(const char *filename,
                                                const char *buf,
                                                size_t len);
SENTRY_API int sentry_capture_minidump(const char *optional_message);
SENTRY_API void sentry_options_init(sentry_options_t *options);

#ifdef __cplusplus
}
#endif
#endif
