/**
 * sentry-native
 *
 * sentry-native is a C client to send events to native from
 * C and C++ applications.  It can work together with breakpad/crashpad
 * but also send events on its own.
 */
#ifndef SENTRY_H_INCLUDED
#define SENTRY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* SDK Version */
#define SENTRY_SDK_NAME "sentry.native"
#define SENTRY_SDK_VERSION "0.2.3"
#define SENTRY_SDK_USER_AGENT (SENTRY_SDK_NAME "/" SENTRY_SDK_VERSION)

/* common platform detection */
#ifdef _WIN32
#    define SENTRY_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#    define SENTRY_PLATFORM_MACOS
#    define SENTRY_PLATFORM_DARWIN
#    define SENTRY_PLATFORM_UNIX
#elif defined(__ANDROID__)
#    define SENTRY_PLATFORM_ANDROID
#    define SENTRY_PLATFORM_LINUX
#    define SENTRY_PLATFORM_UNIX
#elif defined(__linux) || defined(__linux__)
#    define SENTRY_PLATFORM_LINUX
#    define SENTRY_PLATFORM_UNIX
#else
#    error unsupported platform
#endif

/* marks a function as part of the sentry API */
#ifndef SENTRY_API
#    ifdef _WIN32
#        if defined(SENTRY_BUILD_SHARED) /* build dll */
#            define SENTRY_API __declspec(dllexport)
#        elif !defined(SENTRY_BUILD_STATIC) /* use dll */
#            define SENTRY_API __declspec(dllimport)
#        else /* static library */
#            define SENTRY_API
#        endif
#    else
#        if __GNUC__ >= 4
#            define SENTRY_API __attribute__((visibility("default")))
#        else
#            define SENTRY_API
#        endif
#    endif
#endif

/* marks a function as experimental api */
#ifndef SENTRY_EXPERIMENTAL_API
#    define SENTRY_EXPERIMENTAL_API SENTRY_API
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

/* context type dependencies */
#ifdef _WIN32
#    include <wtypes.h>
#else
#    include <signal.h>
#endif

/**
 * The library internally uses the system malloc and free functions to manage
 * memory.  It does not use realloc.  The reason for this is that on unix
 * platforms we fall back to a simplistic page allocator once we have
 * encountered a SIGSEGV or other terminating signal as malloc is no longer
 * safe to use.  Since we cannot portably reallocate allocations made on the
 * pre-existing allocator we're instead not using realloc.
 *
 * Note also that after SIGSEGV sentry_free() becomes a noop.
 */

/**
 * Allocates memory with the underlying allocator.
 */
SENTRY_API void *sentry_malloc(size_t size);

/**
 * Releases memory allocated from the underlying allocator.
 */
SENTRY_API void sentry_free(void *ptr);

/**
 * Legacy function.  Alias for `sentry_free`.
 */
#define sentry_string_free sentry_free

/* -- Protocol Value API -- */

/**
 * Type of a sentry value.
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

/**
 * Represents a sentry protocol value.
 *
 * The members of this type should never be accessed.  They are only here
 * so that alignment for the type can be properly determined.
 *
 * Values must be released with `sentry_value_decref`.  This lowers the
 * internal refcount by one.  If the refcount hits zero it's freed.  Some
 * values like primitives have no refcount (like null) so operations on
 * those are no-ops.
 *
 * In addition values can be frozen.  Some values like primitives are always
 * frozen but lists and dicts are not and can be frozen on demand.  This
 * automatically happens for some shared values in the event payload like
 * the module list.
 */
union sentry_value_u {
    uint64_t _bits;
    double _double;
};
typedef union sentry_value_u sentry_value_t;

/**
 * Increments the reference count on the value.
 */
SENTRY_API void sentry_value_incref(sentry_value_t value);

/**
 * Decrements the reference count on the value.
 */
SENTRY_API void sentry_value_decref(sentry_value_t value);

/**
 * Returns the refcount of a value.
 */
SENTRY_API size_t sentry_value_refcount(sentry_value_t value);

/**
 * Freezes a value.
 */
SENTRY_API void sentry_value_freeze(sentry_value_t value);

/**
 * Checks if a value is frozen.
 */
