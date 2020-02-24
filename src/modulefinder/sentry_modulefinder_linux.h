#ifndef SENTRY_PROCMAPS_MODULEFINDER_H_INCLUDED
#define SENTRY_PROCMAPS_MODULEFINDER_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_slice.h"

typedef struct {
    void *start;
    void *end;
    sentry_slice_t file;
} sentry_module_t;

int sentry__procmaps_parse_module_line(
    const char *line, sentry_module_t *module);

sentry_value_t sentry__procmaps_modules_get_list(void);

#endif
