#ifndef SENTRY_INTERNAL_HPP_INCLUDED
#define SENTRY_INTERNAL_HPP_INCLUDED
#include <sentry.h>

const sentry_options_t *sentry__get_options(void);

static const char *SENTRY_EVENT_FILE_NAME = "sentry-event.mp";

#endif