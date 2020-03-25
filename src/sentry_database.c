#include "sentry_database.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_session.h"
#include <string.h>

sentry_run_t *
sentry__run_new(const sentry_path_t *database_path)
{
    sentry_uuid_t uuid = sentry_uuid_new_v4();
    char uuid_str[37];
    sentry_uuid_as_string(&uuid, uuid_str);
    sentry_path_t *run_path = sentry__path_join_str(database_path, uuid_str);
    if (!run_path) {
        return NULL;
    }
    sentry_path_t *session_path
        = sentry__path_join_str(run_path, "session.json");
    if (!session_path) {
        sentry__path_free(run_path);
        return NULL;
    }

    sentry_run_t *run = SENTRY_MAKE(sentry_run_t);
    if (!run) {
        sentry__path_free(run_path);
        sentry__path_free(session_path);
        return NULL;
    }

    run->uuid = uuid;
    run->run_path = run_path;
    run->session_path = session_path;

    sentry__path_create_dir_all(run->run_path);

    return run;
}

void
sentry__run_free(sentry_run_t *run)
{
    if (!run) {
        return;
    }
    sentry__path_free(run->run_path);
    sentry_free(run);
}

bool
sentry__run_write_envelope(
    const sentry_run_t *run, const sentry_envelope_t *envelope)
{
    // 37 for the uuid, 9 for the `.envelope` suffix
    char envelope_filename[37 + 9];
    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    sentry_uuid_as_string(&event_id, envelope_filename);
    strcpy(&envelope_filename[36], ".envelope");

    sentry_path_t *output_path
        = sentry__path_join_str(run->run_path, envelope_filename);
    if (!output_path) {
        return false;
    }

    int rv = sentry_envelope_write_to_path(envelope, output_path);
    sentry__path_free(output_path);

    if (rv) {
        SENTRY_DEBUG("writing envelope to file failed");
    }

    // the `write_to_path` returns > 0 on failure, but we would like a real bool
    return !rv;
}

bool
sentry__run_write_session(
    const sentry_run_t *run, const sentry_session_t *session)
{
    sentry_jsonwriter_t *jw = sentry__jsonwriter_new_in_memory();
    if (!jw) {
        return false;
    }
    sentry__session_to_json(session, jw);
    size_t buf_len;
    char *buf = sentry__jsonwriter_into_string(jw, &buf_len);
    if (!buf) {
        return false;
    }

    int rv = sentry__path_write_buffer(run->session_path, buf, buf_len);
    sentry_free(buf);

    if (rv) {
        SENTRY_DEBUG("writing session to file failed");
    }
    return !rv;
}

bool
sentry__run_clear_session(const sentry_run_t *run)
{
    int rv = sentry__path_remove(run->session_path);
    return !rv;
}

void
sentry__process_old_runs(const sentry_options_t *options)
{
    sentry_pathiter_t *db_iter
        = sentry__path_iter_directory(options->database_path);
    if (!db_iter) {
        return;
    }
    const sentry_path_t *run_dir;
    sentry_envelope_t *session_envelope = NULL;
    size_t session_num = 0;

    while ((run_dir = sentry__pathiter_next(db_iter)) != NULL) {
        sentry_pathiter_t *run_iter = sentry__path_iter_directory(run_dir);
        const sentry_path_t *file;
        while ((file = sentry__pathiter_next(run_iter)) != NULL) {
            if (options->transport) {
                if (sentry__path_filename_matches(file, "session.json")) {
                    if (!session_envelope) {
                        session_envelope = sentry__envelope_new();
                    }
                    sentry_session_t *session = sentry__session_from_path(file);
                    if (session->status == SENTRY_SESSION_STATUS_OK) {
                        session->status = SENTRY_SESSION_STATUS_ABNORMAL;
                    }
                    sentry__envelope_add_session(session_envelope, session);
                    sentry__session_free(session);
                    if ((++session_num) >= SENTRY_MAX_ENVELOPE_ITEMS) {
                        sentry__capture_envelope(session_envelope);
                        session_envelope = NULL;
                        session_num = 0;
                    }
                } else if (sentry__path_ends_with(file, ".envelope")) {
                    sentry_envelope_t *envelope
                        = sentry__envelope_from_path(file);
                    if (envelope) {
                        sentry__capture_envelope(envelope);
                    }
                }
            }

            sentry__path_remove_all(file);
        }
        sentry__pathiter_free(run_iter);
        sentry__path_remove_all(run_dir);
    }

    if (session_envelope) {
        sentry__capture_envelope(session_envelope);
    }

    sentry__pathiter_free(db_iter);
}
