#ifndef SENTRY_PROCESS_H_INCLUDED
#define SENTRY_PROCESS_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

struct sentry_process_s;
typedef struct sentry_process_s sentry_process_t;

sentry_process_t *sentry__process_new(const sentry_path_t *executable);
void sentry__process_free(sentry_process_t *process);

void sentry__process_set_env(sentry_process_t *process,
    const sentry_pathchar_t *key, const sentry_pathchar_t *value, ...);

bool sentry__process_spawn(sentry_process_t *process);
bool sentry__process_spawn_with_args(
    sentry_process_t *process, const sentry_pathchar_t *arg0, ...);

#endif
