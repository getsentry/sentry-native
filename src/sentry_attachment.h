#ifndef SENTRY_ATTACHMENT_H_INCLUDED
#define SENTRY_ATTACHMENT_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"
#include "sentry_value.h"

#define SENTRY_LARGE_ATTACHMENT_SIZE (100 * 1024 * 1024) // 100 MiB
#define SENTRY_MAX_ATTACHMENT_SIZE (1024 * 1024 * 1024) // 1 GiB

/*
 * Attachments are object values with two data representations:
 * - File attachments store a file `path`.
 * - Byte attachments store in-memory `bytes`; a backend may also assign a path
 *   when it writes those bytes to disk.
 *
 * Both types store a `filename` as the attachment name in the envelope. It
 * defaults to the basename of `path` for file attachments.
 */

/**
 * Returns the attachment type.
 */
const char *sentry__attachment_get_type(sentry_value_t attachment);

/**
 * Returns the attachment content type.
 */
const char *sentry__attachment_get_content_type(sentry_value_t attachment);

/**
 * Returns the size in bytes of the attachment's data (buffer length or file
 * size).
 */
size_t sentry__attachment_get_size(sentry_value_t attachment);

/**
 * Returns the in-memory attachment bytes and writes their length to `len` if
 * provided.
 */
const char *sentry__attachment_get_bytes(
    sentry_value_t attachment, size_t *len);

/**
 * Returns the attachment bytes as an owned string value.
 */
sentry_value_t sentry__attachment_get_bytes_owned(sentry_value_t attachment);

/**
 * Returns the filename string for the attachment (basename of `filename` if
 * set, otherwise basename of `path`).
 */
const char *sentry__attachment_get_filename(sentry_value_t attachment);

/**
 * Returns the attachment path string.
 */
const char *sentry__attachment_get_path(sentry_value_t attachment);

/**
 * Creates an owned path object for the attachment path.
 */
sentry_path_t *sentry__attachment_make_path(sentry_value_t attachment);

/**
 * Returns true if the attachment should be represented as an attachment-ref.
 */
bool sentry__attachment_is_placeholder(
    sentry_value_t attachment, const sentry_options_t *options);

/**
 * Returns the attachment UUID, or a nil UUID when the ID is missing or invalid.
 */
sentry_uuid_t sentry__attachment_get_id(sentry_value_t attachment);

/**
 * Returns whether two attachments have the same non-nil ID.
 */
bool sentry__attachment_eq(sentry_value_t a, sentry_value_t b);

/**
 * Returns whether an attachment has a non-nil ID, a non-empty filename, and
 * either a path or bytes.
 */
bool sentry__attachment_is_valid(sentry_value_t attachment);

/**
 * Sets the attachment path. Passing `NULL` removes it.
 */
void sentry__attachment_set_path(sentry_value_t attachment, const char *path);

/**
 * Adds an attachment to the attachments list at `attachments_ptr`.
 */
sentry_value_t sentry__attachments_add(
    sentry_value_t *attachments_ptr, sentry_value_t attachment);

/**
 * Adds a file attachment to the attachments list at `attachments_ptr`.
 */
sentry_value_t sentry__attachments_add_path(sentry_value_t *attachments_ptr,
    const char *path, const char *attachment_type, const char *content_type);
sentry_value_t sentry__attachments_add_path_n(sentry_value_t *attachments_ptr,
    const char *path, size_t path_len, const char *attachment_type,
    const char *content_type);
#ifdef SENTRY_PLATFORM_WINDOWS
sentry_value_t sentry__attachments_add_wpath_n(sentry_value_t *attachments_ptr,
    const wchar_t *path, size_t path_len, const char *attachment_type,
    const char *content_type);
#endif

/**
 * Removes an attachment from the attachments list by UUID.
 * Returns the removed attachment as an owned value.
 */
sentry_value_t sentry__attachments_remove(
    sentry_value_t attachments, const sentry_uuid_t *attachment_id);

/**
 * Extends the list of attachments at `attachments_ptr` with all
 * attachments in `attachments`.
 */
void sentry__attachments_extend(
    sentry_value_t *attachments_ptr, sentry_value_t attachments);

/**
 * Returns an owned matching attachment from the attachments list.
 */
sentry_value_t sentry__attachments_find(
    sentry_value_t attachments, sentry_value_t attachment);

#endif
