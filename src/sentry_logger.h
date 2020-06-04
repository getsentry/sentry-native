#include "sentry.h"

void sentry__logger_defaultlogger(
    sentry_level_t level, const char *message, va_list args);

const char *sentry__logger_describe(sentry_level_t level);

void sentry__logger_log(sentry_level_t level, const char *message, ...);

#define SENTRY_TRACEF(message, ...)                                            \
    sentry__logger_log(SENTRY_LEVEL_DEBUG, message "\n", __VA_ARGS__)

#define SENTRY_TRACE(message)                                                  \
    sentry__logger_log(SENTRY_LEVEL_DEBUG, message "\n")

#define SENTRY_DEBUGF(message, ...)                                            \
    sentry__logger_log(SENTRY_LEVEL_INFO, message "\n", __VA_ARGS__)

#define SENTRY_DEBUG(message)                                                  \
    sentry__logger_log(SENTRY_LEVEL_INFO, message "\n")

#define SENTRY_WARNF(message, ...)                                             \
    sentry__logger_log(SENTRY_LEVEL_WARNING, message "\n", __VA_ARGS__)

#define SENTRY_WARN(message)                                                   \
    sentry__logger_log(SENTRY_LEVEL_WARNING, message "\n")
