#include "sentry_slice.h"
#include "sentry_string.h"
#include <string.h>

char *
sentry__slice_to_owned(sentry_slice_t slice)
{
    return sentry__string_clonen(slice.ptr, slice.len);
}

bool
sentry__slice_eq(sentry_slice_t a, sentry_slice_t b)
{
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}
