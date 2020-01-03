#include <string.h>

#include "sentry_alloc.h"
#include "sentry_string.h"

#define INITIAL_BUFFER_SIZE 128

void
sentry__stringbuilder_init(sentry_stringbuilder_t *sb)
{
    sb->buf = NULL;
    sb->allocated = 0;
    sb->len = 0;
}

static int
append(sentry_stringbuilder_t *sb, const char *s, size_t len)
{
    size_t needed = sb->len + len + 1;
    if (needed > sb->allocated) {
        size_t new_alloc_size = sb->allocated;
        if (new_alloc_size == 0) {
            new_alloc_size = INITIAL_BUFFER_SIZE;
        }
        while (new_alloc_size < needed) {
            new_alloc_size = new_alloc_size * 2;
        }
        char *new_buf = sentry_malloc(new_alloc_size);
        if (!new_buf) {
            return 1;
        }
        if (sb->buf) {
            memcpy(new_buf, sb->buf, sb->allocated);
            sentry_free(sb->buf);
        }
        sb->buf = new_buf;
        sb->allocated = new_alloc_size;
    }
    memcpy(sb->buf + sb->len, s, len);
    sb->len += len;
    /* make sure we're always zero terminated */
    sb->buf[sb->len] = '\0';
    return 0;
}

int
sentry__stringbuilder_append(sentry_stringbuilder_t *sb, const char *s)
{
    return append(sb, s, strlen(s));
}

int
sentry__stringbuilder_append_buf(
    sentry_stringbuilder_t *sb, const char *s, size_t len)
{
    return append(sb, s, len);
}

int
sentry__stringbuilder_append_char(sentry_stringbuilder_t *sb, char c)
{
    return append(sb, &c, 1);
}

char *
sentry_stringbuilder_take_string(sentry_stringbuilder_t *sb)
{
    char *rv = sb->buf;
    if (!rv) {
        rv = sentry__string_dup("");
    }
    sb->buf = 0;
    sb->allocated = 0;
    sb->len = 0;
    return rv;
}

char *
sentry__stringbuilder_into_string(sentry_stringbuilder_t *sb)
{
    char *rv = sentry_stringbuilder_take_string(sb);
    sentry__stringbuilder_cleanup(sb);
    return rv;
}

void
sentry__stringbuilder_cleanup(sentry_stringbuilder_t *sb)
{
    sentry_free(sb->buf);
}

size_t
sentry__stringbuilder_len(const sentry_stringbuilder_t *sb)
{
    return sb->len;
}

char *
sentry__string_dup(const char *str)
{
    return str ? sentry__string_dupn(str, strlen(str)) : NULL;
}

char *
sentry__string_dupn(const char *str, size_t n)
{
    size_t len = n + 1;
    char *rv = sentry_malloc(len);
    if (rv) {
        memcpy(rv, str, n);
    }
    rv[n] = 0;
    return rv;
}
