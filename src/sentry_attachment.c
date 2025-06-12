#include "sentry_attachment.h"
#include "sentry_alloc.h"
#include "sentry_path.h"

static void
attachment_free(sentry_attachment_t *attachment)
{
    if (!attachment) {
        return;
    }
    sentry__path_free(attachment->path);
    sentry_free(attachment);
}

void
sentry__attachments_free(sentry_attachment_t *attachments)
{
    sentry_attachment_t *next_attachment = attachments;
    while (next_attachment) {
        sentry_attachment_t *attachment = next_attachment;
        next_attachment = attachment->next;

        attachment_free(attachment);
    }
}

sentry_attachment_t *
sentry__attachments_add(sentry_attachment_t **attachments_ptr,
    sentry_path_t *path, sentry_attachment_type_t attachment_type,
    const char *content_type)
{
    if (!path) {
        return NULL;
    }
    sentry_attachment_t *attachment = SENTRY_MAKE(sentry_attachment_t);
    if (!attachment) {
        sentry__path_free(path);
        return NULL;
    }
    attachment->path = path;
    attachment->next = NULL;
    attachment->type = attachment_type;
    attachment->content_type = content_type;

    sentry_attachment_t **next_ptr = attachments_ptr;

    for (sentry_attachment_t *it = *attachments_ptr; it; it = it->next) {
        if (sentry__path_eq(it->path, path)) {
            attachment_free(attachment);
            return it;
        }

        next_ptr = &it->next;
    }

    *next_ptr = attachment;
    return attachment;
}

void
sentry__attachments_remove(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachment)
{
    if (!attachment) {
        return;
    }

    sentry_attachment_t **next_ptr = attachments_ptr;

    for (sentry_attachment_t *it = *attachments_ptr; it; it = it->next) {
        if (it == attachment || sentry__path_eq(it->path, attachment->path)) {

            *next_ptr = it->next;
            attachment_free(it);
            return;
        }

        next_ptr = &it->next;
    }
}

void
sentry__attachments_extend(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachments)
{
    if (!attachments) {
        return;
    }

    for (sentry_attachment_t *attachment = attachments; attachment;
        attachment = attachment->next) {
        sentry__attachments_add(attachments_ptr,
            sentry__path_clone(attachment->path), attachment->type,
            attachment->content_type);
    }
}
