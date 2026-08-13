#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_UNIX)
#    include <errno.h>
#    include <fcntl.h>
#    include <pthread.h>
#    include <signal.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <unistd.h>
#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#        include <sys/prctl.h>
#    elif defined(SENTRY_PLATFORM_MACOS)
#        include <crt_externs.h>
#        include <mach-o/dyld.h>
#        include <spawn.h>
#    endif
#elif defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_XBOX)
#    include <werapi.h>
#endif

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_crash_context.h"
#include "sentry_crash_handler.h"
#include "sentry_crash_ipc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_os.h"
#include "sentry_path.h"

#include "sentry_scope.h"
#include "sentry_session.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_telemetry.h"
#include "sentry_tracing.h"
#include "sentry_transport.h"
#include "sentry_value.h"

// Global process-wide synchronization for IPC and shared memory access
// This lives for the entire backend lifetime and is shared across all threads
#if defined(SENTRY_PLATFORM_WINDOWS)
static HANDLE g_ipc_mutex = NULL;
#elif defined(SENTRY_PLATFORM_MACOS)
// macOS uses a plain pthread mutex instead of named semaphores (sem_open)
// because App Sandbox blocks POSIX named semaphores.
static sentry_mutex_t g_ipc_sync_mutex = SENTRY__MUTEX_INIT;
#else
#    include <semaphore.h>
static sem_t *g_ipc_init_sem = SEM_FAILED;
static char g_ipc_sem_name[64] = { 0 };
#endif

// Mutex to protect IPC initialization (Windows and Linux only, not macOS/iOS)
// macOS uses g_ipc_sync_mutex directly; iOS has no out-of-process daemon.
#if defined(SENTRY_PLATFORM_WINDOWS)                                           \
    || (!defined(SENTRY_PLATFORM_MACOS) && !defined(SENTRY_PLATFORM_IOS))
#    ifdef SENTRY__MUTEX_INIT_DYN
SENTRY__MUTEX_INIT_DYN(g_ipc_init_mutex)
#    else
static sentry_mutex_t g_ipc_init_mutex = SENTRY__MUTEX_INIT;
#    endif
#endif

#if defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_XBOX)
static sentry_wer_registration_t g_wer_registration = { 0 };

static sentry_path_t *g_wer_path = NULL;

static LSTATUS
wer_set_registry_value(const sentry_path_t *wer_path, DWORD value)
{
    return RegSetKeyValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\Windows Error Reporting\\"
        L"RuntimeExceptionHelperModules",
        wer_path->path_w, REG_DWORD, &value, sizeof(value));
}

static LSTATUS
wer_delete_registry_value(const sentry_path_t *wer_path)
{
    return RegDeleteKeyValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\Windows Error Reporting\\"
        L"RuntimeExceptionHelperModules",
        wer_path->path_w);
}

static sentry_path_t *
wer_default_path(void)
{
    sentry_path_t *current_exe = sentry__path_current_exe();
    if (!current_exe) {
        return NULL;
    }

    sentry_path_t *exe_dir = sentry__path_dir(current_exe);
    sentry__path_free(current_exe);
    if (!exe_dir) {
        return NULL;
    }

    sentry_path_t *wer_path = sentry__path_join_str(exe_dir, "sentry-wer.dll");
    sentry__path_free(exe_dir);
    return wer_path;
}

static void
wer_unregister_module(void)
{
    if (!g_wer_path) {
        return;
    }

    WerUnregisterRuntimeExceptionModule(
        g_wer_path->path_w, &g_wer_registration);
    wer_delete_registry_value(g_wer_path);
    sentry__path_free(g_wer_path);
    g_wer_path = NULL;
    memset(&g_wer_registration, 0, sizeof(g_wer_registration));
}

static bool
wer_register_module(uint64_t app_tid)
{
    windows_version_t win_ver;
    if (!sentry__get_windows_version(&win_ver) || win_ver.build < 19041) {
        SENTRY_WARN("Native WER module not registered, because Windows "
                    "doesn't meet version requirements (build >= 19041).");
        return false;
    }

    sentry_path_t *wer_path = wer_default_path();
    if (!wer_path || !sentry__path_is_file(wer_path)) {
        SENTRY_WARN("Native WER module not found");
        sentry__path_free(wer_path);
        return false;
    }

    const DWORD one = 1;
    LSTATUS reg_res = wer_set_registry_value(wer_path, one);
    if (reg_res != ERROR_SUCCESS) {
        SENTRY_WARN("registering native WER module in registry failed");
        sentry__path_free(wer_path);
        return false;
    }

    g_wer_registration.version = 1;
    g_wer_registration.app_pid = GetCurrentProcessId();
    g_wer_registration.app_tid = app_tid;

    HRESULT hr = WerRegisterRuntimeExceptionModule(
        wer_path->path_w, &g_wer_registration);
    if (FAILED(hr)) {
        SENTRY_WARN("registering native WER module failed");
        wer_delete_registry_value(wer_path);
        sentry__path_free(wer_path);
        memset(&g_wer_registration, 0, sizeof(g_wer_registration));
        return false;
    }

    SENTRY_DEBUGF("registered native WER module \"%s\"", wer_path->path);
    g_wer_path = wer_path;
    return true;
}

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
    volatile long crashed;
} native_backend_state_t;

