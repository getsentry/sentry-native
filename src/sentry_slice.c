#include "sentry_slice.h"
#include "sentry_string.h"
#include <string.h>

char *
sentry__slice_to_owned(sentry_slice_t slice)
{
    return sentry__string_clonen(slice.ptr, slice.len);
}

int
sentry__slice_cmp(sentry_slice_t a, sentry_slice_t b)
{
    return strncmp(a.ptr, b.ptr, a.len < b.len ? a.len : b.len);
}
