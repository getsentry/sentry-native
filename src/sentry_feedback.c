#include "sentry_feedback.h"

#include "sentry_alloc.h"
#include "sentry_attachment.h"
#include "sentry_path.h"
#include "sentry_string.h"

#include <string.h>

sentry_feedback_hint_t *
sentry_feedback_hint_new(void)
{
    sentry_feedback_hint_t *hint = SENTRY_MAKE(sentry_feedback_hint_t);
    if (!hint) {
        return NULL;
    }
    memset(hint, 0, sizeof(sentry_feedback_hint_t));
    return hint;
}

void
sentry__feedback_hint_free(sentry_feedback_hint_t *hint)
{
    if (!hint) {
        return;
    }
    sentry__attachments_free(hint->attachments);
    sentry_free(hint);
}

sentry_attachment_t *
sentry_feedback_hint_attach_file(
    sentry_feedback_hint_t *hint, const char *path)
{
    return sentry_feedback_hint_attach_file_n(
        hint, path, sentry__guarded_strlen(path));
}

sentry_attachment_t *
sentry_feedback_hint_attach_file_n(
    sentry_feedback_hint_t *hint, const char *path, size_t path_len)
{
    if (!hint) {
        return NULL;
    }
    return sentry__attachments_add_path(
        &hint->attachments, sentry__path_from_str_n(path, path_len), ATTACHMENT,
        NULL);
}

sentry_attachment_t *
sentry_feedback_hint_attach_bytes(sentry_feedback_hint_t *hint, const char *buf,
    size_t buf_len, const char *filename)
{
    return sentry_feedback_hint_attach_bytes_n(
        hint, buf, buf_len, filename, sentry__guarded_strlen(filename));
}

sentry_attachment_t *
sentry_feedback_hint_attach_bytes_n(sentry_feedback_hint_t *hint,
    const char *buf, size_t buf_len, const char *filename, size_t filename_len)
{
    if (!hint) {
        return NULL;
    }
    return sentry__attachments_add(&hint->attachments,
        sentry__attachment_from_buffer(
            buf, buf_len, sentry__path_from_str_n(filename, filename_len)),
        ATTACHMENT, NULL);
}

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_attachment_t *
sentry_feedback_hint_attach_filew(
    sentry_feedback_hint_t *hint, const wchar_t *path)
{
    size_t path_len = path ? wcslen(path) : 0;
    return sentry_feedback_hint_attach_filew_n(hint, path, path_len);
}

sentry_attachment_t *
sentry_feedback_hint_attach_filew_n(
    sentry_feedback_hint_t *hint, const wchar_t *path, size_t path_len)
{
    if (!hint) {
        return NULL;
    }
    return sentry__attachments_add_path(&hint->attachments,
        sentry__path_from_wstr_n(path, path_len), ATTACHMENT, NULL);
}

sentry_attachment_t *
sentry_feedback_hint_attach_bytesw(sentry_feedback_hint_t *hint, const char *buf,
    size_t buf_len, const wchar_t *filename)
{
    size_t filename_len = filename ? wcslen(filename) : 0;
    return sentry_feedback_hint_attach_bytesw_n(
        hint, buf, buf_len, filename, filename_len);
}

sentry_attachment_t *
sentry_feedback_hint_attach_bytesw_n(sentry_feedback_hint_t *hint,
    const char *buf, size_t buf_len, const wchar_t *filename,
    size_t filename_len)
{
    if (!hint) {
        return NULL;
    }
    return sentry__attachments_add(&hint->attachments,
        sentry__attachment_from_buffer(
            buf, buf_len, sentry__path_from_wstr_n(filename, filename_len)),
        ATTACHMENT, NULL);
}
#endif