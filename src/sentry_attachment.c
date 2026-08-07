#include "sentry_attachment.h"
#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_uuid.h"

#include <string.h>

#define ATTACHMENT_ID "id"
#define ATTACHMENT_PATH "path"
#define ATTACHMENT_BYTES "bytes"
#define ATTACHMENT_FILENAME "filename"
#define ATTACHMENT_TYPE "type"
#define ATTACHMENT_CONTENT_TYPE "content_type"

static const char *
value_string(sentry_value_t value)
{
    return sentry_value_get_type(value) == SENTRY_VALUE_TYPE_STRING
        ? sentry_value_as_string(value)
        : NULL;
}

static void
set_string_n(sentry_value_t attachment, const char *key, const char *value,
    size_t value_len)
{
    if (!value || value_len == 0) {
        sentry_value_remove_by_key(attachment, key);
        return;
    }
    sentry_value_set_by_key(
        attachment, key, sentry_value_new_string_n(value, value_len));
}

static sentry_value_t
attachment_new(void)
{
    sentry_uuid_t id = sentry_uuid_new_v4();
    sentry_value_t attachment = sentry__value_new_object_with_size(6);
    sentry_value_set_by_key(
        attachment, ATTACHMENT_ID, sentry__value_new_uuid(&id));
    return attachment;
}

static void
set_filename_from_path(sentry_value_t attachment, const sentry_path_t *path)
{
    const char *filename = sentry__path_filename(path);
    if (filename) {
        sentry_value_set_by_key(
            attachment, ATTACHMENT_FILENAME, sentry_value_new_string(filename));
    }
}

void
sentry_attachment_set_type(sentry_value_t attachment, const char *type)
{
    sentry_attachment_set_type_n(
        attachment, type, sentry__guarded_strlen(type));
}

void
sentry_attachment_set_type_n(
    sentry_value_t attachment, const char *type, size_t type_len)
{
    if (sentry_value_is_null(attachment)) {
        return;
    }

    set_string_n(attachment, ATTACHMENT_TYPE, type, type_len);

    const char *attachment_type = sentry__attachment_get_type(attachment);
    if (attachment_type && !sentry__attachment_get_content_type(attachment)) {
        if (sentry__string_eq(
                attachment_type, SENTRY_ATTACHMENT_TYPE_MINIDUMP)) {
            sentry_attachment_set_content_type(
                attachment, "application/octet-stream");
        } else if (sentry__string_eq(attachment_type,
                       SENTRY_ATTACHMENT_TYPE_VIEW_HIERARCHY)) {
            sentry_attachment_set_content_type(attachment, "application/json");
        }
    }
}

void
sentry_attachment_set_content_type(
    sentry_value_t attachment, const char *content_type)
{
    sentry_attachment_set_content_type_n(
        attachment, content_type, sentry__guarded_strlen(content_type));
}

void
sentry_attachment_set_content_type_n(sentry_value_t attachment,
    const char *content_type, size_t content_type_len)
{
    if (sentry_value_is_null(attachment)) {
        return;
    }

    set_string_n(
        attachment, ATTACHMENT_CONTENT_TYPE, content_type, content_type_len);
}

void
sentry_attachment_set_filename(sentry_value_t attachment, const char *filename)
{
    sentry_attachment_set_filename_n(
        attachment, filename, sentry__guarded_strlen(filename));
}

void
sentry_attachment_set_filename_n(
    sentry_value_t attachment, const char *filename, size_t filename_len)
{
    if (sentry_value_is_null(attachment)) {
        return;
    }

    sentry_path_t *path = sentry__path_from_str_n(filename, filename_len);
    if (!path) {
        sentry_value_remove_by_key(attachment, ATTACHMENT_FILENAME);
        return;
    }

    set_filename_from_path(attachment, path);
    sentry__path_free(path);
}

#ifdef SENTRY_PLATFORM_WINDOWS
void
sentry_attachment_set_filenamew(
    sentry_value_t attachment, const wchar_t *filename)
{
    size_t filename_len = filename ? wcslen(filename) : 0;
    sentry_attachment_set_filenamew_n(attachment, filename, filename_len);
}

