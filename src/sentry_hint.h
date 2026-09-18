#ifndef SENTRY_HINT_H_INCLUDED
#define SENTRY_HINT_H_INCLUDED

#include "sentry_boot.h"

/**
 * A sentry Hint used to pass additional data along with an event
 * or feedback when it's being captured.
 */
struct sentry_hint_s {
    sentry_value_t attachments;
};

#define SENTRY__HINT_INIT(Hint)                                                \
    ((void)((Hint).attachments = sentry_value_new_null()))
#define SENTRY__HINT_DEINIT(Hint)                                              \
    ((void)sentry_value_decref((Hint).attachments))

/**
 * Replaces a hint's attachments, taking ownership of `attachments`.
 */
void sentry__hint_set_attachments(
    sentry_hint_t *hint, sentry_value_t attachments);

/**
 * Frees a hint (internal use only).
 */
void sentry__hint_free(sentry_hint_t *hint);

#endif
