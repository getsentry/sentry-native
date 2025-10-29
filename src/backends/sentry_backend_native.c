#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_UNIX)
#    include <errno.h>
#    include <fcntl.h>
#    include <pthread.h>
#    include <signal.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <unistd.h>
#endif

#include <string.h>

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_crash_context.h"
#include "sentry_crash_daemon.h"
#include "sentry_crash_handler.h"
#include "sentry_crash_ipc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_logger.h"
#include "sentry_logs.h"
#include "sentry_options.h"
#include "sentry_path.h"

#include "sentry_scope.h"
#include "sentry_session.h"
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "transports/sentry_disk_transport.h"

// Global process-wide synchronization for IPC and shared memory access
// This lives for the entire backend lifetime and is shared across all threads
#if defined(SENTRY_PLATFORM_WINDOWS)
static HANDLE g_ipc_mutex = NULL;
#else
#    include <semaphore.h>
static sem_t *g_ipc_init_sem = SEM_FAILED;
static char g_ipc_sem_name[64] = { 0 };
#endif

// Mutex to protect IPC initialization (POSIX only, not iOS)
#ifdef SENTRY__MUTEX_INIT_DYN
SENTRY__MUTEX_INIT_DYN(g_ipc_init_mutex)
#else
static sentry_mutex_t g_ipc_init_mutex = SENTRY__MUTEX_INIT;
#endif

/**
 * Native backend state
 */
typedef struct {
    sentry_crash_ipc_t *ipc;
    pid_t daemon_pid;
    sentry_path_t *event_path;
    sentry_path_t *breadcrumb1_path;
    sentry_path_t *breadcrumb2_path;
    sentry_path_t *envelope_path;
    size_t num_breadcrumbs;
} native_backend_state_t;

