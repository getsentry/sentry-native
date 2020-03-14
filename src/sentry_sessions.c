#include "sentry_sessions.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_scope.h"
#include "sentry_string.h"
#include "sentry_utils.h"

#include <assert.h>

static const char *
status_as_string(sentry_session_status_t status)
{
    switch (status) {
    case SENTRY_SESSION_STATUS_OK:
        return "ok";
    case SENTRY_SESSION_STATUS_CRASHED:
        return "crashed";
    case SENTRY_SESSION_STATUS_ABNORMAL:
        return "abnormal";
    case SENTRY_SESSION_STATUS_EXITED:
        return "exited";
    default:
        assert(!"should not happen");
    }
}

sentry_session_t *
sentry__session_new(void)
{
    sentry_session_t *rv = SENTRY_MAKE(sentry_session_t);
    rv->session_id = sentry_uuid_new_v4();
    rv->distinct_id = NULL;
    rv->status = SENTRY_SESSION_STATUS_OK;
    rv->init = true;
    rv->errors = 0;
    rv->started_ms = sentry__msec_time();

    return rv;
}

void
sentry__session_free(sentry_session_t *session)
{
    if (!session) {
        return;
    }
    sentry_free(session);
}

void
sentry__session_to_json(
    const sentry_session_t *session, sentry_jsonwriter_t *jw)
{
    const sentry_options_t *opts = sentry_get_options();

    sentry__jsonwriter_write_object_start(jw);
    sentry__jsonwriter_write_key(jw, "sid");
    sentry__jsonwriter_write_uuid(jw, &session->session_id);
    sentry__jsonwriter_write_key(jw, "status");
    sentry__jsonwriter_write_str(jw, status_as_string(session->status));
    if (session->distinct_id) {
        sentry__jsonwriter_write_key(jw, "did");
        sentry__jsonwriter_write_str(jw, session->distinct_id);
    }
    sentry__jsonwriter_write_key(jw, "started");
    sentry__jsonwriter_write_msec_timestamp(jw, session->started_ms);
    sentry__jsonwriter_write_key(jw, "errors");
    sentry__jsonwriter_write_int32(jw, (int32_t)session->errors);

    sentry__jsonwriter_write_key(jw, "duration");
    double duration
        = (double)(sentry__msec_time() - session->started_ms) / 1000.0;
    sentry__jsonwriter_write_double(jw, duration);

    sentry__jsonwriter_write_key(jw, "attrs");
    sentry__jsonwriter_write_object_start(jw);
    sentry__jsonwriter_write_key(jw, "release");
    sentry__jsonwriter_write_str(jw, sentry_options_get_release(opts));
    sentry__jsonwriter_write_key(jw, "environment");
    sentry__jsonwriter_write_str(jw, sentry_options_get_environment(opts));
    sentry__jsonwriter_write_object_end(jw);

    sentry__jsonwriter_write_object_end(jw);
}

void
sentry_start_session(void)
{
    sentry_end_session();
    SENTRY_WITH_SCOPE_MUT (scope) {
        scope->session = sentry__session_new();
    }
}

void
sentry__end_current_session_with_status(sentry_session_status_t status)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        if (scope->session) {
            scope->session->status = status;
        }
    }
    sentry_end_session();
}

void
sentry__record_errors_on_current_session(uint32_t error_count)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        if (scope->session) {
            scope->session->errors += error_count;
        }
    }
}

void
sentry_end_session(void)
{
    sentry_envelope_t *envelope = NULL;

    SENTRY_WITH_SCOPE_MUT (scope) {
        if (scope->session) {
            envelope = sentry__envelope_new();
            sentry__envelope_add_session(envelope, scope->session);
            sentry__session_free(scope->session);
            scope->session = NULL;
        }
    }

    if (envelope) {
        sentry__capture_envelope(envelope);
    }
}

void
sentry__add_current_session_to_envelope(sentry_envelope_t *envelope)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        if (scope->session) {
            sentry__envelope_add_session(envelope, scope->session);
            scope->session->init = false;
        }
    }
}