SENTRY_API int sentry_value_is_frozen(sentry_value_t value);

/**
 * Creates a null value.
 */
SENTRY_API sentry_value_t sentry_value_new_null(void);

/**
 * Creates a new 32-bit signed integer value.
 */
SENTRY_API sentry_value_t sentry_value_new_int32(int32_t value);

/**
 * Creates a new double value.
 */
SENTRY_API sentry_value_t sentry_value_new_double(double value);

/**
 * Creates a new boolen value.
 */
SENTRY_API sentry_value_t sentry_value_new_bool(int value);

/**
 * Creates a new null terminated string.
 */
SENTRY_API sentry_value_t sentry_value_new_string(const char *value);

/**
 * Creates a new list value.
 */
SENTRY_API sentry_value_t sentry_value_new_list(void);

/**
 * Creates a new object.
 */
SENTRY_API sentry_value_t sentry_value_new_object(void);

/**
 * Returns the type of the value passed.
 */
SENTRY_API sentry_value_type_t sentry_value_get_type(sentry_value_t value);

/**
 * Sets a key to a value in the map.
 *
 * This moves the ownership of the value into the map.  The caller does not
 * have to call `sentry_value_decref` on it.
 */
SENTRY_API int sentry_value_set_by_key(
    sentry_value_t value, const char *k, sentry_value_t v);

/**
 * This removes a value from the map by key.
 */
SENTRY_API int sentry_value_remove_by_key(sentry_value_t value, const char *k);

/**
 * Appends a value to a list.
 *
 * This moves the ownership of the value into the list.  The caller does not
 * have to call `sentry_value_decref` on it.
 */
SENTRY_API int sentry_value_append(sentry_value_t value, sentry_value_t v);

/**
 * Inserts a value into the list at a certain position.
 *
 * This moves the ownership of the value into the list.  The caller does not
 * have to call `sentry_value_decref` on it.
 *
 * If the list is shorter than the given index it's automatically extended
 * and filled with `null` values.
 */
SENTRY_API int sentry_value_set_by_index(
    sentry_value_t value, size_t index, sentry_value_t v);

/**
 * This removes a value from the list by index.
 */
SENTRY_API int sentry_value_remove_by_index(sentry_value_t value, size_t index);

/**
 * Looks up a value in a map by key.  If missing a null value is returned.
 * The returned value is borrowed.
 */
SENTRY_API sentry_value_t sentry_value_get_by_key(
    sentry_value_t value, const char *k);

/**
 * Looks up a value in a map by key.  If missing a null value is returned.
 * The returned value is owned.
 *
 * If the caller no longer needs the value it must be released with
 * `sentry_value_decref`.
 */
SENTRY_API sentry_value_t sentry_value_get_by_key_owned(
    sentry_value_t value, const char *k);

/**
 * Looks up a value in a list by index.  If missing a null value is returned.
 * The returned value is borrowed.
 */
SENTRY_API sentry_value_t sentry_value_get_by_index(
    sentry_value_t value, size_t index);

/**
 * Looks up a value in a list by index.  If missing a null value is
 * returned. The returned value is owned.
 *
 * If the caller no longer needs the value it must be released with
 * `sentry_value_decref`.
 */
SENTRY_API sentry_value_t sentry_value_get_by_index_owned(
    sentry_value_t value, size_t index);

/**
 * Returns the length of the given map or list.
 *
 * If an item is not a list or map the return value is 0.
 */
SENTRY_API size_t sentry_value_get_length(sentry_value_t value);

/**
 * Converts a value into a 32bit signed integer.
 */
SENTRY_API int32_t sentry_value_as_int32(sentry_value_t value);

/**
 * Converts a value into a double value.
 */
SENTRY_API double sentry_value_as_double(sentry_value_t value);

/**
 * Returns the value as c string.
 */
SENTRY_API const char *sentry_value_as_string(sentry_value_t value);

/**
 * Returns `true` if the value is boolean true.
 */
SENTRY_API int sentry_value_is_true(sentry_value_t value);

/**
 * Returns `true` if the value is null.
 */
SENTRY_API int sentry_value_is_null(sentry_value_t value);