static int
native_backend_startup(
    sentry_backend_t *backend, const sentry_options_t *options)
{
    SENTRY_DEBUG("starting native backend");

#if defined(SENTRY_PLATFORM_WINDOWS)
    // Create process-wide mutex for IPC synchronization (Windows)
    // Use portable mutex to protect Windows mutex creation
    SENTRY__MUTEX_INIT_DYN_ONCE(g_ipc_init_mutex);
    sentry__mutex_lock(&g_ipc_init_mutex);

    if (!g_ipc_mutex) {
        wchar_t mutex_name[64];
        swprintf(
            mutex_name, 64, L"Local\\SentryIPC-%lu", GetCurrentProcessId());
        g_ipc_mutex = CreateMutexW(NULL, FALSE, mutex_name);
        if (!g_ipc_mutex) {
            sentry__mutex_unlock(&g_ipc_init_mutex);
            SENTRY_WARNF("failed to create IPC mutex: %lu", GetLastError());
            return 1;
        }
    }

    sentry__mutex_unlock(&g_ipc_init_mutex);
#elif !defined(SENTRY_PLATFORM_IOS)
    // Create process-wide IPC initialization semaphore (singleton pattern)
    // Protected by mutex to handle concurrent backend startups
    SENTRY__MUTEX_INIT_DYN_ONCE(g_ipc_init_mutex);
    sentry__mutex_lock(&g_ipc_init_mutex);

    if (g_ipc_init_sem == SEM_FAILED) {
        snprintf(g_ipc_sem_name, sizeof(g_ipc_sem_name), "/sentry-init-%d",
            (int)getpid());
        // Unlink any stale semaphore from previous runs
        sem_unlink(g_ipc_sem_name);
        // Create fresh semaphore with initial value 1
        g_ipc_init_sem = sem_open(g_ipc_sem_name, O_CREAT | O_EXCL, 0600, 1);
        if (g_ipc_init_sem == SEM_FAILED) {
            sentry__mutex_unlock(&g_ipc_init_mutex);
            SENTRY_WARNF("failed to create IPC semaphore: %s", strerror(errno));
            return 1;
        }
    }

    sentry__mutex_unlock(&g_ipc_init_mutex);
#endif

    native_backend_state_t *state = SENTRY_MAKE(native_backend_state_t);
    if (!state) {
        return 1;
    }
    memset(state, 0, sizeof(native_backend_state_t));
    backend->data = state;

    // Initialize IPC (protected by global synchronization for concurrent
    // access)
#if defined(SENTRY_PLATFORM_WINDOWS)
    state->ipc = sentry__crash_ipc_init_app(g_ipc_mutex);
#elif defined(SENTRY_PLATFORM_IOS)
    state->ipc = sentry__crash_ipc_init_app(NULL);
#else
    state->ipc = sentry__crash_ipc_init_app(g_ipc_init_sem);
#endif
    if (!state->ipc) {
        SENTRY_WARN("failed to initialize crash IPC");
        sentry_free(state);
        return 1;
    }

    // Configure crash context (protected by synchronization for concurrent
    // access)
#if defined(SENTRY_PLATFORM_WINDOWS)
    if (g_ipc_mutex) {
        DWORD wait_result = WaitForSingleObject(g_ipc_mutex, INFINITE);
        if (wait_result != WAIT_OBJECT_0) {
            SENTRY_WARNF("failed to acquire mutex for context setup: %lu",
                GetLastError());
            sentry__crash_ipc_free(state->ipc);
            sentry_free(state);
            return 1;
        }
    }
#elif !defined(SENTRY_PLATFORM_IOS)
    if (g_ipc_init_sem && sem_wait(g_ipc_init_sem) < 0) {
        SENTRY_WARNF("failed to acquire semaphore for context setup: %s",
            strerror(errno));
        sentry__crash_ipc_free(state->ipc);
        sentry_free(state);
        return 1;
    }
#endif

    sentry_crash_context_t *ctx = state->ipc->shmem;

    // Set minidump mode from options
    ctx->minidump_mode = (sentry_minidump_mode_t)options->minidump_mode;

    // Pass debug logging setting to daemon
    ctx->debug_enabled = options->debug;

    // Set up event and breadcrumb paths
    sentry_path_t *run_path = options->run->run_path;
    sentry_path_t *db_path = options->database_path;

    // Store database path for daemon use
    if (db_path) {
#ifdef _WIN32
        strncpy_s(ctx->database_path, sizeof(ctx->database_path), db_path->path,
            _TRUNCATE);
#else
        strncpy(
            ctx->database_path, db_path->path, sizeof(ctx->database_path) - 1);
        ctx->database_path[sizeof(ctx->database_path) - 1] = '\0';
#endif
    }

    // Store DSN for daemon to send crashes
    if (options->dsn && options->dsn->raw) {
#ifdef _WIN32
        strncpy_s(ctx->dsn, sizeof(ctx->dsn), options->dsn->raw, _TRUNCATE);
#else
        strncpy(ctx->dsn, options->dsn->raw, sizeof(ctx->dsn) - 1);
        ctx->dsn[sizeof(ctx->dsn) - 1] = '\0';
#endif
    }

    state->event_path = sentry__path_join_str(run_path, "__sentry-event");
    state->breadcrumb1_path
        = sentry__path_join_str(run_path, "__sentry-breadcrumb1");
    state->breadcrumb2_path
        = sentry__path_join_str(run_path, "__sentry-breadcrumb2");

    sentry__path_touch(state->event_path);
    sentry__path_touch(state->breadcrumb1_path);
    sentry__path_touch(state->breadcrumb2_path);

    // Copy paths to crash context
#ifdef _WIN32
    strncpy_s(ctx->event_path, sizeof(ctx->event_path), state->event_path->path,
        _TRUNCATE);
    strncpy_s(ctx->breadcrumb1_path, sizeof(ctx->breadcrumb1_path),
        state->breadcrumb1_path->path, _TRUNCATE);
    strncpy_s(ctx->breadcrumb2_path, sizeof(ctx->breadcrumb2_path),
        state->breadcrumb2_path->path, _TRUNCATE);
#else
    strncpy(
        ctx->event_path, state->event_path->path, sizeof(ctx->event_path) - 1);
    ctx->event_path[sizeof(ctx->event_path) - 1] = '\0';
    strncpy(ctx->breadcrumb1_path, state->breadcrumb1_path->path,
        sizeof(ctx->breadcrumb1_path) - 1);
    ctx->breadcrumb1_path[sizeof(ctx->breadcrumb1_path) - 1] = '\0';
    strncpy(ctx->breadcrumb2_path, state->breadcrumb2_path->path,
        sizeof(ctx->breadcrumb2_path) - 1);
    ctx->breadcrumb2_path[sizeof(ctx->breadcrumb2_path) - 1] = '\0';
#endif

    // Set up crash envelope path
    state->envelope_path = sentry__path_join_str(
        options->run->run_path, "__sentry-crash.envelope");
    if (state->envelope_path) {
#ifdef _WIN32
        strncpy_s(ctx->envelope_path, sizeof(ctx->envelope_path),
            state->envelope_path->path, _TRUNCATE);
#else
        strncpy(ctx->envelope_path, state->envelope_path->path,
            sizeof(ctx->envelope_path) - 1);
        ctx->envelope_path[sizeof(ctx->envelope_path) - 1] = '\0';
#endif
    }

    // Set up external crash reporter if configured
    if (options->external_crash_reporter) {
#ifdef _WIN32
        strncpy_s(ctx->external_reporter_path,
            sizeof(ctx->external_reporter_path),
            options->external_crash_reporter->path, _TRUNCATE);
#else
        strncpy(ctx->external_reporter_path,
            options->external_crash_reporter->path,
            sizeof(ctx->external_reporter_path) - 1);
        ctx->external_reporter_path[sizeof(ctx->external_reporter_path) - 1]
            = '\0';
#endif
    }

#if defined(SENTRY_PLATFORM_WINDOWS)
    // Release mutex after context configuration
    if (g_ipc_mutex) {
        ReleaseMutex(g_ipc_mutex);
    }
#elif !defined(SENTRY_PLATFORM_IOS)
    // Release semaphore after context configuration
    if (g_ipc_init_sem) {
        sem_post(g_ipc_init_sem);
    }
#endif

    // Install crash handlers (signal handlers on Linux/macOS, Mach exception
    // handler on iOS)
#if defined(SENTRY_PLATFORM_IOS)
    if (sentry__crash_handler_init(state->ipc) < 0) {
        SENTRY_WARN("failed to initialize crash handler");
        sentry__crash_ipc_free(state->ipc);
        sentry_free(state);
        return 1;
    }
#else
    // Other platforms: Use out-of-process daemon
    // Pass the notification handles (eventfd/pipe on Unix, events on Windows)
#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    state->daemon_pid = sentry__crash_daemon_start(
        getpid(), state->ipc->notify_fd, state->ipc->ready_fd);