static bool
native_backend_process_old_run(sentry_backend_t *backend,
    const sentry_options_t *options, const sentry_path_t *run_path)
{
    (void)backend;

    sentry_pathiter_t *it = sentry__path_iter_directory(run_path);
    const sentry_path_t *file;
    while (it && (file = sentry__pathiter_next(it)) != NULL) {
        if (!sentry__path_is_file(file) || sentry__path_is_symlink(file)
            || !sentry__path_filename_matches(
                file, "__sentry-crash.envelope")) {
            continue;
        }

        sentry_envelope_t *envelope = options->on_crashed_last_run_func
            ? sentry__envelope_from_path(file)
            : NULL;
        bool materialized = envelope && sentry__envelope_materialize(envelope);
        // remove before invoking to prevent repeated callbacks
        if (sentry__path_remove(file) != 0) {
            sentry_envelope_free(envelope);
            sentry__pathiter_free(it);
            return false;
        }
        if (materialized) {
            options->on_crashed_last_run_func(
                envelope, options->on_crashed_last_run_data);
        }
        sentry_envelope_free(envelope);
    }
    sentry__pathiter_free(it);
    return true;
}

/**
 * Start crash daemon for monitoring app process
 * This forks a child process (Unix) or creates a new process (Windows) that
 * waits for crashes
 *
 * @param app_pid Parent application process ID
 * @param app_tid Parent application thread ID
 * @param notify_handle Crash notification handle
 * @param ready_handle Ready signal handle
 * @return Daemon PID on success, -1 on failure
 */
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
static pid_t
daemon_start(pid_t app_pid, uint64_t app_tid, int notify_eventfd,
    int ready_eventfd, const char *handler_path)
#elif defined(SENTRY_PLATFORM_MACOS)
static pid_t
daemon_start(pid_t app_pid, uint64_t app_tid, int notify_pipe_read,
    int ready_pipe_write, int shm_fd, const char *handler_path)
#elif defined(SENTRY_PLATFORM_WINDOWS)
static pid_t
daemon_start(pid_t app_pid, uint64_t app_tid, HANDLE event_handle,
    HANDLE ready_event_handle, const char *handler_path)
#endif
{
#if defined(SENTRY_PLATFORM_MACOS)
    // macOS: Use posix_spawn instead of fork+exec for App Sandbox
    // compatibility. posix_spawn is Apple's recommended API and works correctly
    // in sandboxed processes, unlike fork() which can have issues with sandbox
    // inheritance.

    // Resolve daemon path
    char daemon_path[SENTRY_CRASH_MAX_PATH];
    if (!sentry__string_empty(handler_path)) {
        strncpy(daemon_path, handler_path, sizeof(daemon_path) - 1);
        daemon_path[sizeof(daemon_path) - 1] = '\0';
    } else {
        char exe_path[SENTRY_CRASH_MAX_PATH];
        uint32_t exe_size = sizeof(exe_path);
        if (_NSGetExecutablePath(exe_path, &exe_size) != 0) {
            SENTRY_WARN("Failed to get executable path for daemon");
            return -1;
        }
        const char *slash = strrchr(exe_path, '/');
        if (!slash
            || (size_t)(slash - exe_path + 1) + strlen("sentry-crash")
                >= sizeof(daemon_path)) {
            SENTRY_WARN("Daemon path too long");
            return -1;
        }
        size_t dir_len = (size_t)(slash - exe_path + 1);
        memcpy(daemon_path, exe_path, dir_len);
        strcpy(daemon_path + dir_len, "sentry-crash");
    }

    // Build argument strings (6 args: pid, tid, notify_fd, ready_fd, shm_fd)
    char pid_str[32], tid_str[32], notify_str[32], ready_str[32], shm_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", (int)app_pid);
    snprintf(tid_str, sizeof(tid_str), "%" PRIx64, app_tid);
    snprintf(notify_str, sizeof(notify_str), "%d", notify_pipe_read);
    snprintf(ready_str, sizeof(ready_str), "%d", ready_pipe_write);
    snprintf(shm_str, sizeof(shm_str), "%d", shm_fd);

    char *spawn_argv[] = { "sentry-crash", pid_str, tid_str, notify_str,
        ready_str, shm_str, NULL };

    // Set up posix_spawn attributes
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    // POSIX_SPAWN_SETSID: create new session (like setsid() after fork)
    // POSIX_SPAWN_CLOEXEC_DEFAULT: close all fds except explicitly inherited
    short spawn_flags = POSIX_SPAWN_SETSID | POSIX_SPAWN_CLOEXEC_DEFAULT;
    posix_spawnattr_setflags(&attr, spawn_flags);

    // Explicitly inherit only the fds the daemon needs
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);
    posix_spawn_file_actions_addinherit_np(&file_actions, notify_pipe_read);
    posix_spawn_file_actions_addinherit_np(&file_actions, ready_pipe_write);
    posix_spawn_file_actions_addinherit_np(&file_actions, shm_fd);
    // Open /dev/null on stdin/stdout/stderr so the daemon starts with valid
    // standard fds. Without this, POSIX_SPAWN_CLOEXEC_DEFAULT closes them,
    // and the first fopen() in the daemon would get fd 0, which the daemon's
    // own close(STDIN_FILENO) would then destroy.
    // Skip if an IPC fd occupies that slot (e.g. caller closed stdin before
    // sentry_init), to avoid clobbering it with /dev/null.
    int std_fds[3] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
    int std_modes[3] = { O_RDONLY, O_WRONLY, O_WRONLY };
    for (int i = 0; i < 3; i++) {
        if (std_fds[i] != notify_pipe_read && std_fds[i] != ready_pipe_write
            && std_fds[i] != shm_fd) {
            posix_spawn_file_actions_addopen(
                &file_actions, std_fds[i], "/dev/null", std_modes[i], 0);
        }
    }

    pid_t daemon_pid;
    int spawn_result = posix_spawn(&daemon_pid, daemon_path, &file_actions,
        &attr, spawn_argv, *_NSGetEnviron());

    posix_spawn_file_actions_destroy(&file_actions);
    posix_spawnattr_destroy(&attr);

    if (spawn_result != 0) {
        SENTRY_WARNF("posix_spawn failed for %s: %s", daemon_path,
            strerror(spawn_result));
        return -1;
    }

    return daemon_pid;

