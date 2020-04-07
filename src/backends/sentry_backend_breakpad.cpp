extern "C" {
#include "sentry_boot.h"

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_path.h"
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "sentry_unix_pageallocator.h"
}

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wvariadic-macros"
#endif

#include "client/linux/handler/exception_handler.h"

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

typedef struct {
    sentry_run_t *run;
    const char *dump_path;
} breakpad_transport_state_t;

static void
sentry__breakpad_backend_send_envelope(
    sentry_transport_t *transport, sentry_envelope_t *envelope)
{
    const breakpad_transport_state_t *state
        = (const breakpad_transport_state_t *)transport->data;

    sentry_path_t *dump_path = sentry__path_from_str(state->dump_path);
    if (!dump_path) {
        sentry_envelope_free(envelope);
        return;
    }
    // when serializing the envelope to disk, and later sending it as a single
    // `x-sentry-envelope`, the minidump needs to be a regular attachment,
    // with special headers:
    // `name: upload_file_minidump; attachment_type: event.minidump`
    sentry_envelope_item_t *item
        = sentry__envelope_add_from_path(envelope, dump_path, "attachment");
    if (!item) {
        sentry__path_free(dump_path);
        sentry_envelope_free(envelope);
        return;
    }
    sentry__envelope_item_set_header(
        item, "name", sentry_value_new_string("upload_file_minidump"));
    sentry__envelope_item_set_header(
        item, "attachment_type", sentry_value_new_string("event.minidump"));
    sentry__envelope_item_set_header(item, "filename",
        sentry_value_new_string(sentry__path_filename(dump_path)));

    sentry__run_write_envelope(state->run, envelope);
    sentry_envelope_free(envelope);

    // now that the envelope was written, we can remove the temporary
    // minidump file
    sentry__path_remove(dump_path);
    sentry__path_free(dump_path);
}

static void
sentry__enforce_breakpad_transport(
    const sentry_options_t *options, const char *dump_path)
{
    sentry_transport_t *transport = SENTRY_MAKE(sentry_transport_t);
    if (!transport) {
        return;
    }
    breakpad_transport_state_t *state = SENTRY_MAKE(breakpad_transport_state_t);
    if (!state) {
        sentry_free(transport);
        return;
    }

    state->run = options->run;
    state->dump_path = dump_path;

    transport->data = state;
    transport->free_func = NULL;
    transport->send_envelope_func = sentry__breakpad_backend_send_envelope;
    transport->startup_func = NULL;
    transport->shutdown_func = NULL;

    ((sentry_options_t *)options)->transport = transport;
}

static bool
sentry__breakpad_backend_callback(
    const google_breakpad::MinidumpDescriptor &descriptor,
    void *UNUSED(context), bool succeeded)
{
    sentry__page_allocator_enable();
    sentry__enter_signal_handler();

    const sentry_options_t *options = sentry_get_options();
    sentry__write_crash_marker(options);
    const char *dump_path = descriptor.path();

    // almost identical to enforcing the disk transport, the breakpad
    // transport will serialize the envelope to disk, but will attach the
    // captured minidump as an additional attachment
    sentry_transport_t *transport = options->transport;
    sentry__enforce_breakpad_transport(options, dump_path);

    // after the transport is set up, we will capture an event, which will
    // create an envelope with all the scope, attachments, etc.
    sentry_value_t event = sentry_value_new_event();
    sentry__end_current_session_with_status(SENTRY_SESSION_STATUS_CRASHED);
    sentry_capture_event(event);

    // after capturing the crash event, try to dump all the in-flight data of
    // the previous transport
    if (transport) {
        sentry__transport_dump_queue(transport);
    }
    SENTRY_DEBUG("crash has been captured");

    sentry__leave_signal_handler();
    return succeeded;
}

static void
sentry__breakpad_backend_startup(sentry_backend_t *backend)
{
    const sentry_options_t *options = sentry_get_options();
    sentry_path_t *current_run_folder = options->run->run_path;

    google_breakpad::MinidumpDescriptor descriptor(current_run_folder->path);
    backend->data = new google_breakpad::ExceptionHandler(
        descriptor, NULL, sentry__breakpad_backend_callback, NULL, true, -1);
}

static void
sentry__breakpad_backend_shutdown(sentry_backend_t *backend)
{
    google_breakpad::ExceptionHandler *eh
        = (google_breakpad::ExceptionHandler *)backend->data;
    backend->data = NULL;
    delete eh;
}

static void
sentry__breakpad_backend_except(
    sentry_backend_t *backend, sentry_ucontext_t *context)
{
    google_breakpad::ExceptionHandler *eh
        = (google_breakpad::ExceptionHandler *)backend->data;
    eh->HandleSignal(context->signum, context->siginfo, context->user_context);
}

extern "C" {

sentry_backend_t *
sentry__backend_new(void)
{
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    if (!backend) {
        return NULL;
    }

    backend->startup_func = sentry__breakpad_backend_startup;
    backend->shutdown_func = sentry__breakpad_backend_shutdown;
    backend->except_func = sentry__breakpad_backend_except;
    backend->free_func = NULL;
    backend->flush_scope_func = NULL;
    backend->add_breadcrumb_func = NULL;
    backend->user_consent_changed_func = NULL;
    backend->data = NULL;

    return backend;
}
}
