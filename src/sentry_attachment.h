#ifndef SENTRY_ATTACHMENT_H_INCLUDED
#define SENTRY_ATTACHMENT_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"
#include "sentry_value.h"

#define SENTRY_LARGE_ATTACHMENT_SIZE (100 * 1024 * 1024) // 100 MiB
#define SENTRY_MAX_ATTACHMENT_SIZE (1024 * 1024 * 1024) // 1 GiB

const char *sentry__attachment_get_id(sentry_value_t attachment);
size_t sentry__attachment_get_size(sentry_value_t attachment);
const char *sentry__attachment_get_filename(sentry_value_t attachment);
const char *sentry__attachment_get_path(sentry_value_t attachment);
const char *sentry__attachment_get_type(sentry_value_t attachment);
const char *sentry__attachment_get_content_type(sentry_value_t attachment);
const char *sentry__attachment_get_bytes(
    sentry_value_t attachment, size_t *len);

bool sentry__attachment_is_placeholder(
    sentry_value_t attachment, const sentry_options_t *options);

sentry_value_t sentry__attachment_from_path(sentry_path_t *path);
sentry_value_t sentry__attachment_from_buffer(
    const char *buf, size_t buf_len, sentry_path_t *filename);

void sentry__attachment_set_path(
    sentry_value_t attachment, sentry_path_t *path);

sentry_value_t sentry__attachments_new(void);
void sentry__attachments_free(sentry_value_t attachments);
sentry_value_t sentry__attachments_add(
    sentry_value_t *attachments_ptr, sentry_value_t attachment);
sentry_value_t sentry__attachments_add_path(sentry_value_t *attachments_ptr,
    sentry_path_t *path, const char *attachment_type, const char *content_type);
bool sentry__attachments_remove(
    sentry_value_t attachments, sentry_value_t attachment);
void sentry__attachments_extend(
    sentry_value_t *attachments_ptr, sentry_value_t attachments);

#endif
