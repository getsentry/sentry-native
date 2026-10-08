#ifndef SENTRY_INTEGRATION_H_INCLUDED
#define SENTRY_INTEGRATION_H_INCLUDED

#include "sentry_boot.h"

typedef struct sentry_integration_s {
    const char *name;
    void *data;

    void (*register_func)(
        void *data, sentry_scope_t *scope, const sentry_options_t *options);
    void (*unregister_func)(
        void *data, sentry_scope_t *scope, const sentry_options_t *options);
    void (*free_func)(void *data);
    void (*crash_handler_enter_func)(void *data); // must be async-signal-safe
    void (*crash_handler_exit_func)(void *data); // must be async-signal-safe

    /**
     * Returns an installation ID that is stable for this device and
     * application, or NULL if the integration cannot provide one. It backs the
     * default `user.id` and is only consulted when the ID is persisted for the
     * first time, so a stored ID is never replaced.
     *
     * The returned string is owned by the integration, which may reuse its
     * storage on the next call. Like a user ID it can be any string, as long as
     * it is non-empty, at most 128 characters long and contains no newline;
     * anything else is rejected. `public_key` is the DSN's public key, or an
     * empty string when there is no valid DSN.
     */
    const char *(*installation_id_func)(void *data, const char *public_key);
} sentry_integration_t;

#ifdef SENTRY_INTEGRATION_PLATFORM
#    ifdef __cplusplus
extern "C" {
#    endif

sentry_integration_t *sentry_integration_platform_new(void);

#    ifdef __cplusplus
}
#    endif
#endif

#endif
