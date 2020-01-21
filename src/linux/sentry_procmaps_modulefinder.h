#ifndef SENTRY_PROCMAPS_MODULEFINDER_H_INCLUDED
#define SENTRY_PROCMAPS_MODULEFINDER_H_INCLUDED

#include "../sentry_boot.h"

struct sentry_module_s {
    uint64_t start;
    uint64_t end;
    char *file;
};
typedef struct sentry_module_s sentry_module_t;

int sentry__procmaps_parse_module_line(char *line, sentry_module_t *module);

sentry_value_t sentry__procmaps_modules_get_list(void);

#endif
