#ifndef SENTRY_TEST_UTIL_H_INCLUDED
#define SENTRY_TEST_UTIL_H_INCLUDED

#include "sentry.h"

#ifdef __cplusplus
extern "C" {
#endif

SENTRY_API void sentry_trigger_crash(void);

#ifdef __cplusplus
}
#endif
#endif //SENTRY_TEST_UTIL_H_INCLUDED
