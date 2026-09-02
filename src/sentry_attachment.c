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

static void set_string_n(sentry_value_t attachment, const char *key,
    const char *value, size_t value_len);
static void set_filename_from_string_n(
    sentry_value_t attachment, const char *path, size_t path_len);
static const char *value_as_string_or_null(sentry_value_t value);

const char *
sentry__attachment_get_type(sentry_value_t attachment)
{
    return value_as_string_or_null(
        sentry_value_get_by_key(attachment, ATTACHMENT_TYPE));
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

const char *
sentry__attachment_get_content_type(sentry_value_t attachment)
{
    return value_as_string_or_null(
        sentry_value_get_by_key(attachment, ATTACHMENT_CONTENT_TYPE));
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

    set_filename_from_string_n(attachment, filename, filename_len);
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

    char *filename_utf8 = sentry__string_from_wstr_n(filename, filename_len);
    if (!filename_utf8) {
        sentry_value_remove_by_key(attachment, ATTACHMENT_FILENAME);
        return;
    }

    set_filename_from_string_n(
        attachment, filename_utf8, strlen(filename_utf8));
    sentry_free(filename_utf8);
}
#endif

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

static const char *
value_as_string_or_null(sentry_value_t value)
{
    return sentry_value_get_type(value) == SENTRY_VALUE_TYPE_STRING
        ? sentry_value_as_string(value)
        : NULL;
}

static sentry_value_t
attachment_new(void)
{
    sentry_uuid_t id = sentry_uuid_new_v4();
    if (sentry_uuid_is_nil(&id)) {
        return sentry_value_new_null();
    }
    sentry_value_t attachment = sentry__value_new_object_with_size(6);
    sentry_value_set_by_key(
        attachment, ATTACHMENT_ID, sentry__value_new_uuid(&id));
    return attachment;
}

static void
set_filename_from_string_n(
    sentry_value_t attachment, const char *path, size_t path_len)
{
    if (!path) {
        sentry_value_remove_by_key(attachment, ATTACHMENT_FILENAME);
        return;
    }

    const char *filename = sentry__path_filename_from_str_n(path, path_len);
    size_t filename_len = path_len - (size_t)(filename - path);

    set_string_n(attachment, ATTACHMENT_FILENAME, filename, filename_len);
}

static void
set_path_value(sentry_value_t attachment, sentry_value_t path)
{
    if (sentry_value_is_null(attachment)
        || sentry_value_get_type(path) != SENTRY_VALUE_TYPE_STRING) {
        sentry_value_decref(path);
        return;
    }

    sentry_value_remove_by_key(attachment, ATTACHMENT_BYTES);
    set_filename_from_string_n(attachment, sentry_value_as_string(path),
        sentry_value_get_length(path));
    sentry_value_set_by_key(attachment, ATTACHMENT_PATH, path);
}

sentry_value_t
sentry_attachment_from_file(const char *path)
{
    return sentry_attachment_from_file_n(path, sentry__guarded_strlen(path));
}

sentry_value_t
sentry_attachment_from_file_n(const char *path, size_t path_len)
{
    if (!path) {
        return sentry_value_new_null();
    }

    sentry_value_t attachment = attachment_new();
    set_path_value(attachment, sentry_value_new_string_n(path, path_len));
    if (!sentry__attachment_is_valid(attachment)) {
        sentry_value_decref(attachment);
        return sentry_value_new_null();
    }
    return attachment;
}

sentry_value_t
sentry_attachment_from_bytes(
    const char *buf, size_t buf_len, const char *filename)
{
    return sentry_attachment_from_bytes_n(
        buf, buf_len, filename, sentry__guarded_strlen(filename));
}

sentry_value_t
sentry_attachment_from_bytes_n(
    const char *buf, size_t buf_len, const char *filename, size_t filename_len)
{
    if (!buf || !buf_len || !filename) {
        return sentry_value_new_null();
    }

    sentry_value_t attachment = attachment_new();
    sentry_value_set_by_key(
        attachment, ATTACHMENT_BYTES, sentry_value_new_string_n(buf, buf_len));
    set_filename_from_string_n(attachment, filename, filename_len);
    if (!sentry__attachment_is_valid(attachment)) {
        sentry_value_decref(attachment);
        return sentry_value_new_null();
    }
    return attachment;
}

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_value_t
sentry_attachment_from_filew(const wchar_t *path)
{
    size_t path_len = path ? wcslen(path) : 0;
    return sentry_attachment_from_filew_n(path, path_len);
}

sentry_value_t
sentry_attachment_from_filew_n(const wchar_t *path, size_t path_len)
{
    if (!path) {
        return sentry_value_new_null();
    }

    char *path_utf8 = sentry__string_from_wstr_n(path, path_len);
    if (!path_utf8) {
        return sentry_value_new_null();
    }
    sentry_value_t attachment = sentry_attachment_from_file(path_utf8);
    sentry_free(path_utf8);
    return attachment;
}

sentry_value_t
sentry_attachment_from_bytesw(
    const char *buf, size_t buf_len, const wchar_t *filename)
{
    size_t filename_len = filename ? wcslen(filename) : 0;
    return sentry_attachment_from_bytesw_n(
        buf, buf_len, filename, filename_len);
}

sentry_value_t
sentry_attachment_from_bytesw_n(const char *buf, size_t buf_len,
    const wchar_t *filename, size_t filename_len)
{
    if (!buf || !buf_len || !filename) {
        return sentry_value_new_null();
    }

    char *filename_utf8 = sentry__string_from_wstr_n(filename, filename_len);
    if (!filename_utf8) {
        return sentry_value_new_null();
    }

    sentry_value_t attachment = sentry_attachment_from_bytes_n(
        buf, buf_len, filename_utf8, strlen(filename_utf8));
    sentry_free(filename_utf8);
    return attachment;
}
#endif

size_t
sentry__attachment_get_size(sentry_value_t attachment)
{
#ifdef SENTRY_UNITTEST
    sentry_value_t size_value = sentry_value_get_by_key(attachment, "size");
    if (sentry_value_get_type(size_value) == SENTRY_VALUE_TYPE_UINT64) {
        return (size_t)sentry_value_as_uint64(size_value);
    }
#endif

    size_t len = 0;
    if (sentry__attachment_get_bytes(attachment, &len)) {
        return len;
    }

    sentry_path_t *path = sentry__attachment_make_path(attachment);
    size_t size = path ? sentry__path_get_size(path) : 0;
    sentry__path_free(path);
    return size;
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

sentry_value_t
sentry__attachment_get_bytes_owned(sentry_value_t attachment)
{
    sentry_value_t bytes
        = sentry_value_get_by_key_owned(attachment, ATTACHMENT_BYTES);
    if (sentry_value_get_type(bytes) != SENTRY_VALUE_TYPE_STRING) {
        sentry_value_decref(bytes);
        return sentry_value_new_null();
    }
    return bytes;
}

const char *
sentry__attachment_get_filename(sentry_value_t attachment)
{
    const char *filename = value_as_string_or_null(
        sentry_value_get_by_key(attachment, ATTACHMENT_FILENAME));
    if (filename) {
        return sentry__path_filename_from_str(filename);
    }

    return sentry__path_filename_from_str(
        sentry__attachment_get_path(attachment));
}

const char *
sentry__attachment_get_path(sentry_value_t attachment)
{
    return value_as_string_or_null(
        sentry_value_get_by_key(attachment, ATTACHMENT_PATH));
}

sentry_path_t *
sentry__attachment_make_path(sentry_value_t attachment)
{
    return sentry__path_from_str(sentry__attachment_get_path(attachment));
}

bool
sentry__attachment_is_placeholder(
    sentry_value_t attachment, const sentry_options_t *options)
{
    return options && options->enable_large_attachments
        && sentry__attachment_get_size(attachment)
        >= SENTRY_LARGE_ATTACHMENT_SIZE;
}

sentry_uuid_t
sentry__attachment_get_id(sentry_value_t attachment)
{
    return sentry__value_as_uuid(
        sentry_value_get_by_key(attachment, ATTACHMENT_ID));
}

bool
sentry__attachment_eq(sentry_value_t a, sentry_value_t b)
{
    sentry_uuid_t a_id = sentry__attachment_get_id(a);
    sentry_uuid_t b_id = sentry__attachment_get_id(b);
    return !sentry_uuid_is_nil(&a_id) && !sentry_uuid_is_nil(&b_id)
        && memcmp(a_id.bytes, b_id.bytes, sizeof(a_id.bytes)) == 0;
}

bool
sentry__attachment_is_valid(sentry_value_t attachment)
{
    if (sentry_value_get_type(attachment) != SENTRY_VALUE_TYPE_OBJECT) {
        return false;
    }
    sentry_uuid_t attachment_id = sentry__attachment_get_id(attachment);
    const char *filename = sentry__attachment_get_filename(attachment);
    return !sentry_uuid_is_nil(&attachment_id)
        && (sentry__attachment_get_path(attachment)
            || sentry__attachment_get_bytes(attachment, NULL))
        && !sentry__string_empty(filename);
}

void
sentry__attachment_set_path(sentry_value_t attachment, const char *path)
{
    if (sentry_value_is_null(attachment)) {
        return;
    }
    if (!path) {
        sentry_value_remove_by_key(attachment, ATTACHMENT_PATH);
        return;
    }

    sentry_value_set_by_key(
        attachment, ATTACHMENT_PATH, sentry_value_new_string(path));
}

/**
 * Compares attachments for equality to avoid adding duplicates.
 *
 * File attachments are equal if their paths and types are equal. Byte
 * attachments are only equal when they have the same identity; filenames need
 * not be unique, and the bytes are not compared.
 */
static bool
attachment_dedupe_eq(sentry_value_t a, sentry_value_t b)
{
    if (sentry__attachment_eq(a, b)) {
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

    sentry_value_decref(*attachments_ptr);
    *attachments_ptr = sentry_value_new_list();
    return sentry_value_get_type(*attachments_ptr) == SENTRY_VALUE_TYPE_LIST;
}

sentry_value_t
sentry__attachments_add(
    sentry_value_t *attachments_ptr, sentry_value_t attachment)
{
    if (!sentry__attachment_is_valid(attachment)
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

    // Attachment metadata must not diverge after observers persist its crash
    // state.
    sentry_value_freeze(attachment);

    sentry_value_t existing
        = sentry__attachments_find(*attachments_ptr, attachment);
    if (!sentry_value_is_null(existing)) {
        sentry_value_decref(attachment);
        return existing;
    }

    size_t len = sentry_value_get_length(*attachments_ptr);
    sentry_value_append(*attachments_ptr, attachment);
    return sentry_value_get_by_index_owned(*attachments_ptr, len);
}

static sentry_value_t
attachments_add_path_value(sentry_value_t *attachments_ptr, sentry_value_t path,
    const char *attachment_type, const char *content_type)
{
    sentry_value_t attachment = attachment_new();
    if (sentry_value_is_null(attachment)
        || sentry_value_get_type(path) != SENTRY_VALUE_TYPE_STRING) {
        sentry_value_decref(path);
        sentry_value_decref(attachment);
        return sentry_value_new_null();
    }
    set_path_value(attachment, path);
    sentry_attachment_set_type(attachment, attachment_type);
    if (content_type) {
        sentry_attachment_set_content_type(attachment, content_type);
    }
    return sentry__attachments_add(attachments_ptr, attachment);
}

sentry_value_t
sentry__attachments_add_path(sentry_value_t *attachments_ptr, const char *path,
    const char *attachment_type, const char *content_type)
{
    return sentry__attachments_add_path_n(attachments_ptr, path,
        sentry__guarded_strlen(path), attachment_type, content_type);
}

sentry_value_t
sentry__attachments_add_path_n(sentry_value_t *attachments_ptr,
    const char *path, size_t path_len, const char *attachment_type,
    const char *content_type)
{
    if (!path) {
        return sentry_value_new_null();
    }

    return attachments_add_path_value(attachments_ptr,
        sentry_value_new_string_n(path, path_len), attachment_type,
        content_type);
}

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_value_t
sentry__attachments_add_wpath_n(sentry_value_t *attachments_ptr,
    const wchar_t *path, size_t path_len, const char *attachment_type,
    const char *content_type)
{
    return attachments_add_path_value(attachments_ptr,
        sentry__value_new_string_owned(
            sentry__string_from_wstr_n(path, path_len)),
        attachment_type, content_type);
}
#endif

sentry_value_t
sentry__attachments_remove(
    sentry_value_t attachments, const sentry_uuid_t *attachment_id)
{
    if (!attachment_id || sentry_uuid_is_nil(attachment_id)
        || sentry_value_get_type(attachments) != SENTRY_VALUE_TYPE_LIST) {
        return sentry_value_new_null();
    }

    size_t len = sentry_value_get_length(attachments);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t existing = sentry_value_get_by_index(attachments, i);
        sentry_uuid_t existing_id = sentry__attachment_get_id(existing);
        if (memcmp(existing_id.bytes, attachment_id->bytes,
                sizeof(existing_id.bytes))
            == 0) {
            sentry_value_t removed = sentry_value_incref(existing);
            sentry_value_remove_by_index(attachments, i);
            return removed;
        }
    }

    return sentry_value_new_null();
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
        sentry_value_t attachment
            = sentry_value_get_by_index_owned(attachments, i);
        sentry_value_decref(
            sentry__attachments_add(attachments_ptr, attachment));
    }
}

sentry_value_t
sentry__attachments_find(sentry_value_t attachments, sentry_value_t attachment)
{
    size_t len = sentry_value_get_length(attachments);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t existing = sentry_value_get_by_index(attachments, i);
        if (attachment_dedupe_eq(existing, attachment)) {
            return sentry_value_incref(existing);
        }
    }
    return sentry_value_new_null();
}
