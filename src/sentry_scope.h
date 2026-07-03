#ifndef SENTRY_SCOPE_H_INCLUDED
#define SENTRY_SCOPE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_attachment.h"
#include "sentry_ringbuffer.h"
#include "sentry_session.h"
#include "sentry_value.h"

/**
 * Scope observer — one callback per scope property.
 *
 * Implementors set the function pointers they care about. NULL pointers are
 * skipped. Callbacks are invoked while the scope lock is held.
 *
 * Ownership: the scope takes ownership of the observer pointer on
 * registration; the caller must not free it after that point.
 */
typedef struct sentry_scope_observer_s {
    void *data;

    void (*set_release)(void *data, const char *release, size_t release_len);
    void (*set_environment)(
        void *data, const char *environment, size_t environment_len);
    void (*set_transaction)(
        void *data, const char *transaction, size_t transaction_len);
    void (*set_fingerprint)(void *data, sentry_value_t fingerprint);
    void (*set_level)(void *data, sentry_level_t level);
    void (*set_user)(void *data, sentry_value_t user);

    void (*add_breadcrumb)(void *data, sentry_value_t breadcrumb);

    void (*set_tag)(void *data, const char *key, size_t key_len,
        const char *value, size_t value_len);
    void (*remove_tag)(void *data, const char *key, size_t key_len);

    void (*set_extra)(
        void *data, const char *key, size_t key_len, sentry_value_t value);
    void (*remove_extra)(void *data, const char *key, size_t key_len);

    void (*set_context)(
        void *data, const char *key, size_t key_len, sentry_value_t value);
    void (*remove_context)(void *data, const char *key, size_t key_len);

    void (*add_attachment)(void *data, sentry_attachment_t *attachment);
    void (*remove_attachment)(void *data, sentry_attachment_t *attachment);
} sentry_scope_observer_t;

/**
 * This represents the current scope.
 */
struct sentry_scope_s {
    char *release;
    char *environment;
    char *transaction;
    sentry_value_t fingerprint;
    sentry_value_t user;
    sentry_value_t tags;
    sentry_value_t extra;
    sentry_value_t attributes;
    sentry_value_t contexts;
    sentry_value_t propagation_context;
    sentry_ringbuffer_t *breadcrumbs;
    sentry_value_t dynamic_sampling_context;
    sentry_level_t level;
    sentry_value_t client_sdk;
    sentry_attachment_t *attachments;

    // The span attached to this scope, if any.
    //
    // Conceptually, every transaction is a span, so it should be possible to
    // attach spans or transactions to a scope. But sentry_span_t and
    // sentry_transaction_t are unrelated types in the native SDK, so we need
    // two distinct pointers. At most one of them should ever be non-null.
    // Whenever possible, `transaction` should pull its value from the
    // `name` property nested in transaction_object or span.
    sentry_transaction_t *transaction_object;
    sentry_span_t *span;
    bool trace_managed;

    sentry_scope_observer_t **observers;
    size_t num_observers;
    bool is_notifying;
};

/**
 * When applying a scope to an event object, this specifies all the additional
 * data that should be added to the event.
 */
typedef enum {
    SENTRY_SCOPE_NONE = 0x0,
    // Add all the breadcrumbs from the scope to the event.
    SENTRY_SCOPE_BREADCRUMBS = 0x1,
    // Add the module list to the event.
    SENTRY_SCOPE_MODULES = 0x2,
    // Symbolize all the stacktraces on-device which are found in the event.
    SENTRY_SCOPE_STACKTRACES = 0x4,
    // All of the above.
    SENTRY_SCOPE_ALL = ~0,
} sentry_scope_mode_t;

/**
 * This will acquire a lock on the global scope.
 */
sentry_scope_t *sentry__scope_lock(void);

/**
 * Release the lock on the global scope.
 */
void sentry__scope_unlock(void);

/**
 * This will free all the data attached to the global scope
 */
void sentry__scope_cleanup(void);

/**
 * This will notify any backend of scope changes.
 * This function must be called while holding the scope lock, and it will be
 * unlocked internally.
 */
void sentry__scope_flush_unlock(void);

/**
 * Deallocates a (local) scope.
 */
void sentry__scope_free(sentry_scope_t *scope);

/**
 * This will merge the requested data which is in the given `scope` to the given
 * `event`.
 * See `sentry_scope_mode_t` for the different types of data that can be
 * attached.
 */