void
sentry_attachment_set_filenamew_n(
    sentry_value_t attachment, const wchar_t *filename, size_t filename_len)
{
    if (sentry_value_is_null(attachment)) {
        return;
    }

    sentry_path_t *path = sentry__path_from_wstr_n(filename, filename_len);
    if (!path) {
        sentry_value_remove_by_key(attachment, ATTACHMENT_FILENAME);
        return;
    }

    set_filename_from_path(attachment, path);
    sentry__path_free(path);
}
#endif

sentry_value_t
sentry__attachment_from_path(sentry_path_t *path)
{
    if (!path) {
        return sentry_value_new_null();
    }

    sentry_value_t attachment = attachment_new();
    sentry_value_set_by_key(
        attachment, ATTACHMENT_PATH, sentry_value_new_string(path->path));
    set_filename_from_path(attachment, path);
    sentry__path_free(path);
    return attachment;
}

sentry_value_t
sentry__attachment_from_buffer(
    const char *buf, size_t buf_len, sentry_path_t *filename)
{
    if (!filename) {
        return sentry_value_new_null();
    }
    if (!buf || !buf_len) {
        sentry__path_free(filename);
        return sentry_value_new_null();
    }

    char *bytes = sentry_malloc(buf_len);
    if (!bytes) {
        sentry__path_free(filename);
        return sentry_value_new_null();
    }
    memcpy(bytes, buf, buf_len);

    sentry_value_t attachment = attachment_new();
    sentry_value_set_by_key(attachment, ATTACHMENT_BYTES,
        sentry__value_new_string_owned_n(bytes, buf_len));
    set_filename_from_path(attachment, filename);
    sentry__path_free(filename);
    return attachment;
}

const char *
sentry__attachment_get_id(sentry_value_t attachment)
{
    return value_string(sentry_value_get_by_key(attachment, ATTACHMENT_ID));
}

size_t
sentry__attachment_get_size(sentry_value_t attachment)
{
    size_t len = 0;
    if (sentry__attachment_get_bytes(attachment, &len)) {
        return len;
    }

    const char *path = sentry__attachment_get_path(attachment);
    sentry_path_t *path_obj = sentry__path_from_str(path);
    size_t size = path_obj ? sentry__path_get_size(path_obj) : 0;
    sentry__path_free(path_obj);
    return size;
}

const char *
sentry__attachment_get_filename(sentry_value_t attachment)
{
    return value_string(
        sentry_value_get_by_key(attachment, ATTACHMENT_FILENAME));
}

const char *
sentry__attachment_get_path(sentry_value_t attachment)
{
    return value_string(sentry_value_get_by_key(attachment, ATTACHMENT_PATH));
}

const char *
sentry__attachment_get_type(sentry_value_t attachment)
{
    return value_string(sentry_value_get_by_key(attachment, ATTACHMENT_TYPE));
}

const char *
sentry__attachment_get_content_type(sentry_value_t attachment)
{
    return value_string(
        sentry_value_get_by_key(attachment, ATTACHMENT_CONTENT_TYPE));
}

const char *
sentry__attachment_get_bytes(sentry_value_t attachment, size_t *len)
{
    sentry_value_t bytes
        = sentry_value_get_by_key(attachment, ATTACHMENT_BYTES);
    if (sentry_value_get_type(bytes) != SENTRY_VALUE_TYPE_STRING) {
        if (len) {
            *len = 0;
        }
        return NULL;
    }

    if (len) {
        *len = sentry_value_get_length(bytes);
    }
    return sentry_value_as_string(bytes);
}

bool
sentry__attachment_is_placeholder(
    sentry_value_t attachment, const sentry_options_t *options)
{
    return options && options->enable_large_attachments
        && sentry__attachment_get_size(attachment)
        >= SENTRY_LARGE_ATTACHMENT_SIZE;
}

void
sentry__attachment_set_path(sentry_value_t attachment, sentry_path_t *path)
{
    if (sentry_value_is_null(attachment)) {
        sentry__path_free(path);
        return;
    }
    if (!path) {
        sentry_value_remove_by_key(attachment, ATTACHMENT_PATH);
        return;
    }

    sentry_value_set_by_key(
        attachment, ATTACHMENT_PATH, sentry_value_new_string(path->path));
    set_filename_from_path(attachment, path);
    sentry__path_free(path);
}

sentry_value_t
sentry__attachments_new(void)
{
    return sentry_value_new_list();
}

void
sentry__attachments_free(sentry_value_t attachments)
{
    sentry_value_decref(attachments);
}

