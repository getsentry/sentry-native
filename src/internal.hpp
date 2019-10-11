#ifndef SENTRY_INTERNAL_HPP_INCLUDED
#define SENTRY_INTERNAL_HPP_INCLUDED

// this marker changes the behavior of the header so that some internals
// can be defined differently
#define _SENTRY_INTERNAL

#include <errno.h>
#include <sentry.h>
#include <stdio.h>

#define SENTRY_SDK_NAME "sentry-native"
#define SENTRY_SDK_VERSION "0.1.1"
#define SENTRY_SDK_USER_AGENT (SENTRY_SDK_NAME "/" SENTRY_SDK_VERSION)

#define SENTRY_LOGF(message, ...)                                   \
    do {                                                            \
        const sentry_options_t *_options = sentry_get_options();    \
        if (_options && sentry_options_get_debug(_options)) {       \
            fprintf(stderr, "[sentry] " message "\n", __VA_ARGS__); \
        }                                                           \
    } while (0)

#define SENTRY_LOG(Message) SENTRY_LOGF("%s", Message "")

#ifdef _WIN32
#define EINTR_RETRY(X, Y)     \
    do {                      \
        int _tmp = (X);       \
        if (Y) {              \
            *(int *)Y = _tmp; \
        }                     \
    } while (false)
#else
#define EINTR_RETRY(X, Y)                       \
    do {                                        \
        int _tmp;                               \
        do {                                    \
            _tmp = (X);                         \
        } while (_tmp == -1 && errno == EINTR); \
        if (Y) {                                \
            *(int *)Y = _tmp;                   \
        }                                       \
    } while (false)
#endif

#define SENTRY_BREADCRUMBS_MAX 100
static const char *SENTRY_RUNS_FOLDER = "sentry-runs";
static const char *SENTRY_EVENT_FILE_NAME = "sentry-event.mp";
static const char *SENTRY_BREADCRUMB1_FILE = "sentry-breadcrumb1.mp";
static const char *SENTRY_BREADCRUMB2_FILE = "sentry-breadcrumb2.mp";
static const char *SENTRY_EVENT_FILE_ATTACHMENT_NAME = "__sentry-event";
static const char *SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME =
    "__sentry-breadcrumb1";
static const char *SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME =
    "__sentry-breadcrumb2";

#include "value.hpp"

#endif
