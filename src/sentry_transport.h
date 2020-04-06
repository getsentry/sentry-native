#ifndef SENTRY_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"

/**
 * This will create a new platform specific HTTP transport.
 */
sentry_transport_t *sentry__transport_new_default(void);

/**
 * This function will instruct the platform specific transport to dump all the
 * envelopes in its send queue to disk.
 */
void sentry__transport_dump_queue(sentry_transport_t *transport);

#define SENTRY_RL_CATEGORY_ANY 0
#define SENTRY_RL_CATEGORY_ERROR 1
#define SENTRY_RL_CATEGORY_SESSION 2
#define SENTRY_RL_CATEGORY_TRANSACTION 3

struct sentry_rate_limiter_s;
typedef struct sentry_rate_limiter_s sentry_rate_limiter_t;

/**
 * This will create a new rate limiter.
 */
sentry_rate_limiter_t *sentry__rate_limiter_new(void);

/**
 * Free a previously allocated rate limiter.
 */
void sentry__rate_limiter_free(sentry_rate_limiter_t *rl);

/**
 * This will update the rate limiters internal state based on the
 * `X-Sentry-Rate-Limits` header.
 */
bool sentry__rate_limiter_update_from_header(
    sentry_rate_limiter_t *rl, const char *sentry_header);

/**
 * This will update the rate limiters internal state based on the `Retry-After`
 * header.
 */
bool sentry__rate_limiter_update_from_http_retry_after(
    sentry_rate_limiter_t *rl, const char *retry_after);

/**
 * This will return `true` if the specified `category` is currently rate
 * limited.
 */
bool sentry__rate_limiter_is_disabled(
    const sentry_rate_limiter_t *rl, int category);

#if SENTRY_UNITTEST
/**
 * The rate limiters state is completely opaque. Unless in tests, where we would
 * want to actually peek into the specific rate limiting `category`.
 */
uint64_t sentry__rate_limiter_get_disabled_until(
    const sentry_rate_limiter_t *rl, int category);
#endif

#endif
