#ifndef SENTRY_PROCMAPS_MODULEFINDER_H_INCLUDED
#define SENTRY_PROCMAPS_MODULEFINDER_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_slice.h"

typedef struct {
    void *start;
    void *end;
    sentry_slice_t file;
} sentry_module_t;

#if SENTRY_UNITTEST
sentry_value_t sentry__procmaps_module_to_value(const sentry_module_t *module);

int sentry__procmaps_parse_module_line(
    const char *line, sentry_module_t *module);
#endif

#endif