#elif defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    // Linux: Use fork+exec
    pid_t daemon_pid = fork();

    if (daemon_pid < 0) {
        SENTRY_WARN("Failed to fork daemon process");
        return -1;
    } else if (daemon_pid == 0) {
        // Child process - exec sentry-crash
        setsid();

        // Clear FD_CLOEXEC on notify and ready fds so they survive exec
        int notify_flags = fcntl(notify_eventfd, F_GETFD);
        if (notify_flags != -1) {
            fcntl(notify_eventfd, F_SETFD, notify_flags & ~FD_CLOEXEC);
        }
        int ready_flags = fcntl(ready_eventfd, F_GETFD);
        if (ready_flags != -1) {
            fcntl(ready_eventfd, F_SETFD, ready_flags & ~FD_CLOEXEC);
        }

        // Convert arguments to strings for exec
        char pid_str[32], tid_str[32], notify_str[32], ready_str[32];
        snprintf(pid_str, sizeof(pid_str), "%d", (int)app_pid);
        snprintf(tid_str, sizeof(tid_str), "%" PRIx64, app_tid);
        snprintf(notify_str, sizeof(notify_str), "%d", notify_eventfd);
        snprintf(ready_str, sizeof(ready_str), "%d", ready_eventfd);

        char *argv[]
            = { "sentry-crash", pid_str, tid_str, notify_str, ready_str, NULL };

        if (!sentry__string_empty(handler_path)) {
            execv(handler_path, argv);
        } else {
            char exe_path[SENTRY_CRASH_MAX_PATH];
            char daemon_exec_path[SENTRY_CRASH_MAX_PATH];

            ssize_t exe_len
                = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
            if (exe_len > 0) {
                exe_path[exe_len] = '\0';
                const char *slash = strrchr(exe_path, '/');
                if (slash) {
                    size_t dir_len = (size_t)(slash - exe_path + 1);
                    if (dir_len + strlen("sentry-crash")
                        < sizeof(daemon_exec_path)) {
                        memcpy(daemon_exec_path, exe_path, dir_len);
                        strcpy(daemon_exec_path + dir_len, "sentry-crash");
                        execv(daemon_exec_path, argv);
                    }
                }
            }
        }

        // exec failed - exit with error
        perror("Failed to exec sentry-crash");
        _exit(1);
    }

    // Parent process - return daemon PID
    return daemon_pid;

#elif defined(SENTRY_PLATFORM_WINDOWS)
    // On Windows, create a separate daemon process using CreateProcess
    // Spawn the sentry-crash.exe executable

    wchar_t daemon_path_w[SENTRY_CRASH_MAX_PATH];

    // If handler_path was explicitly set via options, use it directly
    if (!sentry__string_empty(handler_path)) {
        wchar_t *wpath = sentry__string_to_wstr(handler_path);
        if (wpath) {
            wcsncpy(daemon_path_w, wpath, SENTRY_CRASH_MAX_PATH - 1);
            daemon_path_w[SENTRY_CRASH_MAX_PATH - 1] = L'\0';
            sentry_free(wpath);
        } else {
            SENTRY_WARN("Failed to convert handler_path to wide string");
            return (pid_t)-1;
        }
    } else {
        // Try to find sentry-crash.exe in the same directory as the current
        // executable
        wchar_t exe_dir[SENTRY_CRASH_MAX_PATH];
        DWORD len = GetModuleFileNameW(NULL, exe_dir, SENTRY_CRASH_MAX_PATH);
        if (len == 0 || len >= SENTRY_CRASH_MAX_PATH) {
            SENTRY_WARN("Failed to get current executable path");
            return (pid_t)-1;
        }

        // Remove filename to get directory
        wchar_t *last_slash = wcsrchr(exe_dir, L'\\');
        if (last_slash) {
            *(last_slash + 1) = L'\0'; // Keep the trailing backslash
        }

        // Build full path to sentry-crash.exe
        int path_len = _snwprintf(daemon_path_w, SENTRY_CRASH_MAX_PATH,
            L"%ssentry-crash.exe", exe_dir);
        if (path_len < 0 || path_len >= SENTRY_CRASH_MAX_PATH) {
            SENTRY_WARN("Daemon path too long");
            return (pid_t)-1;
        }
    }

    // Log the daemon path we're trying to launch for debugging
    char *daemon_path_utf8 = sentry__string_from_wstr(daemon_path_w);
    if (daemon_path_utf8) {
        SENTRY_DEBUGF("Attempting to launch daemon: %s", daemon_path_utf8);
        sentry_free(daemon_path_utf8);
    }

    // Build command line: sentry-crash.exe <app_pid> <app_tid> <event_handle>
    // <ready_event_handle>
    wchar_t cmd_line[SENTRY_CRASH_MAX_PATH + 128];
    int cmd_len = _snwprintf(cmd_line, sizeof(cmd_line) / sizeof(wchar_t),
        L"\"%s\" %lu %llx %llu %llu", daemon_path_w, (unsigned long)app_pid,
        (unsigned long long)app_tid,
        (unsigned long long)(uintptr_t)event_handle,
        (unsigned long long)(uintptr_t)ready_event_handle);

    if (cmd_len < 0 || cmd_len >= (int)(sizeof(cmd_line) / sizeof(wchar_t))) {
        SENTRY_WARN("Command line too long for daemon spawn");
        return (pid_t)-1;
    }

    // Prepare process creation structures
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    // Hide console window for daemon
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    // Create the daemon process
    if (!CreateProcessW(NULL, // Application name (use command line)
            cmd_line, // Command line
            NULL, // Process security attributes
            NULL, // Thread security attributes
            TRUE, // Inherit handles (for event_handle)
            CREATE_NO_WINDOW | DETACHED_PROCESS, // Creation flags
            NULL, // Environment
            NULL, // Current directory
            &si, // Startup info
            &pi)) { // Process information
        DWORD error = GetLastError();
        char *daemon_path_err = sentry__string_from_wstr(daemon_path_w);
        if (daemon_path_err) {
            SENTRY_WARNF("Failed to create daemon process at '%s': Error %lu%s",
                daemon_path_err, error,
                error == 2       ? " (File not found)"
                    : error == 3 ? " (Path not found)"
                                 : "");
            sentry_free(daemon_path_err);
        } else {
            SENTRY_WARNF("Failed to create daemon process: %lu", error);
        }
        return (pid_t)-1;
    }

    // Close thread handle (we don't need it)
    CloseHandle(pi.hThread);

    // Close process handle (daemon is independent)
    CloseHandle(pi.hProcess);

    // Return daemon process ID
    return pi.dwProcessId;
