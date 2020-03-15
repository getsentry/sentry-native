#ifndef SENTRY_SLICE_H_INCLUDED
#define SENTRY_SLICE_H_INCLUDED

#include "sentry_boot.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *ptr;
    size_t len;
} sentry_slice_t;

sentry_slice_t sentry__slice_from_str(const char *str);
char *sentry__slice_to_owned(sentry_slice_t slice);
bool sentry__slice_eq(sentry_slice_t a, sentry_slice_t b);

static inline bool
sentry__slice_eqs(sentry_slice_t a, const char *str)
{
    return sentry__slice_eq(a, sentry__slice_from_str(str));
}

sentry_slice_t sentry__slice_split_at(sentry_slice_t a, char c);
size_t sentry__slice_find(sentry_slice_t a, char c);
sentry_slice_t sentry__slice_trim(sentry_slice_t a);

static inline bool
sentry__slice_pop_front_if(sentry_slice_t *a, char c)
{
    if (a->len > 0 && a->ptr[0] == c) {
        a->ptr++;
        a->len--;
        return true;
    } else {
        return false;
    }
}

bool sentry__slice_pop_uint64(sentry_slice_t *a, uint64_t *num_out);

static inline sentry_slice_t
sentry__slice_advance(sentry_slice_t s, size_t bytes)
{
    s.ptr += bytes;
    s.len -= bytes;
    return s;
}

#endif