#    elif defined(SENTRY_PLATFORM_MACOS)
    state->daemon_pid = sentry__crash_daemon_start(
        getpid(), state->ipc->notify_pipe[0], state->ipc->ready_pipe[1]);
#    elif defined(SENTRY_PLATFORM_WINDOWS)
    state->daemon_pid = sentry__crash_daemon_start(GetCurrentProcessId(),
        state->ipc->event_handle, state->ipc->ready_event_handle);
#    endif

    if (state->daemon_pid < 0) {
        SENTRY_WARN("failed to start crash daemon");
        sentry__crash_ipc_free(state->ipc);
        sentry_free(state);
        return 1;
    }

    SENTRY_DEBUGF("crash daemon started with PID %d", state->daemon_pid);

    // Wait for daemon to signal it's ready
    if (!sentry__crash_ipc_wait_for_ready(
            state->ipc, SENTRY_CRASH_DAEMON_READY_TIMEOUT_MS)) {
        SENTRY_WARN("Daemon did not signal ready in time, proceeding anyway");
    } else {
        SENTRY_DEBUG("Daemon signaled ready");
    }

    if (sentry__crash_handler_init(state->ipc) < 0) {
        SENTRY_WARN("failed to initialize crash handler");
#    if defined(SENTRY_PLATFORM_UNIX)
        kill(state->daemon_pid, SIGTERM);
#    elif defined(SENTRY_PLATFORM_WINDOWS)
        // On Windows, terminate the daemon process
        HANDLE hDaemon
            = OpenProcess(PROCESS_TERMINATE, FALSE, state->daemon_pid);
        if (hDaemon) {
            TerminateProcess(hDaemon, 1);
            CloseHandle(hDaemon);
        }
#    endif
        sentry__crash_ipc_free(state->ipc);
        sentry_free(state);
        return 1;
    }
#endif

    SENTRY_DEBUG("native backend started successfully");
    return 0;
}

