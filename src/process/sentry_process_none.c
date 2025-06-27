#include "sentry_process.h"

#include "sentry_core.h"

sentry_process_t *
sentry__process_new(const sentry_path_t *UNUSED(executable))
{
    return NULL;
}

void
sentry__process_free(sentry_process_t *UNUSED(process))
{
}

void
sentry__process_set_env(sentry_process_t *UNUSED(process),
    const sentry_pathchar_t *UNUSED(key),
    const sentry_pathchar_t *UNUSED(value), ...)
{
}

bool
sentry__process_spawn(sentry_process_t *UNUSED(process))
{
    return false;
}

bool
sentry__process_spawn_with_args(sentry_process_t *UNUSED(process),
    const sentry_pathchar_t *UNUSED(arg), ...)
{
    return false;
}
