#include "sentry_database.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include <string.h>

sentry_run_t *
sentry__run_new(const sentry_path_t *database_path)
{
    sentry_run_t *run = SENTRY_MAKE(sentry_run_t);
    if (!run) {
        return NULL;
    }

    run->uuid = sentry_uuid_new_v4();

    char uuid_str[37];
    sentry_uuid_as_string(&run->uuid, uuid_str);
    run->run_path = sentry__path_join_str(database_path, uuid_str);

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
sentry__run_write_envelope(const sentry_run_t *run, sentry_envelope_t *envelope)
{
    // 37 for the uuid, 9 for the `.envelope` suffix
    char envelope_filename[37 + 9];
    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    sentry_uuid_as_string(&event_id, envelope_filename);
    strncpy(&envelope_filename[36], ".envelope", 10);

    sentry_path_t *output_path
        = sentry__path_join_str(run->run_path, envelope_filename);

    int rv = sentry_envelope_write_to_path(envelope, output_path);
    sentry__path_free(output_path);

    if (rv) {
        SENTRY_DEBUG("writing envelope to file failed");
    }

    // the `write_to_path` returns > 0 on failure, but we would like a real bool
    return !rv;
}

void
sentry__enqueue_unsent_envelopes(const sentry_options_t *options)
{
    sentry_pathiter_t *db_iter
        = sentry__path_iter_directory(options->database_path);
    const sentry_path_t *run_dir;
    while ((run_dir = sentry__pathiter_next(db_iter)) != NULL) {
        sentry_pathiter_t *run_iter = sentry__path_iter_directory(run_dir);
        const sentry_path_t *envelope_file;
        while ((envelope_file = sentry__pathiter_next(run_iter)) != NULL) {

            if (options->transport) {
                sentry_envelope_t *envelope
                    = sentry__envelope_from_disk(envelope_file);
                options->transport->send_envelope_func(
                    options->transport, envelope);
            }

            sentry__path_remove_all(envelope_file);
        }
        sentry__path_remove_all(run_dir);
    }
}
