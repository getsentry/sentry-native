#ifndef SENTRY_CRASH_IPC_H_INCLUDED
#define SENTRY_CRASH_IPC_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_crash_context.h"

#include <stddef.h>
#include <stdint.h>

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    include <semaphore.h>
#    include <sys/eventfd.h>
#    include <sys/mman.h>
#elif defined(SENTRY_PLATFORM_MACOS)
#    include "sentry_sync.h"
#    include <mach/mach.h>
#    include <sys/mman.h>
#elif defined(SENTRY_PLATFORM_WINDOWS)
#    include <windows.h>
#endif

#define SENTRY_CRASH_IPC_MESSAGE_PREFIX_SIZE 4u
#define SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE 16u
#define SENTRY_CRASH_IPC_MESSAGE_MIN_LEN 12u
#define SENTRY_CRASH_IPC_MESSAGE_MAX_LEN (1024u * 1024u)

typedef enum {
    SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT = 1,
    SENTRY_CRASH_IPC_MESSAGE_SET_RELEASE = 2,
    SENTRY_CRASH_IPC_MESSAGE_SET_ENVIRONMENT = 3,
    SENTRY_CRASH_IPC_MESSAGE_SET_TRANSACTION = 4,
    SENTRY_CRASH_IPC_MESSAGE_SET_FINGERPRINT = 5,
    SENTRY_CRASH_IPC_MESSAGE_SET_LEVEL = 6,
    SENTRY_CRASH_IPC_MESSAGE_SET_USER = 7,
    SENTRY_CRASH_IPC_MESSAGE_SET_TAG = 8,
    SENTRY_CRASH_IPC_MESSAGE_REMOVE_TAG = 9,
    SENTRY_CRASH_IPC_MESSAGE_SET_EXTRA = 10,
    SENTRY_CRASH_IPC_MESSAGE_REMOVE_EXTRA = 11,
    SENTRY_CRASH_IPC_MESSAGE_SET_CONTEXT = 12,
    SENTRY_CRASH_IPC_MESSAGE_REMOVE_CONTEXT = 13,
    SENTRY_CRASH_IPC_MESSAGE_ADD_BREADCRUMB = 14,
    SENTRY_CRASH_IPC_MESSAGE_SET_ATTACHMENT_LIST = 15,
    SENTRY_CRASH_IPC_MESSAGE_CRASH = 16,
    SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN = 17,
} sentry_crash_ipc_message_type_t;

typedef enum {
    SENTRY_CRASH_IPC_MESSAGE_OK = 0,
    SENTRY_CRASH_IPC_MESSAGE_PARTIAL = 1,
    SENTRY_CRASH_IPC_MESSAGE_INVALID = 2,
    SENTRY_CRASH_IPC_MESSAGE_OVERSIZED = 3,
    SENTRY_CRASH_IPC_MESSAGE_UNKNOWN_TYPE = 4,
    SENTRY_CRASH_IPC_MESSAGE_OOM = 5,
} sentry_crash_ipc_message_result_t;

typedef struct {
    uint16_t type;
    uint16_t flags;
    uint64_t sequence;
    const char *payload;
    size_t payload_len;
} sentry_crash_ipc_message_t;

bool sentry__crash_ipc_message_type_is_known(uint16_t type);

sentry_crash_ipc_message_result_t sentry__crash_ipc_message_encode(
    uint16_t type, uint16_t flags, uint64_t sequence, const char *payload,
    size_t payload_len, char **out_buf, size_t *out_len);

sentry_crash_ipc_message_result_t sentry__crash_ipc_message_decode(
    const char *buf, size_t buf_len, sentry_crash_ipc_message_t *message);

#if defined(SENTRY_PLATFORM_WINDOWS)
typedef HANDLE sentry_process_handle_t;
#else
typedef pid_t sentry_process_handle_t;
#endif

/**
 * IPC handle for crash communication between app and daemon
 */
