#ifndef SENTRY_PROCESS_H_INCLUDED
#define SENTRY_PROCESS_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

/**
 * Spawns a detached child process with the specified arguments and environment.
 */
bool sentry__process_spawn(
    const sentry_pathchar_t **argv, const sentry_pathchar_t **envp);

#endif
