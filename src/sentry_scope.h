#ifndef SENTRY_SCOPE_H_INCLUDED
#define SENTRY_SCOPE_H_INCLUDED

#include "sentry_boot.h"

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
} sentry_scope_t;

sentry_scope_t *sentry__scope_lock(void);
void sentry__scope_unlock(void);

void sentry__scope_flush(const sentry_scope_t *scope);
void sentry__scope_apply_to_event(
    const sentry_scope_t *scope, sentry_value_t event);

#define SENTRY_WITH_SCOPE(Scope)                                               \
    for (const sentry_scope_t *Scope = sentry__scope_lock(); Scope;            \
         sentry__scope_unlock(), Scope = NULL)
#define SENTRY_WITH_SCOPE_MUT(Scope)                                           \
    for (sentry_scope_t *Scope = sentry__scope_lock(); Scope;                  \
         sentry__scope_unlock(), sentry__scope_flush(scope), Scope = NULL)

#endif