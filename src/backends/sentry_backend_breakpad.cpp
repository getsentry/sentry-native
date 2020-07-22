extern "C" {
#include "sentry_boot.h"

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "sentry_unix_pageallocator.h"
}

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wvariadic-macros"
#endif

#ifdef SENTRY_PLATFORM_WINDOWS
#    include "client/windows/handler/exception_handler.h"
#else
#    include "client/linux/handler/exception_handler.h"
#endif

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

typedef struct {
    const sentry_run_t *run;
    sentry_path_t *dump_path;
} breakpad_transport_state_t;

static void
sentry__breakpad_backend_send_envelope(
    sentry_envelope_t *envelope, void *_state)
{
    const breakpad_transport_state_t *state
        = (const breakpad_transport_state_t *)_state;

    if (!state->dump_path) {
        sentry_envelope_free(envelope);
        return;
    }
    // when serializing the envelope to disk, and later sending it as a single
    // `x-sentry-envelope`, the minidump needs to be an attachment, with type
    // `event.minidump`
    sentry_envelope_item_t *item = sentry__envelope_add_from_path(
        envelope, state->dump_path, "attachment");
    if (!item) {
        sentry_envelope_free(envelope);
        return;
    }
    sentry__envelope_item_set_header(
        item, "attachment_type", sentry_value_new_string("event.minidump"));

    sentry__envelope_item_set_header(item, "filename",
#ifdef SENTRY_PLATFORM_WINDOWS
        sentry__value_new_string_from_wstr(
#else
        sentry_value_new_string(
#endif
            sentry__path_filename(state->dump_path)));

    sentry__run_write_envelope(state->run, envelope);
    sentry_envelope_free(envelope);

    // now that the envelope was written, we can remove the temporary
    // minidump file
    sentry__path_remove(state->dump_path);
}

static void
sentry__breakpad_transport_free(void *_state)
{
    breakpad_transport_state_t *state = (breakpad_transport_state_t *)_state;
    sentry__path_free(state->dump_path);
    sentry_free(state);
}

static sentry_transport_t *
sentry__breakpad_transport_new(
    const sentry_options_t *options, sentry_path_t *dump_path)
{
    breakpad_transport_state_t *state = SENTRY_MAKE(breakpad_transport_state_t);
    if (!state) {
        sentry__path_free(dump_path);
        return NULL;
    }
    state->run = options->run;
    state->dump_path = dump_path;

    sentry_transport_t *transport
        = sentry_transport_new(sentry__breakpad_backend_send_envelope);
    if (!transport) {
        sentry__breakpad_transport_free(state);
        return NULL;
    }
    sentry_transport_set_state(transport, state);
    sentry_transport_set_free_func(transport, sentry__breakpad_transport_free);
    return transport;
}

static sentry_transport_t *
sentry__swap_breakpad_transport(
    sentry_options_t *options, sentry_path_t *dump_path)
{
    sentry_transport_t *breakpad_transport
        = sentry__breakpad_transport_new(options, dump_path);
    sentry_transport_t *transport = options->transport;
    options->transport = breakpad_transport;
    return transport;
}

#ifdef SENTRY_PLATFORM_WINDOWS
static bool
sentry__breakpad_backend_callback(const wchar_t *breakpad_dump_path,
    const wchar_t *minidump_id, void *UNUSED(context),
    EXCEPTION_POINTERS *UNUSED(exinfo), MDRawAssertionInfo *UNUSED(assertion),
    bool succeeded)
#else
static bool
sentry__breakpad_backend_callback(
    const google_breakpad::MinidumpDescriptor &descriptor,
    void *UNUSED(context), bool succeeded)
#endif
{
    SENTRY_DEBUG("entering breakpad minidump callback");

    if (!succeeded) {
        SENTRY_WARN("breakpad failed creating minidump");
        return succeeded;
    }

#ifndef SENTRY_PLATFORM_WINDOWS
    sentry__page_allocator_enable();
    sentry__enter_signal_handler();
#endif

    sentry_path_t *dump_path = nullptr;
#ifdef SENTRY_PLATFORM_WINDOWS
    sentry_path_t *tmp_path = sentry__path_new(breakpad_dump_path);
    dump_path = sentry__path_join_wstr(tmp_path, minidump_id);
    sentry__path_free(tmp_path);
    tmp_path = dump_path;
    dump_path = sentry__path_append_str(tmp_path, ".dmp");
    sentry__path_free(tmp_path);
#else
    dump_path = sentry__path_new(descriptor.path());
#endif

    SENTRY_WITH_OPTIONS (options) {
        sentry__write_crash_marker(options);

        // since we can’t use HTTP in crash handlers, we will swap out the
        // transport here to one that serializes the envelope to disk
        sentry_transport_t *transport_original
            = sentry__swap_disk_transport(options);

        // Ending the session will send an envelope, which we *don’t* want to
        // route through the special transport. It will be dumped to disk with
        // the rest of the send queue.
        sentry__end_current_session_with_status(SENTRY_SESSION_STATUS_CRASHED);

        // almost identical to enforcing the disk transport, the breakpad
        // transport will serialize the envelope to disk, but will attach
        // the captured minidump as an additional attachment
        sentry_transport_t *transport_disk
            = sentry__swap_breakpad_transport(options, dump_path);

        // after the transport is set up, we will capture an event, which
        // will create an envelope with all the scope, attachments, etc.
        sentry_value_t event = sentry_value_new_event();
        sentry_capture_event(event);

        // after capturing the crash event, try to dump all the in-flight
        // data of the previous transports
        sentry__transport_dump_queue(options->transport, options->run);
        sentry__transport_dump_queue(transport_disk, options->run);
        sentry__transport_dump_queue(transport_original, options->run);
        // and restore the old transport
        sentry_transport_free(options->transport);
        sentry_transport_free(transport_disk);
        options->transport = transport_original;
        SENTRY_DEBUG("crash has been captured");
    }

#ifndef SENTRY_PLATFORM_WINDOWS
    sentry__leave_signal_handler();
#endif
    return succeeded;
}

static int
sentry__breakpad_backend_startup(
    sentry_backend_t *backend, const sentry_options_t *options)
{
    sentry_path_t *current_run_folder = options->run->run_path;

#ifdef SENTRY_PLATFORM_WINDOWS
    backend->data = new google_breakpad::ExceptionHandler(
        current_run_folder->path, NULL, sentry__breakpad_backend_callback, NULL,
        google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);
#else
    google_breakpad::MinidumpDescriptor descriptor(current_run_folder->path);
    backend->data = new google_breakpad::ExceptionHandler(
        descriptor, NULL, sentry__breakpad_backend_callback, NULL, true, -1);
#endif
    return backend->data == NULL;
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
    sentry_backend_t *backend, const sentry_ucontext_t *context)
{
    google_breakpad::ExceptionHandler *eh
        = (google_breakpad::ExceptionHandler *)backend->data;

#ifdef SENTRY_PLATFORM_WINDOWS
    eh->WriteMinidumpForException(
        const_cast<EXCEPTION_POINTERS *>(&context->exception_ptrs));
#else
    eh->HandleSignal(context->signum, context->siginfo, context->user_context);
#endif
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
