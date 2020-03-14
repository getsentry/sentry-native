#ifndef SENTRY_SCOPE_H_INCLUDED
#define SENTRY_SCOPE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_sessions.h"
#include "sentry_value.h"

typedef struct sentry_scope_s {
    char *transaction;
    sentry_value_t fingerprint;
    sentry_value_t user;
    sentry_value_t tags;
    sentry_value_t extra;
    sentry_value_t contexts;
    sentry_value_t breadcrumbs;
    sentry_level_t level;
    sentry_value_t client_sdk;
    sentry_session_t *session;
} sentry_scope_t;

typedef enum {
    SENTRY_SCOPE_NONE = 0x0,
    SENTRY_SCOPE_BREADCRUMBS = 0x1,
    SENTRY_SCOPE_MODULES = 0x2,
    // TODO: SENTRY_SCOPE_STACKTRACES = 0x4,
    SENTRY_SCOPE_ALL = ~0,
} sentry_scope_mode_t;

sentry_scope_t *sentry__scope_lock(void);
void sentry__scope_unlock(void);

void sentry__scope_cleanup(void);

void sentry__scope_flush(const sentry_scope_t *scope);
void sentry__scope_apply_to_event(const sentry_scope_t *scope,
    sentry_value_t event, sentry_scope_mode_t mode);

#define SENTRY_WITH_SCOPE(Scope)                                               \
    for (const sentry_scope_t *Scope = sentry__scope_lock(); Scope;            \
         sentry__scope_unlock(), Scope = NULL)
#define SENTRY_WITH_SCOPE_MUT(Scope)                                           \
    for (sentry_scope_t *Scope = sentry__scope_lock(); Scope;                  \
         sentry__scope_unlock(), sentry__scope_flush(scope), Scope = NULL)
#define SENTRY_WITH_SCOPE_MUT_NO_FLUSH(Scope)                                  \
    for (sentry_scope_t *Scope = sentry__scope_lock(); Scope;                  \
         sentry__scope_unlock(), Scope = NULL)

#endif
