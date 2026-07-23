#ifndef SENTRY_INTEGRATION_QT_H_INCLUDED
#define SENTRY_INTEGRATION_QT_H_INCLUDED

#include "sentry_integration.h"

#ifdef __cplusplus
#    define C_API extern "C"
#else
#    define C_API
#endif

/**
 * This creates the Qt integration.
 */
C_API sentry_integration_t *sentry_integration_qt_new(void);

#endif
