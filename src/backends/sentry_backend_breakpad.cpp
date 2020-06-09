extern "C" {
#include "sentry_boot.h"

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_transport.h"
#include "sentry_unix_pageallocator.h"
#ifdef SENTRY_PLATFORM_UNIX
#include "sentry_sync.h"
#endif
}

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wvariadic-macros"
#endif

#if defined SENTRY_PLATFORM_LINUX
#include "client/linux/handler/exception_handler.h"
#elif defined SENTRY_PLATFORM_WINDOWS
#include "client/windows/handler/exception_handler.h"
#endif

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

typedef struct {
    sentry_run_t *run;
    const sentry_pathchar_t *dump_path;
} breakpad_transport_state_t;

static void
sentry__breakpad_backend_send_envelope(
    sentry_envelope_t *envelope, void *_state)
{
    const breakpad_transport_state_t *state
        = (const breakpad_transport_state_t *)_state;

    sentry_path_t *dump_path = sentry__path_new((sentry_pathchar_t*)state->dump_path);
    if (!dump_path) {
        sentry_envelope_free(envelope);
        return;
    }
    // when serializing the envelope to disk, and later sending it as a single
    // `x-sentry-envelope`, the minidump needs to be an attachment, with type
    // `event.minidump`
    sentry_envelope_item_t *item
        = sentry__envelope_add_from_path(envelope, dump_path, "attachment");
    if (!item) {
        sentry__path_free(dump_path);
        sentry_envelope_free(envelope);
        return;
    }
    sentry__envelope_item_set_header(
        item, "attachment_type", sentry_value_new_string("event.minidump"));
    sentry__envelope_item_set_header(item, "filename",
#ifdef SENTRY_PLATFORM_WINDOWS
        sentry__value_new_string_from_wstr(sentry__path_filename(dump_path)));
#else
        sentry_value_new_string(sentry__path_filename(dump_path)));
#endif

    sentry__run_write_envelope(state->run, envelope);
    sentry_envelope_free(envelope);

    // now that the envelope was written, we can remove the temporary
    // minidump file
    sentry__path_remove(dump_path);
    sentry__path_free(dump_path);
}

static void
sentry__enforce_breakpad_transport(
    const sentry_options_t *options, const sentry_pathchar_t *dump_path)
{
    breakpad_transport_state_t *state = SENTRY_MAKE(breakpad_transport_state_t);
    if (!state) {
        return;
    }
    state->run = options->run;
    state->dump_path = dump_path;

    sentry_transport_t *transport
        = sentry_transport_new(sentry__breakpad_backend_send_envelope);
    if (!transport) {
        sentry_free(state);
        return;
    }
    sentry_transport_set_state(transport, state);
    sentry_transport_set_free_func(transport, sentry_free);

    ((sentry_options_t *)options)->transport = transport;
}

static bool
sentry__breakpad_backend_common(
    const sentry_pathchar_t* dump_path,
    void *UNUSED(context), bool succeeded)
{
    const sentry_options_t *options = sentry_get_options();
    sentry__write_crash_marker(options);

    // Ending the session will send an envelope, which we *don’t* want to route
    // through the special transport. It will be dumped to disk with the rest of
    // the send queue.
    sentry__end_current_session_with_status(SENTRY_SESSION_STATUS_CRASHED);

    // almost identical to enforcing the disk transport, the breakpad
    // transport will serialize the envelope to disk, but will attach the
    // captured minidump as an additional attachment
    sentry_transport_t *transport = options->transport;
    sentry__enforce_breakpad_transport(options, dump_path);

    // after the transport is set up, we will capture an event, which will
    // create an envelope with all the scope, attachments, etc.
    sentry_value_t event = sentry_value_new_event();
    sentry_capture_event(event);

    // after capturing the crash event, try to dump all the in-flight data of
    // the previous transport
    if (transport) {
        sentry__transport_dump_queue(transport);
    }
    SENTRY_DEBUG("crash has been captured");

    return succeeded;
}

#ifdef SENTRY_PLATFORM_WINDOWS
static bool
sentry__breakpad_backend_callback(
    const wchar_t* dump_path,
    const wchar_t* minidump_id,
    void *UNUSED(context), EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *UNUSED(assertion), bool succeeded)
{
    using std::wstring;
    wstring path = wstring(dump_path) + L"/" + minidump_id + L".dmp";

    succeeded = sentry__breakpad_backend_common( path.c_str(), exinfo, succeeded );

    return succeeded;
}
#else // SENTRY_PLATFORM_UNIX
static bool
sentry__breakpad_backend_callback(
    const google_breakpad::MinidumpDescriptor &descriptor,
    void *context, bool succeeded)
{
    sentry__page_allocator_enable();
    sentry__enter_signal_handler();

    const sentry_pathchar_t *dump_path = descriptor.path();

    succeeded = sentry__breakpad_backend_common( dump_path, context, succeeded );

    sentry__leave_signal_handler();

    return succeeded;
}
#endif

static void
sentry__breakpad_backend_startup(sentry_backend_t *backend)
{
    const sentry_options_t *options = sentry_get_options();
    sentry_path_t *current_run_folder = options->run->run_path;

#ifdef SENTRY_PLATFORM_WINDOWS
    SENTRY_DEBUGF("folder [%ws]",current_run_folder->path);
    backend->data = new google_breakpad::ExceptionHandler(
        current_run_folder->path, NULL, sentry__breakpad_backend_callback, NULL, google_breakpad::ExceptionHandler::HANDLER_ALL);
#else
    google_breakpad::MinidumpDescriptor descriptor(current_run_folder->path);
    backend->data = new google_breakpad::ExceptionHandler(
        descriptor, NULL, sentry__breakpad_backend_callback, NULL, true, -1);
#endif
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
#if defined SENTRY_PLATFORM_WINDOWS
    eh->WriteMinidumpForException((EXCEPTION_POINTERS*)&context->exception_ptrs);
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
