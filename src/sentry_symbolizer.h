#ifndef SENTRY_SYMBOLIZER_H_INCLUDED
#define SENTRY_SYMBOLIZER_H_INCLUDED

#include <sentry.h>

typedef struct sentry_frame_info_s {
    void *load_addr;
    void *symbol_addr;
    void *instruction_addr;
    const char *symbol;
    const char *filename;
    const char *object_name;
    uint32_t lineno;
} sentry_frame_info_t;

bool sentry__symbolize(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data);

#endif
