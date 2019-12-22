#ifndef SENTRY_STRING_H_INCLUDED
#define SENTRY_STRING_H_INCLUDED

#include <ctype.h>
#include <sentry.h>

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

/* appends a character */
int sentry__stringbuilder_append_char(sentry_stringbuilder_t *sb, char c);

/* detaches the buffer from the string builder and deallocates it */
char *sentry__stringbuilder_into_string(sentry_stringbuilder_t *sb);

/* detaches the buffer from the string builder */
char *sentry_stringbuilder_take_string(sentry_stringbuilder_t *sb);

/* deallocates the string builder */
void sentry__stringbuilder_cleanup(sentry_stringbuilder_t *sb);

/* returns the number of bytes in the string builder */
size_t sentry__stringbuilder_len(const sentry_stringbuilder_t *sb);

/* duplicates a zero terminated string */
char *sentry__string_dup(const char *str);

/* duplicates a zero terminated string with a length limit */
char *sentry__string_dupn(const char *str, size_t n);

/* converts a string to lowercase */
static inline void
sentry__string_ascii_lower(char *s)
{
    for (; *s; s++) {
        *s = (char)tolower((char)*s);
    }
}

#endif