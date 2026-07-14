#ifndef SENTRY_FTRACE_H_INCLUDED
#define SENTRY_FTRACE_H_INCLUDED

#include "sentry_boot.h"

#ifdef SENTRY_FTRACE

#    if defined(__GNUC__) || defined(__clang__)
#        define SENTRY_FTRACE_PRINTF(FormatIndex, FirstArg)                    \
            __attribute__((format(printf, FormatIndex, FirstArg)))
#    else
#        define SENTRY_FTRACE_PRINTF(FormatIndex, FirstArg)
#    endif

void sentry__ftrace_begin(const char *format, ...) SENTRY_FTRACE_PRINTF(1, 2);
void sentry__ftrace_end(void);
void sentry__ftrace_instant(const char *format, ...) SENTRY_FTRACE_PRINTF(1, 2);
void sentry__ftrace_counter(int64_t value, const char *format, ...)
    SENTRY_FTRACE_PRINTF(2, 3);

/**
 * Example usage:
 *
 *     SENTRY_FTRACE_BEGIN("request.%s", route);
 *     SENTRY_FTRACE_END();
 *     SENTRY_FTRACE_INSTANT("cache.miss");
 *     SENTRY_FTRACE_COUNTER(queue_size, "queue.%s", queue_name);
 */
#    define SENTRY_FTRACE_BEGIN(...) sentry__ftrace_begin(__VA_ARGS__)
#    define SENTRY_FTRACE_END() sentry__ftrace_end()
#    define SENTRY_FTRACE_INSTANT(...) sentry__ftrace_instant(__VA_ARGS__)
#    define SENTRY_FTRACE_COUNTER(Value, ...)                                  \
        sentry__ftrace_counter((int64_t)(Value), __VA_ARGS__)

#else

#    define SENTRY_FTRACE_BEGIN(...) ((void)0)
#    define SENTRY_FTRACE_END() ((void)0)
#    define SENTRY_FTRACE_INSTANT(...) ((void)0)
#    define SENTRY_FTRACE_COUNTER(Value, ...) ((void)0)

#endif

#endif
