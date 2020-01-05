#ifndef SENTRY_UNIX_SYMBOLIZER_DLADDR_H_INCLUDED
#define SENTRY_UNIX_SYMBOLIZER_DLADDR_H_INCLUDED

#include "../sentry_boot.h"

#include "../sentry_symbolizer.h"

bool
sentry__symbolize_dladdr(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data);

#endif