#endif
}

static int
native_backend_startup(
    sentry_backend_t *backend, const sentry_options_t *options)
{
    SENTRY_DEBUG("starting native backend");

#if defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_BUILD_SHARED)          \
    && defined(SENTRY_THREAD_STACK_GUARANTEE_AUTO_INIT)
    sentry__set_default_thread_stack_guarantee();
#endif

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
#elif defined(SENTRY_PLATFORM_MACOS)
    // macOS uses a plain pthread mutex (no sem_open which is blocked by App
    // Sandbox). The mutex is statically initialized - no setup needed.
    (void)0;
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
    backend->data = state;

    // Initialize IPC (protected by global synchronization for concurrent
    // access)
#if defined(SENTRY_PLATFORM_WINDOWS)
    state->ipc = sentry__crash_ipc_init_app(g_ipc_mutex);
#elif defined(SENTRY_PLATFORM_IOS)
    state->ipc = sentry__crash_ipc_init_app(NULL);
#elif defined(SENTRY_PLATFORM_MACOS)
    state->ipc = sentry__crash_ipc_init_app(&g_ipc_sync_mutex);
#else
    state->ipc = sentry__crash_ipc_init_app(g_ipc_init_sem);
#endif
    if (!state->ipc) {
        SENTRY_WARN("failed to initialize crash IPC");
        sentry_free(state);
        backend->data = NULL;
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
            backend->data = NULL;
            return 1;
        }
    }
#elif defined(SENTRY_PLATFORM_MACOS)
    sentry__mutex_lock(&g_ipc_sync_mutex);
#elif !defined(SENTRY_PLATFORM_IOS)
    if (g_ipc_init_sem && sem_wait(g_ipc_init_sem) < 0) {
        SENTRY_WARNF("failed to acquire semaphore for context setup: %s",
            strerror(errno));
        sentry__crash_ipc_free(state->ipc);
        sentry_free(state);
        backend->data = NULL;
        return 1;
    }
#endif

    sentry_crash_context_t *ctx = state->ipc->shmem;

    // Set minidump mode from options
    ctx->minidump_mode = (sentry_minidump_mode_t)options->minidump_mode;
#ifdef SENTRY_PLATFORM_WINDOWS
    ctx->minidump_flags = options->minidump_flags;
#endif

    // Set crash reporting mode from options
    ctx->crash_reporting_mode = options->crash_reporting_mode;
    ctx->system_crash_reporter_enabled = options->system_crash_reporter_enabled;
    ctx->crash_upload_mode = options->crash_upload_mode;
    ctx->thread_stackwalk_mode = options->thread_stackwalk_mode;

    // Pass debug logging setting to daemon
    ctx->debug_enabled = options->debug;
    ctx->attach_screenshot = options->attach_screenshot;
    ctx->attach_session_replay = options->attach_session_replay;
    ctx->session_replay_duration = options->session_replay_duration;
    ctx->cache_keep = (int)options->cache_keep;
    ctx->require_user_consent = options->require_user_consent;
    ctx->has_on_crashed_last_run = options->on_crashed_last_run_func != NULL;
    ctx->enable_large_attachments = options->enable_large_attachments;
    ctx->http_retry = options->http_retry;
    ctx->shutdown_timeout = options->shutdown_timeout;
    ctx->transfer_timeout = options->transfer_timeout;
    ctx->max_breadcrumbs = (uint32_t)options->max_breadcrumbs;
#if defined(SENTRY_PLATFORM_WINDOWS)
    ctx->platform.wer_integration
        = sentry__options_has_integration(options, "wer");
#endif
    sentry__atomic_store(
        &ctx->user_consent, sentry__atomic_fetch(&options->run->user_consent));

    // Set up event and breadcrumb paths
    sentry_path_t *run_path = options->run->run_path;
    sentry_path_t *db_path = options->database_path;

#ifdef _WIN32
    strncpy_s(ctx->run_path, sizeof(ctx->run_path), run_path->path, _TRUNCATE);
