#ifndef SENTRY_HINT_H_INCLUDED
#define SENTRY_HINT_H_INCLUDED

#include "sentry_boot.h"

/**
 * A sentry Hint used to pass additional data along with an event
 * or feedback when it's being captured.
 */
struct sentry_hint_s {
    sentry_value_t attachments;
    bool modified;
};

/**
 * Initializes a hint with a snapshot of the global scope's attachments.
 */
void sentry__hint_init(sentry_hint_t *hint);

/**
 * Releases resources owned by a hint.
 */
void sentry__hint_deinit(sentry_hint_t *hint);

/**
 * Returns whether the hint's attachments differ from its baseline.
 */
bool sentry__hint_is_modified(const sentry_hint_t *hint);

/**
 * Replaces a hint's attachment baseline, taking ownership of `attachments`.
 */
void sentry__hint_set_attachments(
    sentry_hint_t *hint, sentry_value_t attachments);

/**
 * Frees a hint (internal use only).
 */
void sentry__hint_free(sentry_hint_t *hint);

#endif
