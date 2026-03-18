#ifndef SENTRY_BACKEND_H_INCLUDED
#define SENTRY_BACKEND_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_scope.h"

/**
 * This represents the crash handling backend.
 * It consists of a few hooks that integrate into the sentry lifecycle and which
 * can ensure that any captured crash contains the sentry scope and other
 * information.
 */
struct sentry_backend_s {
    int (*startup_func)(sentry_backend_t *, const sentry_options_t *options);
    void (*shutdown_func)(sentry_backend_t *);
    void (*free_func)(sentry_backend_t *);
    void (*except_func)(sentry_backend_t *, const struct sentry_ucontext_s *);
    void (*flush_scope_func)(
        sentry_backend_t *, const sentry_options_t *options);
    // NOTE: The breadcrumb is not moved into the hook and does not need to be
    // `decref`-d internally.
    void (*add_breadcrumb_func)(sentry_backend_t *, sentry_value_t breadcrumb,
        const sentry_options_t *options);
    void (*user_consent_changed_func)(sentry_backend_t *);
    uint64_t (*get_last_crash_func)(sentry_backend_t *);
    void (*prune_database_func)(sentry_backend_t *);
    void (*add_attachment_func)(sentry_backend_t *, sentry_attachment_t *);
    void (*remove_attachment_func)(sentry_backend_t *, sentry_attachment_t *);
    void *data;
    // Whether this backend still runs after shutdown_func was called.
    bool can_capture_after_shutdown;
};

/**
 * Backend-specific pre-initialization that can be called before sentry_init().
 *
 * Currently only used from the NDK's SentryNdkPreloadProvider via JNI to
 * preload the inproc backend on Android. This installs and chains the Native
 * SDK's signal handlers before the Mono runtime, allowing Mono to correctly
 * handle managed exceptions while chaining native crashes to the Native SDK.
 *
 * If a crash occurs before sentry_init() is called, the handler will fall
 * through to the previously installed handler.
 */
SENTRY_API void sentry__backend_preload(void);

/**
 * This will free a previously allocated backend.
 */
void sentry__backend_free(sentry_backend_t *backend);

/**
 * Create a new backend, depending on build-time configuration.
 */
sentry_backend_t *sentry__backend_new(void);

#endif