void sentry__scope_apply_to_event(const sentry_scope_t *scope,
    const sentry_options_t *options, sentry_value_t event,
    sentry_scope_mode_t mode);

void sentry__scope_set_fingerprint_va(
    sentry_scope_t *scope, const char *fingerprint, va_list va);
void sentry__scope_set_fingerprint_nva(sentry_scope_t *scope,
    const char *fingerprint, size_t fingerprint_len, va_list va);

/**
 * Internal scope-based attribute functions.
 * For now, these are only used by the non-scope API functions that operate
 * on the global scope.
 * Once we have attributes for events or scope-based logs/metrics/spans APIs
 * these can become part of the public API too.
 */
void sentry__scope_set_attribute(
    sentry_scope_t *scope, const char *key, sentry_value_t attribute);
void sentry__scope_set_attribute_n(sentry_scope_t *scope, const char *key,
    size_t key_len, sentry_value_t attribute);
void sentry__scope_remove_attribute(sentry_scope_t *scope, const char *key);
void sentry__scope_remove_attribute_n(
    sentry_scope_t *scope, const char *key, size_t key_len);

sentry_attachment_t *sentry__scope_add_attachment(
    sentry_scope_t *scope, sentry_attachment_t *attachment);

/**
 * These are convenience macros to automatically lock/unlock the global scope
 * inside a code block.
 */
#define SENTRY_WITH_SCOPE(Scope)                                               \
    for (const sentry_scope_t *Scope = sentry__scope_lock(); Scope;            \
        sentry__scope_unlock(), Scope = NULL)
#define SENTRY_WITH_SCOPE_MUT(Scope)                                           \
    for (sentry_scope_t *Scope = sentry__scope_lock(); Scope;                  \
        sentry__scope_flush_unlock(), Scope = NULL)
#define SENTRY_WITH_SCOPE_MUT_NO_FLUSH(Scope)                                  \
    for (sentry_scope_t *Scope = sentry__scope_lock(); Scope;                  \
        sentry__scope_unlock(), Scope = NULL)

/**
 * Allocate and zero-initialize a scope observer.
 *
 * Returns NULL on allocation failure. The caller sets whichever callback
 * function pointers and the `data` pointer they need, then registers the
 * observer with `sentry__scope_add_observer`, which takes ownership.
 */
sentry_scope_observer_t *sentry__scope_observer_new(void);

/**
 * Register a scope observer.
 *
 * Takes ownership of `observer`; the caller must not use or free it after
 * this call. Must be called while holding the scope lock (i.e., inside
 * SENTRY_WITH_SCOPE_MUT). Registration order is respected — observers are
 * notified in registration order.
 */
void sentry__scope_add_observer(
    sentry_scope_t *scope, sentry_scope_observer_t *observer);

/** Re-entrancy guard: set while notifying observers. */
#define SENTRY_SCOPE_NOTIFY(scope, callback, ...)                              \
    do {                                                                       \
        if ((scope)->is_notifying)                                             \
            break;                                                             \
        (scope)->is_notifying = true;                                          \
        for (size_t _i = 0; _i < (scope)->num_observers; _i++) {               \
            sentry_scope_observer_t *_observer = (scope)->observers[_i];       \
            if (_observer->callback) {                                         \
                _observer->callback(_observer->data, __VA_ARGS__);             \
            }                                                                  \
        }                                                                      \
        (scope)->is_notifying = false;                                         \
    } while (0)

/**
 * Rebuilds the scope's dynamic sampling context (DSC) from the SDK options
 * and the current propagation context. The previous DSC is discarded.
 */
void sentry__scope_update_dsc(
    sentry_scope_t *scope, const sentry_options_t *options);

/**
 * Replaces the scope's dynamic sampling context (DSC) with a verbatim copy
 * of the incoming object. Used when continuing an upstream trace: per the
 * trace-propagation spec, the receiving SDK MUST treat the incoming DSC as
 * frozen and propagate its values "as is".
 */
void sentry__scope_freeze_dsc(sentry_scope_t *scope, sentry_value_t incoming);

/**
 * Adds scoped attributes to the telemetry attributes object.
 */
void sentry__scope_apply_attributes(const sentry_scope_t *scope,
    sentry_value_t telemetry, sentry_value_t attributes);

#endif

// this is only used in unit tests
#ifdef SENTRY_UNITTEST
sentry_value_t sentry__scope_get_span_or_transaction(void);
#endif
