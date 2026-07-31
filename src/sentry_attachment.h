#ifndef SENTRY_ATTACHMENT_H_INCLUDED
#define SENTRY_ATTACHMENT_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"

#define SENTRY_LARGE_ATTACHMENT_SIZE (100 * 1024 * 1024) // 100 MiB
#define SENTRY_MAX_ATTACHMENT_SIZE (1024 * 1024 * 1024) // 1 GiB

/**
 * This is a linked list of all the attachments registered via
 * `sentry_options_add_attachment`.
 *
 * This struct represents a union of two attachment types:
 * - File attachments: `path` is set, `buf`/`buf_len` are NULL/0
 * - Buffer attachments: `buf`/`buf_len` are set, `path` is NULL
 *
 * The `filename` field is used for both types to specify the attachment
 * name in the envelope (defaults to basename of `path` for file attachments).
 */
struct sentry_attachment_s {
    // File attachment data (mutually exclusive with buffer data)
    sentry_path_t *path; // Full path to file on disk (NULL for buffers)

    // Buffer attachment data (mutually exclusive with file data)
    char *buf; // In-memory buffer content (NULL for files)
    size_t buf_len; // Buffer size in bytes (0 for files)

    // Common fields for both attachment types
    sentry_path_t *filename; // Attachment name in envelope (can be NULL)
    char *type;
    char *content_type;
    sentry_attachment_t *next; // Linked list pointer
};

/**
 * These mutate the `attachment` without locking or flushing the scope, for
 * attachments that are not owned by the global scope (options, hints, and
 * backend-internal lists).
 *
 * The public `sentry_attachment_set_*` counterparts must be used for anything
 * that can be scope-owned: they mutate under the scope lock and flush the
 * backend on unlock, so a backend's on-disk crash state stays in sync.
 */
void sentry__attachment_set_type_n(
    sentry_attachment_t *attachment, const char *type, size_t type_len);
void sentry__attachment_set_content_type_n(sentry_attachment_t *attachment,
    const char *content_type, size_t content_type_len);
void sentry__attachment_set_filename_n(
    sentry_attachment_t *attachment, const char *filename, size_t filename_len);
#ifdef SENTRY_PLATFORM_WINDOWS
void sentry__attachment_set_filenamew_n(sentry_attachment_t *attachment,
    const wchar_t *filename, size_t filename_len);
#endif

/**
 * Returns the size in bytes of the attachment's data (buffer length or file
 * size).
 */
size_t sentry__attachment_get_size(const sentry_attachment_t *attachment);

/**
 * Returns the filename string for the attachment (basename of `filename` if
 * set, otherwise basename of `path`).
 */
const char *sentry__attachment_get_filename(
    const sentry_attachment_t *attachment);

/**
 * Returns true if the attachment should be represented as an attachment-ref.
 */
bool sentry__attachment_is_placeholder(
    const sentry_attachment_t *att, const sentry_options_t *options);

/**
 *  Creates a new file attachment. Takes ownership of `path`.
 */
sentry_attachment_t *sentry__attachment_from_path(sentry_path_t *path);

/**
 * Creates a new byte attachment from a copy of `buf`. Takes ownership of
 * `filename`.
 */
sentry_attachment_t *sentry__attachment_from_buffer(
    const char *buf, size_t buf_len, sentry_path_t *filename);

/**
 *  Frees the `attachment`.
 */
void sentry__attachment_free(sentry_attachment_t *attachment);

/**
 * Frees the linked list of `attachments`.
 */
void sentry__attachments_free(sentry_attachment_t *attachments);

/**
 * Adds an attachment to the attachments list at `attachments_ptr`.
 */
sentry_attachment_t *sentry__attachments_add(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachment);

/**
 * Adds a file attachment to the attachments list at `attachments_ptr`.
 */
sentry_attachment_t *sentry__attachments_add_path(
    sentry_attachment_t **attachments_ptr, sentry_path_t *path,
    const char *attachment_type, const char *content_type);

/**
 * Removes an attachment from the attachments list at `attachments_ptr`.
 * Returns true if the attachment was found and removed.
 */
bool sentry__attachments_remove(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachment);

/**
 * Extends the linked list of attachments at `attachments_ptr` with all
 * attachments in `attachments`.
 */
void sentry__attachments_extend(
    sentry_attachment_t **attachments_ptr, sentry_attachment_t *attachments);

#endif