typedef struct {
    sentry_crash_context_t *shmem;

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    int shm_fd;
    int notify_fd; // Eventfd for crash notifications
    int ready_fd; // Eventfd for daemon ready signal
    // Anonymous app-to-daemon message stream: app writes, daemon reads.
    int message_fd[2];
    char shm_name[SENTRY_CRASH_IPC_NAME_SIZE];
    sem_t *init_sem; // Named semaphore for initialization synchronization
    char sem_name[SENTRY_CRASH_IPC_NAME_SIZE];
#elif defined(SENTRY_PLATFORM_MACOS)
    int shm_fd;
    int notify_pipe[2]; // Pipe for crash notifications (fork-safe)
    int ready_pipe[2]; // Pipe for daemon ready signal (fork-safe)
    int message_fd[2]; // Anonymous app-to-daemon message stream: app writes,
                       // daemon reads
    char shm_path[SENTRY_CRASH_MAX_PATH]; // File-backed shm path (sandbox-safe)
    sentry_mutex_t
        *init_mutex; // Process-wide mutex (sandbox-safe, no sem_open)
#elif defined(SENTRY_PLATFORM_WINDOWS)
    HANDLE shm_handle;
    HANDLE event_handle; // Event for crash notifications (parent -> daemon)
    HANDLE ready_event_handle; // Event for ready signal (daemon -> parent)
    HANDLE message_read_handle; // Anonymous app-to-daemon message stream read
                                // end
    HANDLE message_write_handle; // Anonymous app-to-daemon message stream write
                                 // end
    wchar_t shm_name[SENTRY_CRASH_IPC_NAME_SIZE];
    wchar_t event_name[SENTRY_CRASH_IPC_NAME_SIZE];
    wchar_t ready_event_name[SENTRY_CRASH_IPC_NAME_SIZE];
    HANDLE init_mutex; // Named mutex for initialization synchronization
#endif

    sentry_process_handle_t parent_handle;
    bool is_daemon; // true if this is the daemon side of IPC
} sentry_crash_ipc_t;

/**
 * Initialize IPC for application process.
 * Creates shared memory and notification mechanism.
 * @param init_sem Optional semaphore for synchronizing init (can be NULL)
 * @param init_mutex Optional mutex for synchronizing init on Windows (can be
 * NULL)
 */
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
sentry_crash_ipc_t *sentry__crash_ipc_init_app(sem_t *init_sem);
#elif defined(SENTRY_PLATFORM_MACOS)
sentry_crash_ipc_t *sentry__crash_ipc_init_app(sentry_mutex_t *init_mutex);
#elif defined(SENTRY_PLATFORM_WINDOWS)
sentry_crash_ipc_t *sentry__crash_ipc_init_app(HANDLE init_mutex);
#else
sentry_crash_ipc_t *sentry__crash_ipc_init_app(void);
#endif

/**
 * Initialize IPC for daemon process.
 * Attaches to existing shared memory created by app.
 * @param app_pid Parent process ID
 * @param app_tid Parent thread ID
 * @param notify_handle Notification handle inherited from parent (eventfd on
 * Linux, pipe fd on macOS, event on Windows)
 * @param ready_handle Ready signal handle inherited from parent (eventfd on
 * Linux, pipe fd on macOS, event on Windows)
 */
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
sentry_crash_ipc_t *sentry__crash_ipc_init_daemon(pid_t app_pid,
    uint64_t app_tid, int notify_eventfd, int ready_eventfd, int message_fd);
#elif defined(SENTRY_PLATFORM_MACOS)
sentry_crash_ipc_t *sentry__crash_ipc_init_daemon(pid_t app_pid,
    uint64_t app_tid, int notify_pipe_read, int ready_pipe_write, int shm_fd,
    int message_fd);
#elif defined(SENTRY_PLATFORM_WINDOWS)
sentry_crash_ipc_t *sentry__crash_ipc_init_daemon(pid_t app_pid,
    uint64_t app_tid, HANDLE event_handle, HANDLE ready_event_handle,
    HANDLE message_read_handle);
#endif

/**
 * Signal that daemon is ready (called by daemon after initialization).
 */
void sentry__crash_ipc_signal_ready(sentry_crash_ipc_t *ipc);

/**
 * Wait for daemon to signal ready (called by parent after spawning daemon).
 * Returns true if daemon signaled ready, false on timeout or error.
 */
bool sentry__crash_ipc_wait_for_ready(sentry_crash_ipc_t *ipc, int timeout_ms);

/**
 * Notify daemon that a crash occurred (called from signal handler).
 * This function is signal-safe.
 */
void sentry__crash_ipc_notify(sentry_crash_ipc_t *ipc);

/**
 * Wait for crash notification (called by daemon).
 * Blocks until a crash is signaled or timeout expires.
 * Returns true if crash occurred, false on timeout.
 */
bool sentry__crash_ipc_wait(sentry_crash_ipc_t *ipc, int timeout_ms);

/**
 * Unlink the shared memory.
 */
void sentry__crash_ipc_unlink(sentry_crash_ipc_t *ipc);

/**
 * Clean up IPC resources.
 */
void sentry__crash_ipc_free(sentry_crash_ipc_t *ipc);

#endif
