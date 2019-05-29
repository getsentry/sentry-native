#ifndef SENTRY_INTERNAL_HPP_INCLUDED
#define SENTRY_INTERNAL_HPP_INCLUDED
#include <errno.h>
#include <sentry.h>
#include <stdio.h>

#define SENTRY_PRINT_ARGS(message, ...)                             \
    do {                                                            \
        if (sentry_options_get_debug(sentry_get_options())) {       \
            fprintf(stderr, "[sentry] " message "\n", __VA_ARGS__); \
        }                                                           \
    } while (0)

#define SENTRY_PRINT_DEBUG(Message) SENTRY_PRINT_ARGS("%s", Message "")
#define SENTRY_PRINT_DEBUG_ARGS(Message, ...) \
    SENTRY_PRINT_ARGS(Message, __VA_ARGS__)
#define SENTRY_PRINT_ERROR(Message) SENTRY_PRINT_ARGS("%s", Message "")
#define SENTRY_PRINT_ERROR_ARGS(Message, ...) \
    SENTRY_PRINT_ARGS(Message, __VA_ARGS__)

#define EINTR_RETRY(X)                         \
    {                                          \
        int _rv;                               \
        do {                                   \
            _rv = (X);                         \
        } while (_rv == -1 && errno == EINTR); \
        _rv;                                   \
    }

#define SENTRY_BREADCRUMBS_MAX 100
static const char *SENTRY_EVENT_FILE_NAME = "sentry-event.mp";
static const char *SENTRY_BREADCRUMB1_FILE = "sentry-breadcrumb1.mp";
static const char *SENTRY_BREADCRUMB2_FILE = "sentry-breadcrumb2.mp";
static const char *SENTRY_EVENT_FILE_ATTACHMENT_NAME = "__sentry-event";
static const char *SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME =
    "__sentry-breadcrumb1";
static const char *SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME =
    "__sentry-breadcrumb2";

#endif
