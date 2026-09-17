#include "sentry_hint.h"

#include "sentry_alloc.h"
#include "sentry_attachment.h"
#include "sentry_string.h"

#include <string.h>

sentry_hint_t *
sentry_hint_new(void)
{
    sentry_hint_t *hint = SENTRY_MAKE(sentry_hint_t);
    if (!hint) {
        return NULL;
    }
    SENTRY__HINT_INIT(*hint);
    return hint;
}

void
sentry__hint_set_attachments(sentry_hint_t *hint, sentry_value_t attachments)
{
    if (hint) {
        sentry_value_decref(hint->attachments);
        hint->attachments = attachments;
    } else {
        sentry_value_decref(attachments);
    }
}

void
sentry__hint_free(sentry_hint_t *hint)
{
    if (!hint) {
        return;
    }
    SENTRY__HINT_DEINIT(*hint);
    sentry_free(hint);
}

sentry_uuid_t
sentry_hint_add_attachment(sentry_hint_t *hint, sentry_value_t attachment)
{
    if (!hint) {
        sentry_value_decref(attachment);
        return sentry_uuid_nil();
    }

    size_t len = sentry_value_get_length(hint->attachments);
    sentry_value_t attachments = sentry_value_is_frozen(hint->attachments)
        ? sentry__attachments_clone(hint->attachments)
        : sentry_value_incref(hint->attachments);
    if (len && sentry_value_is_null(attachments)) {
        sentry_value_decref(attachment);
        return sentry_uuid_nil();
    }
    sentry_value_t added = sentry__attachments_add(&attachments, attachment);
    if (sentry_value_get_length(attachments) != len) {
        sentry__hint_set_attachments(hint, attachments);
    } else {
        sentry_value_decref(attachments);
    }
    sentry_uuid_t attachment_id = sentry__attachment_get_id(added);
    sentry_value_decref(added);
    return attachment_id;
}

sentry_uuid_t
sentry_hint_attach_file(sentry_hint_t *hint, const char *path)
{
    return sentry_hint_attach_file_n(hint, path, sentry__guarded_strlen(path));
}

sentry_uuid_t
sentry_hint_attach_file_n(
    sentry_hint_t *hint, const char *path, size_t path_len)
{
    return sentry_hint_add_attachment(
        hint, sentry_attachment_from_file_n(path, path_len));
}

sentry_uuid_t
sentry_hint_attach_bytes(
    sentry_hint_t *hint, const char *buf, size_t buf_len, const char *filename)
{
    return sentry_hint_attach_bytes_n(
        hint, buf, buf_len, filename, sentry__guarded_strlen(filename));
}

sentry_uuid_t
sentry_hint_attach_bytes_n(sentry_hint_t *hint, const char *buf, size_t buf_len,
    const char *filename, size_t filename_len)
{
    return sentry_hint_add_attachment(hint,
        sentry_attachment_from_bytes_n(buf, buf_len, filename, filename_len));
}

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_uuid_t
sentry_hint_attach_filew(sentry_hint_t *hint, const wchar_t *path)
{
    size_t path_len = path ? wcslen(path) : 0;
    return sentry_hint_attach_filew_n(hint, path, path_len);
}

sentry_uuid_t
sentry_hint_attach_filew_n(
    sentry_hint_t *hint, const wchar_t *path, size_t path_len)
{
    return sentry_hint_add_attachment(
        hint, sentry_attachment_from_filew_n(path, path_len));
}

sentry_uuid_t
sentry_hint_attach_bytesw(sentry_hint_t *hint, const char *buf, size_t buf_len,
    const wchar_t *filename)
{
    size_t filename_len = filename ? wcslen(filename) : 0;
    return sentry_hint_attach_bytesw_n(
        hint, buf, buf_len, filename, filename_len);
}

sentry_uuid_t
sentry_hint_attach_bytesw_n(sentry_hint_t *hint, const char *buf,
    size_t buf_len, const wchar_t *filename, size_t filename_len)
{
    return sentry_hint_add_attachment(hint,
        sentry_attachment_from_bytesw_n(buf, buf_len, filename, filename_len));
}
#endif

void
sentry_hint_remove_attachment(sentry_hint_t *hint, sentry_uuid_t attachment_id)
{
    if (hint) {
        sentry_value_t attachments = sentry_value_is_frozen(hint->attachments)
            ? sentry__attachments_clone(hint->attachments)
            : sentry_value_incref(hint->attachments);
        sentry_value_t removed
            = sentry__attachments_remove(attachments, &attachment_id);
        if (!sentry_value_is_null(removed)) {
            sentry__hint_set_attachments(hint, attachments);
        } else {
            sentry_value_decref(attachments);
        }
        sentry_value_decref(removed);
    }
}

void
sentry_hint_clear_attachments(sentry_hint_t *hint)
{
    if (hint && sentry_value_get_length(hint->attachments)) {
        sentry_value_t attachments = sentry_value_new_list();
        if (!sentry_value_is_null(attachments)) {
            sentry__hint_set_attachments(hint, attachments);
        }
    }
}
