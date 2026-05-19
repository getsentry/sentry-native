#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_transport.h"

#include <errno.h>
#include <sentry.h>
#include <stdio.h>
#include <string.h>

#define ENSURE(Expr)                                                           \
    if (!(Expr))                                                               \
    return

static int
android_transport_startup(const sentry_options_t *options, void *data)
{
    char **outbox_path = data;
    if (!outbox_path || !options->run || !options->run->run_path) {
        return 1;
    }

    sentry_path_t *database_path = sentry__path_dir(options->run->run_path);
    if (!database_path) {
        return 1;
    }
    sentry_path_t *files_dir = sentry__path_dir(database_path);
    sentry__path_free(database_path);
    if (!files_dir) {
        return 1;
    }

    sentry_path_t *outbox_dir = sentry__path_join_str(files_dir, "outbox");
    sentry__path_free(files_dir);
    if (!outbox_dir) {
        return 1;
    }

    if (sentry__path_create_dir_all(outbox_dir) != 0) {
        SENTRY_WARNF("android transport startup: mkdir failed for %s: %s",
            outbox_dir->path, strerror(errno));
        sentry__path_free(outbox_dir);
        return 1;
    }

    char *outbox_path_str = sentry_malloc(strlen(outbox_dir->path) + 1);
    if (!outbox_path_str) {
        sentry__path_free(outbox_dir);
        return 1;
    }
    strcpy(outbox_path_str, outbox_dir->path);
    sentry__path_free(outbox_dir);

    sentry_free(*outbox_path);
    *outbox_path = outbox_path_str;
    return 0;
}

static void
send_envelope(sentry_envelope_t *envelope, void *data)
{
    const char *outbox_path = (const char *)data;
    char envelope_id_str[40];

    sentry_uuid_t envelope_id = sentry_uuid_new_v4();
    sentry_uuid_as_string(&envelope_id, envelope_id_str);

    size_t outbox_len = strlen(outbox_path);
    size_t final_len = outbox_len + 42; // "/" + envelope_id_str + "\0" = 42
    char *envelope_path = sentry_malloc(final_len);
    ENSURE(envelope_path);
    int written = snprintf(
        envelope_path, final_len, "%s/%s", outbox_path, envelope_id_str);
    if (written > outbox_len && written < final_len) {
        sentry_envelope_write_to_file(envelope, envelope_path);
    }

    sentry_free(envelope_path);
    sentry_envelope_free(envelope);
}

static void
android_transport_send_envelope(sentry_envelope_t *envelope, void *data)
{
    char **outbox_path = data;
    if (!outbox_path || !*outbox_path) {
        SENTRY_WARN(
            "android transport send_envelope: outbox path not initialized");
        sentry_envelope_free(envelope);
        return;
    }
    send_envelope(envelope, *outbox_path);
}

static void
android_transport_free(void *data)
{
    char **outbox_path = data;
    if (!outbox_path) {
        return;
    }
    sentry_free(*outbox_path);
    sentry_free(outbox_path);
}

sentry_transport_t *
sentry__transport_new_default(void)
{
    char **outbox_path = sentry__calloc(1, sizeof(*outbox_path));
    if (!outbox_path) {
        return NULL;
    }

    sentry_transport_t *transport
        = sentry_transport_new(android_transport_send_envelope);
    if (!transport) {
        sentry_free(outbox_path);
        return NULL;
    }

    sentry_transport_set_state(transport, outbox_path);
    sentry_transport_set_free_func(transport, android_transport_free);
    sentry_transport_set_startup_func(transport, android_transport_startup);
    return transport;
}
