#ifndef SENTRY_SLICE_H_INCLUDED
#define SENTRY_SLICE_H_INCLUDED

#include <stddef.h>

typedef struct {
    const char *ptr;
    size_t len;
} sentry_slice_t;

char *sentry__slice_to_owned(sentry_slice_t slice);
int sentry__slice_cmp(sentry_slice_t a, sentry_slice_t b);

#endif