/**
 * Serialize a sentry value to JSON.
 *
 * The string is freshly allocated and must be freed with
 * `sentry_string_free`.
 */
SENTRY_API char *sentry_value_to_json(sentry_value_t value);

/**
 * Sentry levels for events and breadcrumbs.
 */
typedef enum sentry_level_e {
    SENTRY_LEVEL_DEBUG = -1,
    SENTRY_LEVEL_INFO = 0,
    SENTRY_LEVEL_WARNING = 1,
    SENTRY_LEVEL_ERROR = 2,
    SENTRY_LEVEL_FATAL = 3,
} sentry_level_t;

/**
 * Creates a new empty event value.
 */
SENTRY_API sentry_value_t sentry_value_new_event(void);

/**
 * Creates a new message event value.
 *
 * `logger` can be NULL to omit the logger value.
 */
SENTRY_API sentry_value_t sentry_value_new_message_event(
    sentry_level_t level, const char *logger, const char *text);

/**
 * Creates a new breadcrumb with a specific type and message.
 *
 * Either parameter can be NULL in which case no such attributes is created.
 */
SENTRY_API sentry_value_t sentry_value_new_breadcrumb(
    const char *type, const char *message);

/* -- Experimental APIs -- */

/**
 * Serialize a sentry value to msgpack.
 *
 * The string is freshly allocated and must be freed with
 * `sentry_string_free`.  Since msgpack is not zero terminated
 * the size is written to the `size_out` parameter.
 */
SENTRY_EXPERIMENTAL_API char *sentry_value_to_msgpack(
    sentry_value_t value, size_t *size_out);

/**
 * Adds a stacktrace to an event.
 *
 * If `ips` is NULL the current stacktrace is captured, otherwise `len`
 * stacktrace instruction pointers are attached to the event.
 */
SENTRY_EXPERIMENTAL_API void sentry_event_value_add_stacktrace(
    sentry_value_t event, void **ips, size_t len);

/**
 * This represents the OS dependent user context in the case of a crash, and can
 * be used to manually capture a crash.
 */
typedef struct sentry_ucontext_s {
#ifdef _WIN32
    EXCEPTION_POINTERS exception_ptrs;
#else
    int signum;
    siginfo_t *siginfo;
    ucontext_t *user_context;
#endif
} sentry_ucontext_t;

/**
 * Unwinds the stack from the given address.
 *
 * If the address is given in `addr` the stack is unwound form there.
 * Otherwise (NULL is passed) the current instruction pointer is used as
 * start address. The stacktrace is written to `stacktrace_out` with upt o
 * `max_len` frames being written.  The actual number of unwound stackframes
 * is returned.
 */
SENTRY_EXPERIMENTAL_API size_t sentry_unwind_stack(
    void *addr, void **stacktrace_out, size_t max_len);

/**
 * Unwinds the stack from the given context.
 *
 * The stacktrace is written to `stacktrace_out` with upt o `max_len` frames
 * being written.  The actual number of unwound stackframes is returned.
 */
SENTRY_EXPERIMENTAL_API size_t sentry_unwind_stack_from_ucontext(
    const sentry_ucontext_t *uctx, void **stacktrace_out, size_t max_len);

/**
 * A UUID
 */
typedef struct sentry_uuid_s {
    char bytes[16];
} sentry_uuid_t;

/**
 * Creates the nil uuid.
 */
SENTRY_API sentry_uuid_t sentry_uuid_nil(void);

/**
 * Creates a new uuid4.
 */
SENTRY_API sentry_uuid_t sentry_uuid_new_v4(void);

/**
 * Parses a uuid from a string.
 */
SENTRY_API sentry_uuid_t sentry_uuid_from_string(const char *str);

/**
 * Creates a uuid from bytes.
 */
SENTRY_API sentry_uuid_t sentry_uuid_from_bytes(const char bytes[16]);

/**
 * Checks if the uuid is nil.
 */
SENTRY_API int sentry_uuid_is_nil(const sentry_uuid_t *uuid);

/**
 * Returns the bytes of the uuid.
 */
SENTRY_API void sentry_uuid_as_bytes(const sentry_uuid_t *uuid, char bytes[16]);

/**
 * Formats the uuid into a string buffer.
 */
