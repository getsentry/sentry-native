#include "sentry_attachment.h"
#include "sentry_alloc.h"
#include "sentry_path.h"
#include "sentry_string.h"

#include <string.h>

void
sentry_attachment_set_content_type(
    sentry_attachment_t *attachment, const char *content_type)
{
    sentry_attachment_set_content_type_n(
        attachment, content_type, sentry__guarded_strlen(content_type));
}

void
sentry_attachment_set_content_type_n(sentry_attachment_t *attachment,
    const char *content_type, size_t content_type_len)
{
    if (!attachment) {
        return;
    }

    sentry_free(attachment->content_type);
    attachment->content_type
        = sentry__string_clone_n(content_type, content_type_len);
}

sentry_attachment_t *
sentry__attachment_from_path(sentry_path_t *path)
{
    if (!path) {
        return NULL;
    }
    sentry_attachment_t *attachment = SENTRY_MAKE(sentry_attachment_t);
    if (!attachment) {
        sentry__path_free(path);
        return NULL;
    }
    memset(attachment, 0, sizeof(sentry_attachment_t));
    attachment->path = path;
    return attachment;
}

sentry_attachment_t *
sentry__attachment_from_buffer(
    const char *buf, size_t buf_len, sentry_path_t *filename)
{
    if (!filename) {
        return NULL;
    }
    if (!buf || !buf_len) {
        sentry__path_free(filename);
        return NULL;
    }
    sentry_attachment_t *attachment = SENTRY_MAKE(sentry_attachment_t);
    if (!attachment) {
        sentry__path_free(filename);
        return NULL;
    }
    memset(attachment, 0, sizeof(sentry_attachment_t));
    attachment->path = filename;
    attachment->buf = sentry_malloc(buf_len * sizeof(char));
    memcpy(attachment->buf, buf, buf_len * sizeof(char));
    attachment->buf_len = buf_len;
    return attachment;
}

void
sentry__attachment_free(sentry_attachment_t *attachment)
{
    if (!attachment) {
        return;
    }
    sentry__path_free(attachment->path);
    sentry_free(attachment->buf);
    sentry_free(attachment->content_type);
    sentry_free(attachment);
}

void
sentry__attachments_free(sentry_attachment_t *attachments)
{
    sentry_attachment_t *it = attachments;
    while (it) {
        sentry_attachment_t *attachment = it;
        it = attachment->next;

        sentry__attachment_free(attachment);
    }
}

static bool
attachment_eq(const sentry_attachment_t *a, const sentry_attachment_t *b)
{
    if (a == b) {
        return true;
    }
    if (!a || !b || a->buf || b->buf || a->type != b->type) {
        return false;
    }
    return sentry__path_eq(a->path, b->path);
}

sentry_attachment_t *
sentry__attachments_add(sentry_attachment_t **attachments_ptr,
    sentry_attachment_t *attachment, sentry_attachment_type_t attachment_type,
    const char *content_type)
{
    if (!attachment) {
        return NULL;
    }
    attachment->type = attachment_type;
    attachment->content_type = sentry__string_clone(content_type);

    sentry_attachment_t **next_ptr = attachments_ptr;

    for (sentry_attachment_t *it = *attachments_ptr; it; it = it->next) {
        if (attachment_eq(it, attachment)) {
            sentry__attachment_free(attachment);
            return it;
        }

        next_ptr = &it->next;
    }

    *next_ptr = attachment;
    return attachment;
}

sentry_attachment_t *
sentry__attachments_add_path(sentry_attachment_t **attachments_ptr,
    sentry_path_t *path, sentry_attachment_type_t attachment_type,
    const char *content_type)
{
    sentry_attachment_t *attachment = sentry__attachment_from_path(path);
    return sentry__attachments_add(
        attachments_ptr, attachment, attachment_type, content_type);
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
        if (attachment_eq(it, attachment)) {
            *next_ptr = it->next;
            sentry__attachment_free(it);
            return;
        }

        next_ptr = &it->next;
    }
}

static sentry_attachment_t *
attachment_clone(const sentry_attachment_t *attachment)
{
    if (!attachment) {
        return NULL;
    }

    sentry_attachment_t *clone = SENTRY_MAKE(sentry_attachment_t);
    if (!clone) {
        return NULL;
    }
    memset(clone, 0, sizeof(sentry_attachment_t));

    clone->path = sentry__path_clone(attachment->path);
    if (!clone->path) {
        goto fail;
    }
    if (attachment->buf) {
        clone->buf_len = attachment->buf_len;
        clone->buf = sentry_malloc(attachment->buf_len * sizeof(char));
        if (!clone->buf) {
            goto fail;
        }
        memcpy(clone->buf, attachment->buf, attachment->buf_len * sizeof(char));
    }
    return clone;

fail:
    sentry__attachment_free(clone);
    return NULL;
}

void
sentry__attachments_extend(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachments)
{
    for (sentry_attachment_t *it = attachments; it; it = it->next) {
        sentry__attachments_add(
            attachments_ptr, attachment_clone(it), it->type, it->content_type);
    }
}
