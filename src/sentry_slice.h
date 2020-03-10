#ifndef SENTRY_SLICE_H_INCLUDED
#define SENTRY_SLICE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *ptr;
    size_t len;
} sentry_slice_t;

char *sentry__slice_to_owned(sentry_slice_t slice);
bool sentry__slice_eq(sentry_slice_t a, sentry_slice_t b);

#endif