SENTRY_API void sentry_uuid_as_string(const sentry_uuid_t *uuid, char str[37]);

struct sentry_options_s;
typedef struct sentry_options_s sentry_options_t;

struct sentry_envelope_s;
typedef struct sentry_envelope_s sentry_envelope_t;

/**
 * Frees an envelope.
 */
SENTRY_API void sentry_envelope_free(sentry_envelope_t *envelope);

/**
 * Given an envelope returns the embedded event if there is one.
 *
 * This returns a borrowed value to the event in the envelope.
 */
SENTRY_API sentry_value_t sentry_envelope_get_event(
    const sentry_envelope_t *envelope);

/**
 * Serializes the envelope.
 *
 * The return value needs to be freed with sentry_string_free().
 */
SENTRY_API char *sentry_envelope_serialize(
    const sentry_envelope_t *envelope, size_t *size_out);

/**
 * serializes the envelope into a file.
 *
 * returns 0 on success.
 */
SENTRY_API int sentry_envelope_write_to_file(
    const sentry_envelope_t *envelope, const char *path);

/**
 * Type of the callback for transports.
 */
typedef void (*sentry_transport_function_t)(
    const sentry_envelope_t *envelope, void *data);

/**
 * This represents an interface for user-defined transports.
 *
 * This type is *deprecated*, and will be replaced by an opaque pointer type and
 * builder methods in a future version.
 */
struct sentry_transport_s;
typedef struct sentry_transport_s {
    void (*send_envelope_func)(
        struct sentry_transport_s *, sentry_envelope_t *envelope);
    void (*startup_func)(struct sentry_transport_s *);
    void (*shutdown_func)(struct sentry_transport_s *);
    void (*free_func)(struct sentry_transport_s *);
    void *data;
} sentry_transport_t;

/**
 * Creates a new function transport.
 */
SENTRY_API sentry_transport_t *sentry_new_function_transport(
    void (*func)(sentry_envelope_t *envelope, void *data), void *data);

/**
 * Generic way to free a transport.
 */
SENTRY_API void sentry_transport_free(sentry_transport_t *transport);

/**
 * This represents an opaque backend.
 *
 * This declaration is *deprecated* and will be removed in a future version.
 */
struct sentry_backend_s;
typedef struct sentry_backend_s sentry_backend_t;

/**
 * Generic way to free a backend.
 *
 * This function is *deprecated* and will be removed in a future version.
 */
SENTRY_API void sentry_backend_free(sentry_backend_t *backend);

/**
 * Type of the callback for modifying events.
 */
typedef sentry_value_t (*sentry_event_function_t)(
    sentry_value_t event, void *hint, void *closure);

/* -- Options APIs -- */

/**
 * The state of user consent.
 */
typedef enum {
    SENTRY_USER_CONSENT_UNKNOWN = -1,
    SENTRY_USER_CONSENT_GIVEN = 1,
    SENTRY_USER_CONSENT_REVOKED = 0,
} sentry_user_consent_t;

/**
 * Creates a new options struct.
 * Can be freed with `sentry_options_free`.
 */
SENTRY_API sentry_options_t *sentry_options_new(void);

/**
 * Deallocates previously allocated sentry options.
 */
SENTRY_API void sentry_options_free(sentry_options_t *opts);

/**
 * Sets a transport.
 */
SENTRY_API void sentry_options_set_transport(
    sentry_options_t *opts, sentry_transport_t *transport);

/**
 * Sets the before send callback.
 */
SENTRY_API void sentry_options_set_before_send(
    sentry_options_t *opts, sentry_event_function_t func, void *data);

/**
 * Sets the DSN.
 */
SENTRY_API void sentry_options_set_dsn(sentry_options_t *opts, const char *dsn);

/**
 * Gets the DSN.
 */
SENTRY_API const char *sentry_options_get_dsn(const sentry_options_t *opts);

/**
 * Sets the sample rate, which should be a double between `0.0` and `1.0`.
 * Sentry will randomly discard any event that is captured using
 * `sentry_capture_event` when a sample rate < 1 is set.
 */
SENTRY_API void sentry_options_set_sample_rate(
    sentry_options_t *opts, double sample_rate);

