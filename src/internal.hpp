#ifndef SENTRY_INTERNAL_HPP_INCLUDED
#define SENTRY_INTERNAL_HPP_INCLUDED
#include <sentry.h>

const sentry_options_t *sentry__get_options(void);

static const int BREADCRUMB_MAX = 100;
static const int ATTACHMENTS_MAX = 100;

static const char *SENTRY_EVENT_FILE_NAME = "sentry-event.mp";
// Refs are used to keep track of current breadcrumb file
static char *BREADCRUMB_FILE_1 = "sentry-breadcrumb1.mp";
static char *BREADCRUMB_FILE_2 = "sentry-breadcrumb2.mp";
// Names used when uploading to Sentry
static const char *SENTRY_EVENT_FILE_ATTACHMENT_NAME = "__sentry-event";
static const char *SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME =
    "__sentry-breadcrumb1";
static const char *SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME =
    "__sentry-breadcrumb2";
#endif