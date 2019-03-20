#ifndef SENTRY_MACROS_HPP_INCLUDED
#define SENTRY_MACROS_HPP_INCLUDED

#include <stdio.h>
#include <sys/errno.h>
#include "internal.hpp"

#define SENTRY_PRINT(std, message)         \
    do {                                   \
        if (sentry_get_options()->debug) { \
            fprintf(stderr, message);      \
        }                                  \
    } while (0)

#define SENTRY_PRINT_ARGS(std, message, args) \
    do {                                      \
        if (sentry_get_options()->debug) {    \
            fprintf(stderr, message, args);   \
        }                                     \
    } while (0)

#define SENTRY_PRINT_DEBUG(message) SENTRY_PRINT(stdout, message)

#define SENTRY_PRINT_DEBUG_ARGS(message, args) \
    SENTRY_PRINT_ARGS(stdout, message, args)

#define SENTRY_PRINT_ERROR(message) SENTRY_PRINT(stderr, message)

#define SENTRY_PRINT_ERROR_ARGS(message, args) \
    SENTRY_PRINT_ARGS(stderr, message, args)

#define EINTR_RETRY(X)                         \
    ({                                         \
        int _rv;                               \
        do {                                   \
            _rv = (X);                         \
        } while (_rv == -1 && errno == EINTR); \
        _rv;                                   \
    })
#endif