/**
 * Gets the sample rate.
 */
SENTRY_API double sentry_options_get_sample_rate(const sentry_options_t *opts);

/**
 * Sets the release.
 */
SENTRY_API void sentry_options_set_release(
    sentry_options_t *opts, const char *release);

/**
 * Gets the release.
 */
SENTRY_API const char *sentry_options_get_release(const sentry_options_t *opts);

/**
 * Sets the environment.
 */
SENTRY_API void sentry_options_set_environment(
    sentry_options_t *opts, const char *environment);

/**
 * Gets the environment.
 */
SENTRY_API const char *sentry_options_get_environment(
    const sentry_options_t *opts);

/**
 * Sets the dist.
 */
SENTRY_API void sentry_options_set_dist(
    sentry_options_t *opts, const char *dist);

/**
 * Gets the dist.
 */
SENTRY_API const char *sentry_options_get_dist(const sentry_options_t *opts);

/**
 * Configures the http proxy.
 */
SENTRY_API void sentry_options_set_http_proxy(
    sentry_options_t *opts, const char *proxy);

/**
 * Returns the configured http proxy.
 */
SENTRY_API const char *sentry_options_get_http_proxy(
    const sentry_options_t *opts);

/**
 * Configures the path to a file containing ssl certificates for
 * verification.
 */
SENTRY_API void sentry_options_set_ca_certs(
    sentry_options_t *opts, const char *path);

/**
 * Returns the configured path for ca certificates.
 */
SENTRY_API const char *sentry_options_get_ca_certs(
    const sentry_options_t *opts);

/**
 * Enables or disables debug printing mode.
 */
SENTRY_API void sentry_options_set_debug(sentry_options_t *opts, int debug);

/**
 * Returns the current value of the debug flag.
 */
SENTRY_API int sentry_options_get_debug(const sentry_options_t *opts);

/**
 * Enables or disabled user consent requirements for uploads.
 *
 * This disables uploads until the user has given the consent to the SDK.
 * Consent itself is given with `sentry_user_consent_give` and
 * `sentry_user_consent_revoke`.
 */
SENTRY_API void sentry_options_set_require_user_consent(
    sentry_options_t *opts, int val);

/**
 * Returns true if user consent is required.
 */
SENTRY_API int sentry_options_get_require_user_consent(
    const sentry_options_t *opts);

/**
 * Adds a new attachment to be sent along.
 */
SENTRY_API void sentry_options_add_attachment(
    sentry_options_t *opts, const char *name, const char *path);

/**
 * Sets the path to the crashpad handler if the crashpad backend is used.
 *
 * The path defaults to the `crashpad_handler`/`crashpad_handler.exe`
 * executable, depending on platform, which is expected to be present in the
 * same directory as the app executable.
 *
 * It is recommended that library users set an explicit handler path, depending
 * on the directory/executable structure of their app.
 */
SENTRY_API void sentry_options_set_handler_path(
    sentry_options_t *opts, const char *path);

/**
 * Sets the path to the Sentry Database Directory.
 *
 * Sentry will use this path to persist user consent, sessions, and other
 * artifacts in case of a crash. This will also be used by the crashpad backend
 * if it is configured.
 *
 * The path defaults to `.sentry-native` in the current working directory, will
 * be created if it does not exist, and will be resolved to an absolute path
 * inside of `sentry_init`.
 *
 * It is recommended that library users set an explicit absolute path, depending
 * on their own apps directory.
 */
SENTRY_API void sentry_options_set_database_path(
    sentry_options_t *opts, const char *path);

#ifdef SENTRY_PLATFORM_WINDOWS
/**
 * Wide char version of `sentry_options_add_attachment`.
 */
SENTRY_API void sentry_options_add_attachmentw(
    sentry_options_t *opts, const char *name, const wchar_t *path);

/**
 * Wide char version of `sentry_options_set_handler_path`.
 */
SENTRY_API void sentry_options_set_handler_pathw(
    sentry_options_t *opts, const wchar_t *path);

/**
 * Wide char version of `sentry_options_set_database_path`.
 */
SENTRY_API void sentry_options_set_database_pathw(
    sentry_options_t *opts, const wchar_t *path);