static void
native_backend_shutdown(sentry_backend_t *backend)
{
    SENTRY_DEBUG("shutting down native backend");

    native_backend_state_t *state = (native_backend_state_t *)backend->data;
    if (!state) {
        return;
    }

    // Shutdown crash handlers (signal handlers on Linux/macOS, Mach exception
    // handler on iOS)
    sentry__crash_handler_shutdown();

#if defined(SENTRY_PLATFORM_UNIX) && !defined(SENTRY_PLATFORM_IOS)
    // Terminate daemon (Unix)
    if (state->daemon_pid > 0) {
        kill(state->daemon_pid, SIGTERM);
        // Wait for daemon to exit
        waitpid(state->daemon_pid, NULL, 0);
    }
#elif defined(SENTRY_PLATFORM_WINDOWS)
    // Terminate daemon (Windows)
    if (state->daemon_pid > 0) {
        HANDLE hDaemon = OpenProcess(
            PROCESS_TERMINATE | SYNCHRONIZE, FALSE, state->daemon_pid);
        if (hDaemon) {
            TerminateProcess(hDaemon, 0);
            // Wait for daemon to exit (with timeout)
            WaitForSingleObject(hDaemon, 5000); // 5 second timeout
            CloseHandle(hDaemon);
        }
    }
#endif

    // Cleanup IPC
    if (state->ipc) {
        sentry__crash_ipc_free(state->ipc);
        state->ipc = NULL; // Prevent use-after-free
    }

#if !defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_IOS)
    // Don't clean up semaphore here - it persists for the process lifetime
    // and may be reused if backend is restarted within same process
#endif

    SENTRY_DEBUG("native backend shutdown complete");
}

static void
native_backend_free(sentry_backend_t *backend)
{
    native_backend_state_t *state = (native_backend_state_t *)backend->data;
    if (!state) {
        return;
    }

    sentry__path_free(state->event_path);
    sentry__path_free(state->breadcrumb1_path);
    sentry__path_free(state->breadcrumb2_path);
    sentry__path_free(state->envelope_path);

    sentry_free(state);
}

static void
native_backend_flush_scope(
    sentry_backend_t *backend, const sentry_options_t *options)
{
    native_backend_state_t *state = (native_backend_state_t *)backend->data;
    if (!state || !state->event_path) {
        return;
    }

    // Create event with current scope
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "level", sentry__value_new_level(SENTRY_LEVEL_FATAL));

    // Apply scope with contexts (includes OS, device info from Sentry)
    SENTRY_WITH_SCOPE (scope) {
        // Get contexts from scope (includes OS info)
        sentry_value_t contexts
            = sentry_value_get_by_key(scope->contexts, "os");
        if (!sentry_value_is_null(contexts)) {
            sentry_value_t event_contexts = sentry_value_new_object();
            sentry_value_set_by_key(event_contexts, "os", contexts);
            sentry_value_incref(contexts);
            sentry_value_set_by_key(event, "contexts", event_contexts);
        }

        // Also copy other scope data (user, tags, extra, etc.)
        sentry_value_t user = scope->user;
        if (!sentry_value_is_null(user)) {
            sentry_value_set_by_key(event, "user", user);
            sentry_value_incref(user);
        }

        sentry_value_t tags = scope->tags;
        if (!sentry_value_is_null(tags)) {
            sentry_value_set_by_key(event, "tags", tags);
            sentry_value_incref(tags);
        }

        sentry_value_t extra = scope->extra;
        if (!sentry_value_is_null(extra)) {
            sentry_value_set_by_key(event, "extra", extra);
            sentry_value_incref(extra);
        }
    }

    // Serialize to JSON (so it can be deserialized on next start)
    char *json_str = sentry_value_to_json(event);
    sentry_value_decref(event);

    if (json_str) {
        size_t json_len = strlen(json_str);
        sentry__path_write_buffer(state->event_path, json_str, json_len);
        sentry_free(json_str);
    }

    // Write attachment metadata (paths and filenames) so crash daemon can find
    // them
    SENTRY_WITH_SCOPE (scope) {
        if (scope->attachments) {
            sentry_path_t *run_path = sentry__path_dir(state->event_path);
            if (run_path) {
                sentry_path_t *attach_list_path
                    = sentry__path_join_str(run_path, "__sentry-attachments");
                if (attach_list_path) {
                    // Write attachment list as JSON array
                    sentry_value_t attach_list = sentry_value_new_list();
                    for (sentry_attachment_t *it = scope->attachments; it;
                        it = it->next) {
                        if (it->path) {
                            sentry_value_t attach_info
                                = sentry_value_new_object();
                            sentry_value_set_by_key(attach_info, "path",
                                sentry_value_new_string(it->path->path));
                            const char *filename = sentry__path_filename(
                                it->filename ? it->filename : it->path);
                            sentry_value_set_by_key(attach_info, "filename",
                                sentry_value_new_string(filename));
                            if (it->content_type) {
                                sentry_value_set_by_key(attach_info,
                                    "content_type",
                                    sentry_value_new_string(it->content_type));
                            }
                            sentry_value_append(attach_list, attach_info);
                        }
                    }
                    char *attach_json = sentry_value_to_json(attach_list);
                    sentry_value_decref(attach_list);
                    if (attach_json) {
                        sentry__path_write_buffer(
                            attach_list_path, attach_json, strlen(attach_json));
                        sentry_free(attach_json);
                    }
                    sentry__path_free(attach_list_path);
                }
                sentry__path_free(run_path);
            }
        }
    }

    // Flush external crash report envelope if configured
    if (options->external_crash_reporter && state->envelope_path) {
        sentry_envelope_t *envelope = sentry__envelope_new();
        if (envelope && options->session) {
            sentry__envelope_add_session(envelope, options->session);
            sentry__run_write_external(options->run, envelope);
        }
        sentry_envelope_free(envelope);
    }
}

