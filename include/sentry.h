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

#include <inttypes.h>
#include <stddef.h>

#ifdef _WIN32
#include <Rpc.h>
#include <wchar.h>
#else
#include <uuid/uuid.h>
#endif

#define SENTRY_SDK_VERSION "0.0.3"

/*
 * type type of a sentry value.
 */
typedef enum {
    SENTRY_VALUE_TYPE_NULL,
    SENTRY_VALUE_TYPE_BOOL,
    SENTRY_VALUE_TYPE_INT32,
    SENTRY_VALUE_TYPE_DOUBLE,
    SENTRY_VALUE_TYPE_STRING,
    SENTRY_VALUE_TYPE_LIST,
    SENTRY_VALUE_TYPE_OBJECT,
} sentry_value_type_t;

/*
 * Represents a sentry protocol value.
 *
 * The members of this type should never be accessed.  They are only here
 * so that alignment for the type can be properly determined.
 */
union sentry_value_u {
    uint64_t _bits;
    double _val;
};

typedef union sentry_value_u sentry_value_t;

/* sentry value functions */
SENTRY_API sentry_value_t sentry_value_new_null(void);
SENTRY_API sentry_value_t sentry_value_new_int32(int32_t value);
SENTRY_API sentry_value_t sentry_value_new_double(double value);
SENTRY_API sentry_value_t sentry_value_new_bool(int value);
SENTRY_API sentry_value_t sentry_value_new_string(const char *value);
SENTRY_API sentry_value_t sentry_value_new_list(void);
SENTRY_API sentry_value_t sentry_value_new_object(void);
SENTRY_API void sentry_value_incref(sentry_value_t value);
SENTRY_API void sentry_value_decref(sentry_value_t value);
SENTRY_API sentry_value_type_t sentry_value_get_type(sentry_value_t value);
SENTRY_API int sentry_value_set_key(sentry_value_t value,
                                    const char *k,
                                    sentry_value_t v);
SENTRY_API int sentry_value_remove_key(sentry_value_t value, const char *k);
SENTRY_API int sentry_value_append(sentry_value_t value, sentry_value_t v);
SENTRY_API sentry_value_t sentry_value_get_by_key(sentry_value_t value,
                                                  const char *k);
SENTRY_API sentry_value_t sentry_value_get_by_index(sentry_value_t value,
                                                    size_t index);
SENTRY_API size_t sentry_value_get_length(sentry_value_t value);
SENTRY_API int32_t sentry_value_as_int32(sentry_value_t value);
SENTRY_API double sentry_value_as_double(sentry_value_t value);
SENTRY_API const char *sentry_value_as_string(sentry_value_t value);
SENTRY_API int sentry_value_is_true(sentry_value_t value);
SENTRY_API int sentry_value_is_null(sentry_value_t value);
SENTRY_API sentry_value_t sentry_event_value_new(void);
SENTRY_API sentry_value_t sentry_breadcrumb_value_new(const char *type,
                                                      const char *message);
SENTRY_API void sentry_event_value_add_stacktrace(sentry_value_t event,
                                                  void **ips);

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

/*
 * A UUID
 */
typedef struct sentry_uuid_s {
#ifdef _WIN32
    GUID native_uuid;
#else
    uuid_t native_uuid;
#endif
} sentry_uuid_t;

SENTRY_API sentry_uuid_t sentry_uuid_nil(void);
SENTRY_API sentry_uuid_t sentry_uuid_new_v4(void);
SENTRY_API sentry_uuid_t sentry_uuid_from_string(const char *str);
SENTRY_API int sentry_uuid_is_nil(const sentry_uuid_t *uuid);
SENTRY_API void sentry_uuid_as_bytes(const sentry_uuid_t *uuid, char bytes[16]);
SENTRY_API void sentry_uuid_as_string(const sentry_uuid_t *uuid, char str[37]);

struct sentry_options_s;
typedef struct sentry_options_s sentry_options_t;

/* type of the callback for transports */
typedef void (*sentry_transport_function_t)(sentry_value_t event);

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

/* Unified API */

/*
 * Initializes the Sentry SDK with the specified options.
 *
 * This takes ownership of the options.  After the options have been set they
 * cannot be modified any more.
 */
SENTRY_API int sentry_init(sentry_options_t *options);

/*
 * Returns the client options.
 */
SENTRY_API const sentry_options_t *sentry_get_options(void);

/*
 * Sends a sentry event.
 */
SENTRY_API sentry_uuid_t sentry_capture_event(sentry_value_t event);

/*
 * Adds the breadcrumb to be sent in case of an event.
 */
SENTRY_API void sentry_add_breadcrumb(sentry_value_t breadcrumb);
/*
 * Sets the specified user.
 */
SENTRY_API void sentry_set_user(sentry_value_t user);
/*
 * Removes a user.
 */
SENTRY_API void sentry_remove_user(void);
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
SENTRY_API void sentry_set_extra(const char *key, sentry_value_t value);
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
SENTRY_API void sentry_remove_fingerprint(void);
/*
 * Sets the transaction.
 */
SENTRY_API void sentry_set_transaction(const char *transaction);
/*
 * Removes the transaction.
 */
SENTRY_API void sentry_remove_transaction(void);
/*
 * Sets the event level.
 */
SENTRY_API void sentry_set_level(enum sentry_level_t level);

#ifdef __cplusplus
}
#endif
#endif
