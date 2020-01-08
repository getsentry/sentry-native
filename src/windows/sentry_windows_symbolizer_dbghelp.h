#ifndef SENTRY_WINDOWS_SYMBOLIZER_DBGHELP_H_INCLUDED
#define SENTRY_WINDOWS_SYMBOLIZER_DBGHELP_H_INCLUDED

#include "../sentry_boot.h"

#include "../sentry_symbolizer.h"

bool sentry__symbolize_dbghelp(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data);

#endif