static void
native_backend_add_breadcrumb(sentry_backend_t *backend,
    sentry_value_t breadcrumb, const sentry_options_t *options)
{
    native_backend_state_t *state = (native_backend_state_t *)backend->data;
    if (!state) {
        return;
    }

    size_t max_breadcrumbs = options->max_breadcrumbs;
    if (!max_breadcrumbs) {
        return;
    }

    bool first_breadcrumb = state->num_breadcrumbs % max_breadcrumbs == 0;

    const sentry_path_t *breadcrumb_file
        = state->num_breadcrumbs % (max_breadcrumbs * 2) < max_breadcrumbs
        ? state->breadcrumb1_path
        : state->breadcrumb2_path;

    state->num_breadcrumbs++;

    if (!breadcrumb_file) {
        return;
    }

    // Serialize to JSON (so it can be deserialized on next start)
    char *json_str = sentry_value_to_json(breadcrumb);
    if (!json_str) {
        return;
    }

    size_t json_len = strlen(json_str);
    int rv = first_breadcrumb
        ? sentry__path_write_buffer(breadcrumb_file, json_str, json_len)
        : sentry__path_append_buffer(breadcrumb_file, json_str, json_len);

    sentry_free(json_str);

    if (rv != 0) {
        SENTRY_WARN("failed to write breadcrumb");
    }
}

/**
 * Ensures that buffer attachments have a unique path in the run directory.
 * Similar to Crashpad's ensure_unique_path function.
 */
static bool
ensure_attachment_path(sentry_attachment_t *attachment)
{
    if (!attachment || !attachment->filename) {
        return false;
    }

    // Generate UUID for unique path
    sentry_uuid_t uuid = sentry_uuid_new_v4();
    char uuid_str[37];
    sentry_uuid_as_string(&uuid, uuid_str);

    sentry_path_t *base_path = NULL;
    SENTRY_WITH_OPTIONS (options) {
        if (options->run && options->run->run_path) {
            base_path = sentry__path_join_str(options->run->run_path, uuid_str);
        }
    }

    if (!base_path || sentry__path_create_dir_all(base_path) != 0) {
        sentry__path_free(base_path);
        return false;
    }

    sentry_path_t *old_path = attachment->path;
    attachment->path = sentry__path_join_str(
        base_path, sentry__path_filename(attachment->filename));

    sentry__path_free(base_path);
    sentry__path_free(old_path);
    return attachment->path != NULL;
}

static void
native_backend_add_attachment(
    sentry_backend_t *backend, sentry_attachment_t *attachment)
{
    (void)backend; // Unused

    // For buffer attachments, assign a path in the run directory and write to
    // disk
    if (attachment->buf) {
        if (!attachment->path) {
            if (!ensure_attachment_path(attachment)) {
                SENTRY_WARN("failed to assign path for buffer attachment");
                return;
            }
        }

        // Write buffer to disk
        if (sentry__path_write_buffer(
                attachment->path, attachment->buf, attachment->buf_len)
            != 0) {
            SENTRY_WARNF("failed to write native backend attachment \"%s\"",
                attachment->path->path);
        }
    }
    // For file attachments, the path is already set and points to the actual
    // file. The crash daemon will read these files from their original
    // locations.
}