static bool
attachment_eq(sentry_value_t a, sentry_value_t b)
{
    const char *a_id = sentry__attachment_get_id(a);
    const char *b_id = sentry__attachment_get_id(b);
    if (a_id && b_id && sentry__string_eq(a_id, b_id)) {
        return true;
    }

    if (sentry__attachment_get_bytes(a, NULL)
        || sentry__attachment_get_bytes(b, NULL)) {
        return false;
    }

    const char *a_type = sentry__attachment_get_type(a);
    const char *b_type = sentry__attachment_get_type(b);
    if (!sentry__string_eq(a_type ? a_type : "", b_type ? b_type : "")) {
        return false;
    }

    const char *a_path = sentry__attachment_get_path(a);
    const char *b_path = sentry__attachment_get_path(b);
    return a_path && b_path && sentry__string_eq(a_path, b_path);
}

static bool
ensure_attachments(sentry_value_t *attachments_ptr)
{
    if (!attachments_ptr) {
        return false;
    }
    if (sentry_value_get_type(*attachments_ptr) == SENTRY_VALUE_TYPE_LIST) {
        return true;
    }

    *attachments_ptr = sentry__attachments_new();
    return sentry_value_get_type(*attachments_ptr) == SENTRY_VALUE_TYPE_LIST;
}

sentry_value_t
sentry__attachments_add(
    sentry_value_t *attachments_ptr, sentry_value_t attachment)
{
    if (sentry_value_is_null(attachment)
        || !ensure_attachments(attachments_ptr)) {
        sentry_value_decref(attachment);
        return sentry_value_new_null();
    }

    size_t size = sentry__attachment_get_size(attachment);
    if (size > SENTRY_MAX_ATTACHMENT_SIZE) {
        SENTRY_WARNF("rejected oversized attachment \"%s\" (%zu > %d MiB)",
            sentry__attachment_get_filename(attachment), size / (1024 * 1024),
            SENTRY_MAX_ATTACHMENT_SIZE / (1024 * 1024));
        sentry_value_decref(attachment);
        return sentry_value_new_null();
    }
    if (size >= SENTRY_LARGE_ATTACHMENT_SIZE) {
        SENTRY_INFOF("added large attachment \"%s\" (%zu MiB)",
            sentry__attachment_get_filename(attachment), size / (1024 * 1024));
    }

    size_t len = sentry_value_get_length(*attachments_ptr);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t existing
            = sentry_value_get_by_index(*attachments_ptr, i);
        if (attachment_eq(existing, attachment)) {
            sentry_value_decref(attachment);
            return sentry_value_incref(existing);
        }
    }

    sentry_value_append(*attachments_ptr, attachment);
    return sentry_value_get_by_index_owned(*attachments_ptr, len);
}

sentry_value_t
sentry__attachments_add_path(sentry_value_t *attachments_ptr,
    sentry_path_t *path, const char *attachment_type, const char *content_type)
{
    sentry_value_t attachment = sentry__attachment_from_path(path);
    if (sentry_value_is_null(attachment)) {
        return sentry_value_new_null();
    }
    sentry_attachment_set_type(attachment, attachment_type);
    if (content_type || !sentry__attachment_get_content_type(attachment)) {
        sentry_attachment_set_content_type(attachment, content_type);
    }
    return sentry__attachments_add(attachments_ptr, attachment);
}

bool
sentry__attachments_remove(
    sentry_value_t attachments, sentry_value_t attachment)
{
    if (sentry_value_is_null(attachment)
        || sentry_value_get_type(attachments) != SENTRY_VALUE_TYPE_LIST) {
        return false;
    }

    size_t len = sentry_value_get_length(attachments);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t existing = sentry_value_get_by_index(attachments, i);
        if (attachment_eq(existing, attachment)) {
            sentry_value_remove_by_index(attachments, i);
            return true;
        }
    }

    return false;
}

void
sentry__attachments_extend(
    sentry_value_t *attachments_ptr, sentry_value_t attachments)
{
    if (sentry_value_get_type(attachments) != SENTRY_VALUE_TYPE_LIST) {
        return;
    }

    size_t len = sentry_value_get_length(attachments);
    if (len == 0 || !ensure_attachments(attachments_ptr)) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        sentry_value_t clone
            = sentry__value_clone(sentry_value_get_by_index(attachments, i));
        sentry_value_decref(sentry__attachments_add(attachments_ptr, clone));
    }
}
