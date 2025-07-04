#include "sentry_process.h"

#include "sentry_core.h"

bool
sentry__process_spawn(const sentry_path_t *UNUSED(executable),
    const sentry_pathchar_t *UNUSED(arg0), ...)
{
    return false;
}