#endif

/**
 * Enables forwarding to the system crash reporter. Disabled by default.
 *
 * This setting only has an effect when using Crashpad on macOS. If enabled,
 * Crashpad forwards crashes to the macOS system crash reporter. Depending
 * on the crash, this may impact the crash time. Even if enabled, Crashpad
 * may choose not to forward certain crashes.
 */
SENTRY_API void sentry_options_set_system_crash_reporter_enabled(
    sentry_options_t *opts, int enabled);

/* -- Global APIs -- */

/**
 * Initializes the Sentry SDK with the specified options.
 *
 * This takes ownership of the options.  After the options have been set
 * they cannot be modified any more.
 */
SENTRY_API int sentry_init(sentry_options_t *options);

/**
 * Shuts down the sentry client and forces transports to flush out.
 */
SENTRY_API void sentry_shutdown(void);

/**
 * Clears the internal module cache.
 *
 * For performance reasons, sentry will cache the list of loaded libraries when
 * capturing events. This cache can get out-of-date when loading or unloading
 * libraries at runtime. It is therefore recommended to call
 * `sentry_clear_modulecache` when doing so, to make sure that the next call to
 * `sentry_capture_event` will have an up-to-date module list.
 */
SENTRY_EXPERIMENTAL_API void sentry_clear_modulecache(void);

/**
 * Returns the client options.
 *
 * This might return NULL if sentry is not yet initialized.
 */
SENTRY_API const sentry_options_t *sentry_get_options(void);

/**
 * Gives user consent.
 */
SENTRY_API void sentry_user_consent_give(void);

/**
 * Revokes user consent.
 */
SENTRY_API void sentry_user_consent_revoke(void);

/**
 * Resets the user consent (back to unknown).
 */
SENTRY_API void sentry_user_consent_reset(void);

/**
 * Checks the current state of user consent.
 */
SENTRY_API sentry_user_consent_t sentry_user_consent_get(void);

/**
 * Sends a sentry event.
 */
SENTRY_API sentry_uuid_t sentry_capture_event(sentry_value_t event);

/**
 * Captures an exception to be handled by the backend.
 *
 * This is safe to be called from a crashing thread and may not return.
 */
SENTRY_EXPERIMENTAL_API void sentry_handle_exception(sentry_ucontext_t *uctx);

/**
 * Adds the breadcrumb to be sent in case of an event.
 */
SENTRY_API void sentry_add_breadcrumb(sentry_value_t breadcrumb);

/**
 * Sets the specified user.
 */
SENTRY_API void sentry_set_user(sentry_value_t user);

/**
 * Removes a user.
 */
SENTRY_API void sentry_remove_user(void);

/**
 * Sets a tag.
 */
SENTRY_API void sentry_set_tag(const char *key, const char *value);

/**
 * Removes the tag with the specified key.
 */
SENTRY_API void sentry_remove_tag(const char *key);

/**
 * Sets extra information.
 */
SENTRY_API void sentry_set_extra(const char *key, sentry_value_t value);

/**
 * Removes the extra with the specified key.
 */
SENTRY_API void sentry_remove_extra(const char *key);

/**
 * Sets a context object.
 */
SENTRY_API void sentry_set_context(const char *key, sentry_value_t value);

/**
 * Removes the context object with the specified key.
 */
SENTRY_API void sentry_remove_context(const char *key);

/**
 * Sets the event fingerprint.
 */
SENTRY_API void sentry_set_fingerprint(const char *fingerprint, ...);

/**
 * Removes the fingerprint.
 */
SENTRY_API void sentry_remove_fingerprint(void);

/**
 * Sets the transaction.
 */
SENTRY_API void sentry_set_transaction(const char *transaction);

/**
 * Removes the transaction.
 */
SENTRY_API void sentry_remove_transaction(void);

/**
 * Sets the event level.
 */
SENTRY_API void sentry_set_level(sentry_level_t level);

/**
 * Starts a new session.
 */
SENTRY_EXPERIMENTAL_API void sentry_start_session(void);

/**
 * Ends a session.
 */
SENTRY_EXPERIMENTAL_API void sentry_end_session(void);

#ifdef __cplusplus
}
#endif
#endif