#else
    strncpy(ctx->run_path, run_path->path, sizeof(ctx->run_path) - 1);
    ctx->run_path[sizeof(ctx->run_path) - 1] = '\0';
#endif

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

    // Store transport configuration for daemon's curl transport
    if (options->ca_certs) {
#ifdef _WIN32
        strncpy_s(
            ctx->ca_certs, sizeof(ctx->ca_certs), options->ca_certs, _TRUNCATE);
#else
        strncpy(ctx->ca_certs, options->ca_certs, sizeof(ctx->ca_certs) - 1);
        ctx->ca_certs[sizeof(ctx->ca_certs) - 1] = '\0';
#endif
    }

    if (options->proxy) {
#ifdef _WIN32
        strncpy_s(ctx->proxy, sizeof(ctx->proxy), options->proxy, _TRUNCATE);
#else
        strncpy(ctx->proxy, options->proxy, sizeof(ctx->proxy) - 1);
        ctx->proxy[sizeof(ctx->proxy) - 1] = '\0';
#endif
    }

    if (options->user_agent) {
#ifdef _WIN32
        strncpy_s(ctx->user_agent, sizeof(ctx->user_agent), options->user_agent,
            _TRUNCATE);
#else
        strncpy(
            ctx->user_agent, options->user_agent, sizeof(ctx->user_agent) - 1);
        ctx->user_agent[sizeof(ctx->user_agent) - 1] = '\0';
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
#elif defined(SENTRY_PLATFORM_MACOS)
    sentry__mutex_unlock(&g_ipc_sync_mutex);
#elif !defined(SENTRY_PLATFORM_IOS)
    // Release semaphore after context configuration
    if (g_ipc_init_sem) {
        sem_post(g_ipc_init_sem);
    }
#endif

    // Install crash handlers (signal handlers on Linux/macOS, Mach exception
    // handler on iOS)
#if defined(SENTRY_PLATFORM_IOS)
    if (sentry__crash_handler_init(state->ipc, SENTRY_HANDLER_STRATEGY_DEFAULT)
        < 0) {
        SENTRY_WARN("failed to initialize crash handler");
        sentry__crash_ipc_free(state->ipc);
        sentry_free(state);
        backend->data = NULL;
        return 1;
    }
#else
    // Other platforms: Use out-of-process daemon
    // Pass the notification handles (eventfd/pipe on Unix, events on Windows)
    const char *daemon_handler_path
        = options->handler_path ? options->handler_path->path : NULL;
#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    uint64_t tid = (uint64_t)pthread_self();
    state->daemon_pid = daemon_start(getpid(), tid, state->ipc->notify_fd,
        state->ipc->ready_fd, daemon_handler_path);
#    elif defined(SENTRY_PLATFORM_MACOS)
    uint64_t tid = (uint64_t)pthread_self();
    state->daemon_pid = daemon_start(getpid(), tid, state->ipc->notify_pipe[0],
        state->ipc->ready_pipe[1], state->ipc->shm_fd, daemon_handler_path);
#    elif defined(SENTRY_PLATFORM_WINDOWS)
    uint64_t tid = (uint64_t)GetCurrentThreadId();
    state->daemon_pid
        = daemon_start(GetCurrentProcessId(), tid, state->ipc->event_handle,
            state->ipc->ready_event_handle, daemon_handler_path);
#    endif

    // On Windows, pid_t is DWORD (unsigned), so (pid_t)-1 == 0xFFFFFFFF.
    // On Unix, pid_t is signed and fork returns -1 on failure.
#    if defined(SENTRY_PLATFORM_WINDOWS)
    if (state->daemon_pid == (pid_t)-1) {
#    else
    if (state->daemon_pid < 0) {
#    endif
        SENTRY_WARN("failed to start crash daemon");
        sentry__crash_ipc_free(state->ipc);
        sentry_free(state);
        backend->data = NULL;
        return 1;
    }

    SENTRY_DEBUGF("crash daemon started with PID %d", state->daemon_pid);

#    if defined(SENTRY_PLATFORM_MACOS)
    // Close unused pipe ends in parent process
    close(state->ipc->notify_pipe[0]); // Daemon reads from this
    close(state->ipc->ready_pipe[1]); // Daemon writes to this
    state->ipc->notify_pipe[0] = -1;
    state->ipc->ready_pipe[1] = -1;
#    endif

#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    // Close unused eventfd ends in parent process
    // (eventfds are bidirectional, but we only use one direction per fd)
    // Parent writes to notify_fd, daemon reads from it - parent can close for
    // reading Daemon writes to ready_fd, parent reads from it - parent can
    // close for writing Actually, eventfds can't be closed for one direction,
    // so keep them open

    // On Linux, allow the daemon to ptrace this process
    // This is required when Yama LSM ptrace_scope is enabled
    if (prctl(PR_SET_PTRACER, state->daemon_pid, 0, 0, 0) != 0) {
        SENTRY_WARNF(
            "prctl(PR_SET_PTRACER) failed: %s - daemon may not be able to "
            "read process memory",
            strerror(errno));
    } else {
        SENTRY_DEBUGF("Set daemon PID %d as ptracer", state->daemon_pid);
    }
#    endif

    // Wait for daemon to signal it's ready
    if (!sentry__crash_ipc_wait_for_ready(
            state->ipc, SENTRY_CRASH_DAEMON_READY_TIMEOUT_MS)) {
        SENTRY_WARN("Daemon did not signal ready in time, proceeding anyway");
    } else {
        SENTRY_DEBUG("Daemon signaled ready");
    }

#    if defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_XBOX)
    state->ipc->shmem->platform.wer_enabled = wer_register_module(tid);
#    endif

    sentry_handler_strategy_t strategy =
#    if defined(SENTRY_PLATFORM_LINUX) && !defined(SENTRY_PLATFORM_ANDROID)
        options ? sentry_options_get_handler_strategy(options) :
#    endif
                SENTRY_HANDLER_STRATEGY_DEFAULT;
    if (sentry__crash_handler_init(state->ipc, strategy) < 0) {
        SENTRY_WARN("failed to initialize crash handler");
#    if defined(SENTRY_PLATFORM_UNIX)
        kill(state->daemon_pid, SIGTERM);
#    elif defined(SENTRY_PLATFORM_WINDOWS)
#        if !defined(SENTRY_PLATFORM_XBOX)
        wer_unregister_module();
#        endif
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
        backend->data = NULL;
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

#if defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_XBOX)
    wer_unregister_module();
#endif

    // Shutdown crash handlers (signal handlers on Linux/macOS, Mach exception
    // handler on iOS)
    sentry__crash_handler_shutdown();

    bool daemon_stopped = false;
#if defined(SENTRY_PLATFORM_UNIX) && !defined(SENTRY_PLATFORM_IOS)
    // Terminate daemon (Unix)
    if (state->daemon_pid > 0) {
        kill(state->daemon_pid, SIGTERM);
        // Wait for daemon to exit
        pid_t wait_result;
        do {
            wait_result = waitpid(state->daemon_pid, NULL, 0);
        } while (wait_result < 0 && errno == EINTR);
        daemon_stopped = wait_result == state->daemon_pid;
    }
#elif defined(SENTRY_PLATFORM_WINDOWS)
    // Terminate daemon (Windows)
    if (state->daemon_pid > 0) {
        HANDLE hDaemon = OpenProcess(
            PROCESS_TERMINATE | SYNCHRONIZE, FALSE, state->daemon_pid);
        if (hDaemon) {
            TerminateProcess(hDaemon, 0);
            // Wait for daemon to exit (with timeout)
            daemon_stopped
                = WaitForSingleObject(hDaemon, 5000) == WAIT_OBJECT_0;
            CloseHandle(hDaemon);
        }
    }
#endif

    if (daemon_stopped && state->ipc && state->ipc->shmem
        && state->ipc->shmem->run_path[0]) {
        sentry_path_t *run_path
            = sentry__path_from_str(state->ipc->shmem->run_path);
        sentry_path_t *lock_path = run_path
            ? sentry__path_append_str(run_path, ".daemon.lock")
            : NULL;
        if (lock_path) {
            sentry__path_remove(lock_path);
        }
        sentry__path_free(lock_path);
        sentry__path_free(run_path);
    }

    // Dump daemon log file for debugging (especially useful in CI).
    // This bypasses the SDK logger and writes straight to stderr, so it must
    // only run when debug logging was enabled. When debug is off the daemon
    // keeps logging to its file (at INFO level), but we stay silent on stderr
    // rather than spamming the terminal on shutdown.
    if (state->ipc && state->ipc->shmem && state->ipc->shmem->debug_enabled) {
        char log_path[SENTRY_CRASH_MAX_PATH];
        int log_path_len = -1;
        FILE *log_file = NULL;

#if defined(SENTRY_PLATFORM_WINDOWS)
        log_path_len = _snprintf(log_path, sizeof(log_path),
            "%s\\sentry-daemon.log", state->ipc->shmem->run_path);
        if (log_path_len > 0 && log_path_len < (int)sizeof(log_path)) {
            wchar_t *wpath = sentry__string_to_wstr(log_path);
            log_file = wpath ? _wfopen(wpath, L"r") : NULL;
            sentry_free(wpath);
        }
#else
        log_path_len = snprintf(log_path, sizeof(log_path),
            "%s/sentry-daemon.log", state->ipc->shmem->run_path);
        if (log_path_len > 0 && log_path_len < (int)sizeof(log_path)) {
            log_file = fopen(log_path, "r");
        }
#endif
        if (log_file) {
            fprintf(stderr, "\n========== Daemon Log ==========\n");
            char line[1024];
            while (fgets(line, sizeof(line), log_file)) {
                fprintf(stderr, "%s", line);
            }
            fprintf(stderr, "================================\n\n");
            fclose(log_file);
        }
    }

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
native_backend_user_consent_changed(sentry_backend_t *backend)
{
    native_backend_state_t *state = (native_backend_state_t *)backend->data;
    if (!state || !state->ipc || !state->ipc->shmem) {
        return;
    }
    SENTRY_WITH_OPTIONS (options) {
        if (options->run) {
            sentry__atomic_store(&state->ipc->shmem->user_consent,
                sentry__atomic_fetch(&options->run->user_consent));
        }
    }
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

/**
 * Creates an attachment path, deriving a unique path in the run directory for
 * buffer attachments.
 */
static sentry_path_t *
make_attachment_path(const sentry_path_t *run_path, sentry_value_t attachment)
{
    if (!sentry__attachment_get_bytes(attachment, NULL)) {
        return sentry__attachment_make_path(attachment);
    }

    sentry_uuid_t id = sentry__attachment_get_id(attachment);
    const char *filename = sentry__attachment_get_filename(attachment);
    if (!run_path || sentry_uuid_is_nil(&id) || !filename) {
        return NULL;
    }

    char uuid[37];
    sentry_uuid_as_string(&id, uuid);
    sentry_path_t *dir = sentry__path_join_str(run_path, uuid);
    sentry_path_t *path = dir ? sentry__path_join_str(dir, filename) : NULL;
    sentry__path_free(dir);
    return path;
}

// Writes the scope's attachment list to <run>/__sentry-attachments so the
// crash daemon can locate and append them to the crash envelope.
static void
native_backend_write_attachments(const sentry_path_t *event_path)
{
    if (!event_path) {
        return;
    }
    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t attachments = scope->attachments;
        if (sentry_value_get_length(attachments) == 0) {
            continue;
        }
        sentry_path_t *run_path = sentry__path_dir(event_path);
        if (!run_path) {
            continue;
        }
        sentry_path_t *attach_list_path
            = sentry__path_join_str(run_path, "__sentry-attachments");
        if (attach_list_path) {
            sentry_value_t attach_list = sentry_value_new_list();
            size_t len = sentry_value_get_length(attachments);
            for (size_t i = 0; i < len; i++) {
                sentry_value_t attachment
                    = sentry_value_get_by_index(attachments, i);
                sentry_path_t *path
                    = make_attachment_path(run_path, attachment);
                if (!path) {
                    continue;
                }
                // skip missing or partially written attachments
                size_t bytes_len = 0;
                if (sentry__attachment_get_bytes(attachment, &bytes_len)
                    && sentry__path_get_size(path) != bytes_len) {
                    sentry__path_free(path);
                    continue;
                }
                sentry_value_t attach_info = sentry_value_new_object();
                sentry_value_set_by_key(
                    attach_info, "path", sentry_value_new_string(path->path));
                const char *filename
                    = sentry__attachment_get_filename(attachment);
                sentry_value_set_by_key(
                    attach_info, "filename", sentry_value_new_string(filename));
                const char *type = sentry__attachment_get_type(attachment);
                if (type && *type) {
                    sentry_value_set_by_key(attach_info, "attachment_type",
                        sentry_value_new_string(type));
                }
                const char *content_type
                    = sentry__attachment_get_content_type(attachment);
                if (content_type) {
                    sentry_value_set_by_key(attach_info, "content_type",
                        sentry_value_new_string(content_type));
                }
                sentry_value_append(attach_list, attach_info);
                sentry__path_free(path);
            }
            size_t attach_json_len = 0;
            char *attach_json
                = sentry__value_to_json(attach_list, &attach_json_len);
            sentry_value_decref(attach_list);
            if (attach_json) {
                sentry__path_write_buffer(
                    attach_list_path, attach_json, attach_json_len);
                sentry_free(attach_json);
            }
            sentry__path_free(attach_list_path);
        }
        sentry__path_free(run_path);
    }
}

#if defined(SENTRY_PLATFORM_WINDOWS)
// Sentry's symbolicator needs `contexts.device.arch` to process PE modules. If
// the scope already carries a device context with arch (host SDKs like Unity
// provide one), leave it; otherwise synthesize a minimal one so native-only
// consumers still symbolicate.
static void
ensure_device_arch(sentry_value_t event)
{
    sentry_value_t contexts = sentry_value_get_by_key(event, "contexts");
    if (sentry_value_is_null(contexts)) {
        contexts = sentry_value_new_object();
        sentry_value_set_by_key(event, "contexts", contexts);
    }
    sentry_value_t device = sentry_value_get_by_key(contexts, "device");
    if (sentry_value_is_null(device)) {
        device = sentry_value_new_object();
        sentry_value_set_by_key(
            device, "type", sentry_value_new_string("device"));
        sentry_value_set_by_key(contexts, "device", device);
    }
    if (!sentry_value_is_null(sentry_value_get_by_key(device, "arch"))) {
        return;
    }
#    if defined(_M_AMD64)
    sentry_value_set_by_key(device, "arch", sentry_value_new_string("x86_64"));
#    elif defined(_M_IX86)
    sentry_value_set_by_key(device, "arch", sentry_value_new_string("x86"));
#    elif defined(_M_ARM64)
    sentry_value_set_by_key(device, "arch", sentry_value_new_string("arm64"));
#    endif
}
#endif

static void
native_backend_flush_scope(
    sentry_backend_t *backend, const sentry_options_t *options)
{
    native_backend_state_t *state = (native_backend_state_t *)backend->data;
    if (!state || !state->event_path) {
        return;
    }

    // Manifest writes must continue post-crash so attachments registered
    // from on_crash/before_send reach the daemon
    native_backend_write_attachments(state->event_path);

    if (sentry__atomic_fetch(&state->crashed)) {
        return;
    }

    // Create event with current scope
    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(
        event, "level", sentry__value_new_level(SENTRY_LEVEL_FATAL));

    // Apply scope with contexts
    SENTRY_WITH_SCOPE (scope) {
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
    }
#if defined(SENTRY_PLATFORM_WINDOWS)
    ensure_device_arch(event);
#endif

    size_t json_len = 0;
    char *json_str = sentry__value_to_json(event, &json_len);
    sentry_value_decref(event);

    if (json_str) {
        sentry__path_write_buffer(state->event_path, json_str, json_len);
        sentry_free(json_str);
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

    // Append as msgpack, matching the crashpad backend. msgpack values are
    // self-delimiting, so the daemon can read the concatenated ring file back
    // into a list via `sentry__value_from_msgpack`.
    size_t mpack_size = 0;
    char *mpack = sentry_value_to_msgpack(breadcrumb, &mpack_size);
    if (!mpack) {
        return;
    }

    int rv = first_breadcrumb
        ? sentry__path_write_buffer(breadcrumb_file, mpack, mpack_size)
        : sentry__path_append_buffer(breadcrumb_file, mpack, mpack_size);

    sentry_free(mpack);

    if (rv != 0) {
        SENTRY_WARN("failed to write breadcrumb");
    }
}

static void
native_backend_add_attachment(sentry_backend_t *backend,
    sentry_value_t attachment, const sentry_options_t *options)
{
    (void)backend; // Unused

    // For buffer attachments, derive a path in the run directory and write to
    // disk
    size_t bytes_len = 0;
    const char *bytes = sentry__attachment_get_bytes(attachment, &bytes_len);
    if (bytes) {
        sentry_path_t *path
            = make_attachment_path(options->run->run_path, attachment);
        if (!path) {
            const char *filename = sentry__attachment_get_filename(attachment);
            SENTRY_WARNF("failed to create path for native backend attachment "
                         "\"%s\"",
                filename ? filename : "<unknown>");
            return;
        }
        sentry_path_t *dir = sentry__path_dir(path);
        int rv = dir ? sentry__path_create_dir_all(dir) : 1;
        sentry__path_free(dir);
        // Write buffer to disk
        if (rv != 0 || sentry__path_write_buffer(path, bytes, bytes_len) != 0) {
            SENTRY_WARNF(
                "failed to write native backend attachment \"%s\"", path->path);
            sentry__path_remove(path);
        }
        sentry__path_free(path);
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
    native_backend_state_t *state = (native_backend_state_t *)backend->data;
    if (state) {
        sentry__atomic_store(&state->crashed, 1);
    }

    SENTRY_WITH_OPTIONS (options) {
        // Disable logging during crash handling if configured
        if (!options->enable_logging_when_crashed) {
            sentry__logger_disable();
        }

        SENTRY_DEBUG("handling native backend exception");

        sentry__transport_suspend(options->transport);

        // Flush logs and metrics in a crash-safe manner before crash handling
        sentry__telemetry_flush_crash_safe();

        // Write crash marker
        sentry__write_crash_marker(options);

        sentry_value_t transaction
            = sentry__trace_finish(SENTRY_SPAN_STATUS_ABORTED);

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
            // Apply before_send hook if on_crash wasn't set
            if (!options->on_crash_func && options->before_send_func) {
                SENTRY_DEBUG("invoking `before_send` hook");
                event = options->before_send_func(
                    event, NULL, options->before_send_data);
                should_handle = !sentry_value_is_null(event);
            }

            if (should_handle) {
                // Apply scope to the event. The daemon assembles breadcrumbs
                // from the ring files
                SENTRY_WITH_SCOPE (scope) {
                    sentry__scope_apply_to_event(
                        scope, options, event, SENTRY_SCOPE_NONE);
                }
#if defined(SENTRY_PLATFORM_WINDOWS)
                ensure_device_arch(event);
#endif

#ifndef SENTRY_SCREENSHOT_NONE
                // The screenshot is captured by the daemon out-of-process, so
                // we invoke the hook here (in the crashing process, where
                // user callbacks can run) and communicate the decision to the
                // daemon by flipping attach_screenshot in the shared crash
                // context. Screenshots are only captured on Windows.
                if (options->attach_screenshot
                    && options->before_screenshot_func && state && state->ipc
                    && state->ipc->shmem) {
                    SENTRY_DEBUG("invoking `before_screenshot` hook");
                    if (options->before_screenshot_func(
                            event, options->before_screenshot_data)
                        == 0) {
                        SENTRY_DEBUG("screenshot skipped by "
                                     "`before_screenshot` hook");
                        state->ipc->shmem->attach_screenshot = false;
                    }
                }
#endif

                // Write event as JSON file
                // Daemon will read this and create envelope with minidump
                if (state && state->event_path) {
                    size_t event_json_len = 0;
                    char *event_json
                        = sentry__value_to_json(event, &event_json_len);
                    if (event_json) {
                        int rv = sentry__path_write_buffer(
                            state->event_path, event_json, event_json_len);
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

                if (session || !sentry_value_is_null(transaction)) {
                    if (!sentry_value_is_null(transaction)) {
                        sentry_envelope_t *tx_envelope
                            = sentry__prepare_transaction(
                                options, transaction, NULL);
                        if (tx_envelope) {
                            sentry__capture_envelope(
                                options->transport, tx_envelope, options);
                        }
                    }
                    if (session) {
                        sentry_envelope_t *envelope = sentry__envelope_new();
                        if (envelope) {
                            sentry__envelope_add_session(envelope, session);
                            sentry__capture_envelope(
                                options->transport, envelope, options);
                        }
                    }
                }

                // Dump any pending transport queue
                sentry__transport_dump_queue(options->transport, options->run);

                SENTRY_DEBUG("crash event and session written, daemon will "
                             "create and send minidump");
            } else {
                sentry_value_decref(transaction);
            }
        } else {
            SENTRY_DEBUG("event was discarded by the `on_crash` hook");
            sentry_value_decref(event);
            sentry_value_decref(transaction);
        }
    }
}

void
sentry__backend_preload(void)
{
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

    backend->startup_func = native_backend_startup;
    backend->shutdown_func = native_backend_shutdown;
    backend->free_func = native_backend_free;
    backend->except_func = native_backend_except;
    backend->flush_scope_func = native_backend_flush_scope;
    backend->add_breadcrumb_func = native_backend_add_breadcrumb;
    backend->add_attachment_func = native_backend_add_attachment;
    backend->user_consent_changed_func = native_backend_user_consent_changed;
    backend->process_old_run_func = native_backend_process_old_run;
    backend->can_capture_after_shutdown = false;

    return backend;
}
