#ifndef SENTRY_FEEDBACK_H_INCLUDED
#define SENTRY_FEEDBACK_H_INCLUDED

#include "sentry_boot.h"

/**
 * A sentry Feedback Hint used to pass additional data along with a feedback
 * when it’s being captured.
 */
struct sentry_feedback_hint_s {
    sentry_attachment_t *attachments;
};

#endif
