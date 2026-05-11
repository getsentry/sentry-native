#ifndef SENTRY_WER_REPORT_H_INCLUDED
#define SENTRY_WER_REPORT_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_value.h"

#define SENTRY_WER_EVENT_ID_KEY "SentryEventId"
#define SENTRY_WER_EVENT_ID_KEY_W L"SentryEventId"

sentry_value_t sentry__wer_report_new(const char *event_id);
sentry_value_t sentry__wer_report_from_buffer(const char *buffer, size_t size);

#endif
