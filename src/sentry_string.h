#ifndef SENTRY_STRING_H_INCLUDED
#define SENTRY_STRING_H_INCLUDED

#include "sentry_boot.h"

#include <ctype.h>
#include <stdio.h>

/* a string builder can be used to concatenate bytes together. */
typedef struct sentry_stringbuilder_s {
    char *buf;
    size_t allocated;
    size_t len;
} sentry_stringbuilder_t;

/* creates a new string builder */
void sentry__stringbuilder_init(sentry_stringbuilder_t *sb);

/* appends a zero terminated string to the builder */
int sentry__stringbuilder_append(sentry_stringbuilder_t *sb, const char *s);

/* appends a buffer */
int sentry__stringbuilder_append_buf(
    sentry_stringbuilder_t *sb, const char *s, size_t len);

/* appends a character */
int sentry__stringbuilder_append_char(sentry_stringbuilder_t *sb, char c);

/* appends an int64 */
static inline int
sentry__stringbuilder_append_int64(sentry_stringbuilder_t *sb, int64_t val)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%" PRId64, val);
    return sentry__stringbuilder_append(sb, buf);
}

/* detaches the buffer from the string builder and deallocates it */
char *sentry__stringbuilder_into_string(sentry_stringbuilder_t *sb);

/* detaches the buffer from the string builder */
char *sentry_stringbuilder_take_string(sentry_stringbuilder_t *sb);

/* deallocates the string builder */
void sentry__stringbuilder_cleanup(sentry_stringbuilder_t *sb);

/* returns the number of bytes in the string builder */
size_t sentry__stringbuilder_len(const sentry_stringbuilder_t *sb);

/* duplicates a zero terminated string */
char *sentry__string_clone(const char *str);

/* duplicates a zero terminated string with a length limit */
char *sentry__string_clonen(const char *str, size_t n);

/* converts a string to lowercase */
static inline void
sentry__string_ascii_lower(char *s)
{
    for (; *s; s++) {
        *s = (char)tolower((char)*s);
    }
}

/* converts an int64_t into a string */
static inline char *
sentry__int64_to_string(int64_t val)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%" PRId64, val);
    return sentry__string_clone(buf);
}

#ifdef SENTRY_PLATFORM_WINDOWS
/* create a string from a wstr */
char *sentry__string_from_wstr(const wchar_t *s);
/* convert a normal string to a wstr */
wchar_t *sentry__string_to_wstr(const char *s);
#endif

#endif
