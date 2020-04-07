#include "sentry_boot.h"

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_scope.h"
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "sentry_unix_pageallocator.h"
#include <string.h>

#define SIGNAL_DEF(Sig, Desc)                                                  \
    {                                                                          \
        Sig, #Sig, Desc                                                        \
    }

struct signal_slot {
    int signum;
    const char *signame;
    const char *sigdesc;
};

#define MAX_FRAMES 128

// we need quite a bit of space for backtrace generation
#define SIGNAL_COUNT 6
#define SIGNAL_STACK_SIZE 65536
static struct sigaction g_sigaction;
static struct sigaction g_previous_handlers[SIGNAL_COUNT];
stack_t g_signal_stack;

static const struct signal_slot SIGNAL_DEFINITIONS[SIGNAL_COUNT] = {
    SIGNAL_DEF(SIGILL, "IllegalInstruction"),
    SIGNAL_DEF(SIGTRAP, "Trap"),
    SIGNAL_DEF(SIGABRT, "Abort"),
    SIGNAL_DEF(SIGBUS, "BusError"),
    SIGNAL_DEF(SIGFPE, "FloatingPointException"),
    SIGNAL_DEF(SIGSEGV, "Segfault"),
};

static void
reset_signal_handlers(void)
{
    for (size_t i = 0; i < SIGNAL_COUNT; i++) {
        sigaction(SIGNAL_DEFINITIONS[i].signum, &g_previous_handlers[i], 0);
    }
}

static void
invoke_signal_handler(int signum, siginfo_t *info, void *user_context)
{
    for (int i = 0; i < SIGNAL_COUNT; ++i) {
        if (SIGNAL_DEFINITIONS[i].signum == signum) {
            struct sigaction *handler = &g_previous_handlers[i];
            if (handler->sa_handler == SIG_DFL) {
                raise(signum);
            } else if (handler->sa_flags & SA_SIGINFO) {
                handler->sa_sigaction(signum, info, user_context);
            } else if (handler->sa_handler != SIG_IGN) {
                // This handler can only handle to signal number (ANSI C)
                void (*func)(int) = handler->sa_handler;
                func(signum);
            }
        }
    }
}

static void
startup_inproc_backend(sentry_backend_t *UNUSED(backend))
{
    sigaltstack(&g_signal_stack, 0);

    for (size_t i = 0; i < SIGNAL_COUNT; ++i) {
        if (sigaction(
                SIGNAL_DEFINITIONS[i].signum, NULL, &g_previous_handlers[i])
            == -1) {
            return;
        }
    }

    for (size_t i = 0; i < SIGNAL_COUNT; ++i) {
        sigaction(SIGNAL_DEFINITIONS[i].signum, &g_sigaction, NULL);
    }
}

static sentry_value_t
make_signal_event(
    const struct signal_slot *sig_slot, const sentry_ucontext_t *uctx)
{
    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(
        event, "level", sentry__value_new_level(SENTRY_LEVEL_FATAL));

    sentry_value_t exc = sentry_value_new_object();
    sentry_value_set_by_key(exc, "type",
        sentry_value_new_string(
            sig_slot ? sig_slot->signame : "UNKNOWN_SIGNAL"));
    sentry_value_set_by_key(exc, "value",
        sentry_value_new_string(
            sig_slot ? sig_slot->sigdesc : "UnknownSignal"));

    sentry_value_t mechanism = sentry_value_new_object();
    sentry_value_set_by_key(exc, "mechanism", mechanism);

    sentry_value_t mechanism_meta = sentry_value_new_object();
    sentry_value_t signal_meta = sentry_value_new_object();
    if (sig_slot) {
        sentry_value_set_by_key(
            signal_meta, "name", sentry_value_new_string(sig_slot->signame));
        sentry_value_set_by_key(signal_meta, "number",
            sentry_value_new_int32((int32_t)sig_slot->signum));
    }
    sentry_value_set_by_key(mechanism_meta, "signal", signal_meta);
    sentry_value_set_by_key(
        mechanism, "type", sentry_value_new_string("signalhandler"));
    sentry_value_set_by_key(
        mechanism, "synthetic", sentry_value_new_bool(true));
    sentry_value_set_by_key(mechanism, "handled", sentry_value_new_bool(false));
    sentry_value_set_by_key(mechanism, "meta", mechanism_meta);

    void *backtrace[MAX_FRAMES];
    size_t frame_count
        = sentry_unwind_stack_from_ucontext(uctx, &backtrace[0], MAX_FRAMES);
    SENTRY_TRACEF("captured backtrace with %zu frames", frame_count);

    sentry_value_t frames = sentry__value_new_list_with_size(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
        sentry_value_t frame = sentry_value_new_object();
        sentry_value_set_by_key(frame, "instruction_addr",
            sentry__value_new_addr((uint64_t)backtrace[frame_count - i - 1]));
        sentry_value_append(frames, frame);
    }

    sentry_value_t stacktrace = sentry_value_new_object();
    sentry_value_set_by_key(stacktrace, "frames", frames);

    sentry_value_set_by_key(exc, "stacktrace", stacktrace);

    sentry_value_t exceptions = sentry_value_new_object();
    sentry_value_t values = sentry_value_new_list();
    sentry_value_set_by_key(exceptions, "values", values);
    sentry_value_append(values, exc);
    sentry_value_set_by_key(event, "exception", exceptions);

    return event;
}

