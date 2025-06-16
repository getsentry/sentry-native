#ifndef SENTRY_ATTACHMENT_H_INCLUDED
#define SENTRY_ATTACHMENT_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"

/**
 * The attachment_type.
 */
typedef enum {
    ATTACHMENT,
    MINIDUMP,
    VIEW_HIERARCHY,
} sentry_attachment_type_t;

/**
 * This is a linked list of all the attachments registered via
 * `sentry_options_add_attachment`.
 */
struct sentry_attachment_s {
    sentry_path_t *path;
    sentry_attachment_type_t type;
    char *content_type;
    sentry_attachment_t *next;
};

/**
 * Frees the linked list of `attachments`.
 */
void sentry__attachments_free(sentry_attachment_t *attachments);

/**
 * Adds an attachment to the attachments list at `attachments_ptr`.
 */
sentry_attachment_t *sentry__attachments_add(
    sentry_attachment_t **attachments_ptr, sentry_path_t *path,
    sentry_attachment_type_t attachment_type, const char *content_type);

/**
 * Removes an attachment from the attachments list at `attachments_ptr`.
 */
void sentry__attachments_remove(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachment);

/**
 * Extends the linked list of attachments at `attachments_ptr` with all
 * attachments in `attachments`.
 */
void sentry__attachments_extend(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachments);

#endif
