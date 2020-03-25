#ifndef SENTRY_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"

sentry_transport_t *sentry__transport_new_default(void);

void sentry__transport_dump_queue(sentry_transport_t *transport);

#define SENTRY_RL_CATEGORY_ANY 0
#define SENTRY_RL_CATEGORY_ERROR 1
#define SENTRY_RL_CATEGORY_SESSION 2
#define SENTRY_RL_CATEGORY_TRANSACTION 3

struct sentry_rate_limiter_s;
typedef struct sentry_rate_limiter_s sentry_rate_limiter_t;

sentry_rate_limiter_t *sentry__rate_limiter_new(void);
void sentry__rate_limiter_free(sentry_rate_limiter_t *rl);
bool sentry__rate_limiter_update_from_header(
    sentry_rate_limiter_t *rl, const char *sentry_header);
bool sentry__rate_limiter_update_from_http_retry_after(
    sentry_rate_limiter_t *rl, const char *retry_after);
bool sentry__rate_limiter_is_disabled(
    const sentry_rate_limiter_t *rl, int category);

#if SENTRY_UNITTEST
uint64_t sentry__rate_limiter_get_disabled_until(
    const sentry_rate_limiter_t *rl, int category);
#endif

#endif