static void
handle_ucontext(sentry_ucontext_t *uctx)
{
    const struct signal_slot *sig_slot = NULL;
    for (int i = 0; i < SIGNAL_COUNT; ++i) {
        if (SIGNAL_DEFINITIONS[i].signum == uctx->signum) {
            sig_slot = &SIGNAL_DEFINITIONS[i];
        }
    }

    // give us an allocator we can use safely in signals before we tear down.
    sentry__page_allocator_enable();

    // inform the sentry_sync system that we're in a signal handler.  This will
    // make mutexes spin on a spinlock instead as it's no longer safe to use a
    // pthread mutex.
    sentry__enter_signal_handler();

    const sentry_options_t *opts = sentry_get_options();
    sentry__write_crash_marker(opts);

    // since we can’t use HTTP in signal handlers, we will swap out the
    // transport here to one that serializes the envelope to disk
    sentry_transport_t *transport = opts->transport;
    sentry__enforce_disk_transport();

    // now create an capture an event.  Note that this assumes the transport
    // only dumps to disk at the moment.
    SENTRY_DEBUG("capturing event from signal");
    sentry__end_current_session_with_status(SENTRY_SESSION_STATUS_CRASHED);
    sentry_capture_event(make_signal_event(sig_slot, uctx));

    // after capturing the crash event, try to dump all the in-flight data of
    // the previous transport
    if (transport) {
        sentry__transport_dump_queue(transport);
    }
    SENTRY_DEBUG("crash has been captured");

    // reset signal handlers and invoke the original ones.  This will then tear
    // down the process.  In theory someone might have some other handler here
    // which recovers the process but this will cause a memory leak going
    // forward as we're not restoring the page allocator.
    reset_signal_handlers();
    sentry__leave_signal_handler();
    invoke_signal_handler(
        uctx->signum, uctx->siginfo, (void *)uctx->user_context);
}

static void
handle_signal(int signum, siginfo_t *info, void *user_context)
{
    sentry_ucontext_t uctx;
    uctx.signum = signum;
    uctx.siginfo = info;
    uctx.user_context = (ucontext_t *)user_context;
    handle_ucontext(&uctx);
}

static void
handle_except(sentry_backend_t *UNUSED(backend), sentry_ucontext_t *uctx)
{
    handle_ucontext(uctx);
}

static void
free_backend(sentry_backend_t *UNUSED(backend))
{
    g_signal_stack.ss_flags = SS_DISABLE;
    sigaltstack(&g_signal_stack, 0);
    sentry_free(g_signal_stack.ss_sp);
}

sentry_backend_t *
sentry__backend_new(void)
{
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    if (!backend) {
        return NULL;
    }

    g_signal_stack.ss_sp = sentry_malloc(SIGNAL_STACK_SIZE);
    g_signal_stack.ss_size = SIGNAL_STACK_SIZE;
    g_signal_stack.ss_flags = 0;
    memset(g_previous_handlers, 0, sizeof(g_previous_handlers));
    sigemptyset(&g_sigaction.sa_mask);
    g_sigaction.sa_sigaction = handle_signal;
    g_sigaction.sa_flags = SA_SIGINFO | SA_ONSTACK;

    backend->startup_func = startup_inproc_backend;
    backend->shutdown_func = NULL;
    backend->except_func = handle_except;
    backend->free_func = free_backend;
    backend->flush_scope_func = NULL;
    backend->add_breadcrumb_func = NULL;
    backend->user_consent_changed_func = NULL;
    backend->data = NULL;

    return backend;
}
