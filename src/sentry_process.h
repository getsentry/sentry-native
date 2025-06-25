#ifndef SENTRY_PROCESS_H_INCLUDED
#define SENTRY_PROCESS_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

/**
 * Spawns a detached child process that runs the given executable.
 * Command line arguments can be passed as a variable argument list.
 * The arguments must of of type `const wchar_t *` on Windows or
 * `const char *` on POSIX, and must be terminated with `NULL`.
 */
bool sentry__spawn_process(const sentry_path_t *executable, ...);

#endif