/**
 * Handle exception - called from signal handler via sentry_handle_exception
 * This processes the event with on_crash/before_send hooks and ends the session
 */
static void
native_backend_except(sentry_backend_t *backend, const sentry_ucontext_t *uctx)
{
    SENTRY_WITH_OPTIONS (options) {
        // Disable logging during crash handling if configured
        if (!options->enable_logging_when_crashed) {
            sentry__logger_disable();
        }

        SENTRY_DEBUG("handling native backend exception");

        // Flush logs in crash-safe manner
        if (options->enable_logs) {
            sentry__logs_flush_crash_safe();
        }

        // Write crash marker
        sentry__write_crash_marker(options);

        // Create crash event
        sentry_value_t event = sentry_value_new_event();
        sentry_value_set_by_key(
            event, "level", sentry__value_new_level(SENTRY_LEVEL_FATAL));

        bool should_handle = true;

        // Call on_crash hook if configured
        if (options->on_crash_func) {
            SENTRY_DEBUG("invoking `on_crash` hook");
            sentry_value_t result
                = options->on_crash_func(uctx, event, options->on_crash_data);
            should_handle = !sentry_value_is_null(result);
            event = result;
        }

        if (should_handle) {
            native_backend_state_t *state
                = (native_backend_state_t *)backend->data;

            // Apply before_send hook if on_crash wasn't set
            if (!options->on_crash_func && options->before_send_func) {
                SENTRY_DEBUG("invoking `before_send` hook");
                event = options->before_send_func(
                    event, NULL, options->before_send_data);
                should_handle = !sentry_value_is_null(event);
            }

            if (should_handle) {
                // Apply scope to event including breadcrumbs
                SENTRY_WITH_SCOPE (scope) {
                    sentry__scope_apply_to_event(
                        scope, options, event, SENTRY_SCOPE_BREADCRUMBS);
                }

                // Write event as JSON file
                // Daemon will read this and create envelope with minidump
                if (state && state->event_path) {
                    char *event_json = sentry_value_to_json(event);
                    if (event_json) {
                        int rv = sentry__path_write_buffer(
                            state->event_path, event_json, strlen(event_json));
                        sentry_free(event_json);
                        if (rv == 0) {
                            SENTRY_DEBUG("Wrote crash event JSON for daemon");
                        } else {
                            SENTRY_WARN("Failed to write event JSON");
                        }
                    }
                }

                sentry_value_decref(event);

                // End session with crashed status and write session envelope to
                // disk
                sentry__record_errors_on_current_session(1);
                sentry_session_t *session
                    = sentry__end_current_session_with_status(
                        SENTRY_SESSION_STATUS_CRASHED);

                if (session) {
                    sentry_envelope_t *envelope = sentry__envelope_new();
                    sentry__envelope_add_session(envelope, session);

                    // Write session envelope to disk
                    sentry_transport_t *disk_transport
                        = sentry_new_disk_transport(options->run);
                    if (disk_transport) {
                        sentry__capture_envelope(disk_transport, envelope);
                        sentry__transport_dump_queue(
                            disk_transport, options->run);
                        sentry_transport_free(disk_transport);
                    }
                }

                // Dump any pending transport queue
                sentry__transport_dump_queue(options->transport, options->run);

                SENTRY_DEBUG("crash event and session written, daemon will "
                             "create and send minidump");
            }
        } else {
            SENTRY_DEBUG("event was discarded by the `on_crash` hook");
            sentry_value_decref(event);
        }
    }
}

/**
 * Create native backend
 */
sentry_backend_t *
sentry__backend_new(void)
{
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    if (!backend) {
        return NULL;
    }

    memset(backend, 0, sizeof(sentry_backend_t));

    backend->startup_func = native_backend_startup;
    backend->shutdown_func = native_backend_shutdown;
    backend->free_func = native_backend_free;
    backend->except_func = native_backend_except;
    backend->flush_scope_func = native_backend_flush_scope;
    backend->add_breadcrumb_func = native_backend_add_breadcrumb;
    backend->add_attachment_func = native_backend_add_attachment;
    backend->can_capture_after_shutdown = false;

    return backend;
}
