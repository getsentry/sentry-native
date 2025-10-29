#ifndef SENTRY_CRASH_IPC_H_INCLUDED
#define SENTRY_CRASH_IPC_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_crash_context.h"

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    include <semaphore.h>
#    include <sys/eventfd.h>
#    include <sys/mman.h>
#elif defined(SENTRY_PLATFORM_MACOS)
#    include <mach/mach.h>
#    include <semaphore.h>
#    include <sys/mman.h>
#elif defined(SENTRY_PLATFORM_WINDOWS)
#    include <windows.h>
#endif

/**
 * IPC handle for crash communication between app and daemon
 */
typedef struct {
    sentry_crash_context_t *shmem;

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    int shm_fd;
    int eventfd;
    char shm_name[SENTRY_CRASH_IPC_NAME_SIZE];
    sem_t *init_sem;        // Named semaphore for initialization synchronization
    char sem_name[SENTRY_CRASH_IPC_NAME_SIZE];
#elif defined(SENTRY_PLATFORM_MACOS)
    int shm_fd;
    int notify_pipe[2]; // Pipe for crash notifications (fork-safe)
    char shm_name[SENTRY_CRASH_IPC_NAME_SIZE];
    sem_t *init_sem;        // Named semaphore for initialization synchronization
    char sem_name[SENTRY_CRASH_IPC_NAME_SIZE];
#elif defined(SENTRY_PLATFORM_WINDOWS)
    HANDLE shm_handle;
    HANDLE event_handle;
    wchar_t shm_name[SENTRY_CRASH_IPC_NAME_SIZE];
    wchar_t event_name[SENTRY_CRASH_IPC_NAME_SIZE];
    HANDLE init_mutex;      // Named mutex for initialization synchronization
#endif

    bool is_daemon; // true if this is the daemon side of IPC
} sentry_crash_ipc_t;

/**
 * Initialize IPC for application process.
 * Creates shared memory and notification mechanism.
 * @param init_sem Optional semaphore for synchronizing init (can be NULL)
 * @param init_mutex Optional mutex for synchronizing init on Windows (can be NULL)
 */
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID) \
    || defined(SENTRY_PLATFORM_MACOS)
sentry_crash_ipc_t *sentry__crash_ipc_init_app(sem_t *init_sem);
#elif defined(SENTRY_PLATFORM_WINDOWS)
sentry_crash_ipc_t *sentry__crash_ipc_init_app(HANDLE init_mutex);
#else
sentry_crash_ipc_t *sentry__crash_ipc_init_app(void);
#endif

/**
 * Initialize IPC for daemon process.
 * Attaches to existing shared memory created by app.
 */
sentry_crash_ipc_t *sentry__crash_ipc_init_daemon(pid_t app_pid);

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
 * Clean up IPC resources.
 */
void sentry__crash_ipc_free(sentry_crash_ipc_t *ipc);

#endif
