#ifndef SENTRY_PROCESS_H_INCLUDED
#define SENTRY_PROCESS_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

bool sentry__process_spawn(
    const sentry_path_t *executable, const sentry_pathchar_t *arg0, ...);

#endif
