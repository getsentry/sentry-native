/*
 * sentrypad
 *
 * sentrypad is an abstraction over crashpad/breakpad with some additional
 * features that improve the quality of errors sent to Sentry.
 */
#ifndef SENTRY_H_INCLUDED
#define SENTRY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SENTRY_API
#ifdef _WIN32
#include <wchar.h>
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

#define SENTRY_SDK_VERSION "0.0.1"

/*
 * Sentry levels for events and breadcrumbs.
 */
enum sentry_level_t {
    SENTRY_LEVEL_DEBUG = -1,
    SENTRY_LEVEL_INFO = 0,
    SENTRY_LEVEL_WARNING = 1,
    SENTRY_LEVEL_ERROR = 2,
    SENTRY_LEVEL_FATAL = 3,
};

struct sentry_options_s;
typedef struct sentry_options_s sentry_options_t;

/*
 * creates a new options struct.  Can be freed with `sentry_options_free`
 */
SENTRY_API sentry_options_t *sentry_options_new(void);

/*
 * deallocates previously allocated sentry options
 */
SENTRY_API void sentry_options_free(sentry_options_t *opts);

/*
 * sets the DSN
 */
SENTRY_API void sentry_options_set_dsn(sentry_options_t *opts, const char *dsn);

/*
 * gets the DSN
 */
SENTRY_API const char *sentry_options_get_dsn(const sentry_options_t *opts);

/*
 * sets the release
 */
SENTRY_API void sentry_options_set_release(sentry_options_t *opts,
                                           const char *release);

/*
 * gets the release
 */
SENTRY_API const char *sentry_options_get_release(const sentry_options_t *opts);

/*
 * sets the environment
 */
SENTRY_API void sentry_options_set_environment(sentry_options_t *opts,
                                               const char *environment);

/*
 * gets the environment
 */
SENTRY_API const char *sentry_options_get_environment(
    const sentry_options_t *opts);

/*
 * sets the dist
 */
SENTRY_API void sentry_options_set_dist(sentry_options_t *opts,
                                        const char *dist);

/*
 * gets the dist
 */
SENTRY_API const char *sentry_options_get_dist(const sentry_options_t *opts);

/*
 * enables or disables debug printing mode
 */
SENTRY_API void sentry_options_set_debug(sentry_options_t *opts, int debug);

/*
 * returns the current value of the debug flag.
 */
SENTRY_API int sentry_options_get_debug(const sentry_options_t *opts);

/*
 * adds a new attachment to be sent along
 */
SENTRY_API void sentry_options_add_attachment(sentry_options_t *opts,
                                              const char *name,
                                              const char *path);

/*
 * sets the path to the crashpad handler if the crashpad backend is used
 */
SENTRY_API void sentry_options_set_handler_path(sentry_options_t *opts,
                                                const char *path);

/*
 * sets the path to the sentrypad/crashpad/breakpad database
 */
SENTRY_API void sentry_options_set_database_path(sentry_options_t *opts,
                                                 const char *path);

#ifdef _WIN32
/* wide char version of `sentry_options_add_attachment` */
SENTRY_API void sentry_options_add_attachmentw(sentry_options_t *opts,
                                               const char *name,
                                               const wchar_t *path);
/* wide char version of `sentry_options_set_handler_path` */
SENTRY_API void sentry_options_set_handler_pathw(sentry_options_t *opts,
                                                 const wchar_t *path);
/* wide char version of `sentry_options_set_database_path` */
SENTRY_API void sentry_options_set_database_pathw(sentry_options_t *opts,
                                                  const wchar_t *path);
#endif

/* Unified API */

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
 *
 * This takes ownership of the options.  After the options have been set they
 * cannot be modified any more.
 */
SENTRY_API int sentry_init(sentry_options_t *options);

/*
 * Returns the bound options.
 */
SENTRY_API const sentry_options_t *sentry_get_options(void);

/*
 * Adds the breadcrumb to be sent in case of an event.
 */
SENTRY_API void sentry_add_breadcrumb(const sentry_breadcrumb_t *breadcrumb);
/*
 * Sets the specified user.
 */
SENTRY_API void sentry_set_user(const sentry_user_t *user);
/*
 * Removes a user.
 */
SENTRY_API void sentry_remove_user();
/*
 * Sets a tag.
 */
SENTRY_API void sentry_set_tag(const char *key, const char *value);
/*
 * Removes the tag with the specified key.
 */
SENTRY_API void sentry_remove_tag(const char *key);
/*
 * Sets extra information.
 */
SENTRY_API void sentry_set_extra(const char *key, const char *value);
/*
 * Removes the extra with the specified key.
 */
SENTRY_API void sentry_remove_extra(const char *key);
/*
 * Sets the event fingerprint.
 */
SENTRY_API void sentry_set_fingerprint(const char *fingerprint, ...);
/*
 * Removes the fingerprint.
 */
SENTRY_API void sentry_remove_fingerprint();
/*
 * Sets the transaction.
 */
SENTRY_API void sentry_set_transaction(const char *transaction);
/*
 * Removes the transaction.
 */
SENTRY_API void sentry_remove_transaction();
/*
 * Sets the event level.
 */
SENTRY_API void sentry_set_level(enum sentry_level_t level);

#ifdef __cplusplus
}
#endif
#endif
