#include "sentry_crash_ipc.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_sync.h"

#include <stdio.h>
#include <string.h>

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)

#    include <errno.h>
#    include <fcntl.h>
#    include <sys/file.h>
#    include <unistd.h>

sentry_crash_ipc_t *
sentry__crash_ipc_init_app(sem_t *init_sem)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    memset(ipc, 0, sizeof(sentry_crash_ipc_t));
    ipc->is_daemon = false;
    ipc->init_sem = init_sem; // Use provided semaphore (managed by backend)

    // Create shared memory with unique name based on PID
    snprintf(ipc->shm_name, sizeof(ipc->shm_name), "/sentry-crash-%d",
        (int)getpid());

    // Acquire semaphore for exclusive access during initialization
    if (ipc->init_sem && sem_wait(ipc->init_sem) < 0) {
        SENTRY_WARNF(
            "failed to acquire initialization semaphore: %s", strerror(errno));
        sentry_free(ipc);
        return NULL;
    }

    // Try to create or open shared memory
    bool shm_exists = false;
    ipc->shm_fd = shm_open(ipc->shm_name, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (ipc->shm_fd < 0 && errno == EEXIST) {
        // Shared memory already exists - reuse it
        shm_exists = true;
        ipc->shm_fd = shm_open(ipc->shm_name, O_RDWR, 0600);
    }

    if (ipc->shm_fd < 0) {
        SENTRY_WARNF("failed to open shared memory: %s", strerror(errno));
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Set shared memory size (only if newly created)
    if (!shm_exists && ftruncate(ipc->shm_fd, SENTRY_CRASH_SHM_SIZE) < 0) {
        SENTRY_WARNF("failed to resize shared memory: %s", strerror(errno));
        close(ipc->shm_fd);
        shm_unlink(ipc->shm_name);
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Map shared memory
    ipc->shmem = mmap(NULL, SENTRY_CRASH_SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, ipc->shm_fd, 0);
    if (ipc->shmem == MAP_FAILED) {
        SENTRY_WARNF("failed to map shared memory: %s", strerror(errno));
        close(ipc->shm_fd);
        if (!shm_exists) {
            shm_unlink(ipc->shm_name);
        }
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Create eventfd for crash notifications
    ipc->eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (ipc->eventfd < 0) {
        SENTRY_WARNF("failed to create eventfd: %s", strerror(errno));
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        if (!shm_exists) {
            shm_unlink(ipc->shm_name);
        }
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Create eventfd for daemon ready signal
    ipc->ready_eventfd = eventfd(0, EFD_CLOEXEC);
    if (ipc->ready_eventfd < 0) {
        SENTRY_WARNF("failed to create ready eventfd: %s", strerror(errno));
        close(ipc->eventfd);
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        if (!shm_exists) {
            shm_unlink(ipc->shm_name);
        }
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Initialize shared memory only if newly created
    if (!shm_exists) {
        memset(ipc->shmem, 0, SENTRY_CRASH_SHM_SIZE);
        ipc->shmem->magic = SENTRY_CRASH_MAGIC;
        ipc->shmem->version = SENTRY_CRASH_VERSION;
        sentry__atomic_store(&ipc->shmem->state, SENTRY_CRASH_STATE_READY);
        sentry__atomic_store(&ipc->shmem->sequence, 0);
    }

    // Release semaphore after initialization
    if (ipc->init_sem) {
        sem_post(ipc->init_sem);
    }

    SENTRY_DEBUGF("initialized crash IPC (shm=%s, eventfd=%d)", ipc->shm_name,
        ipc->eventfd);

    return ipc;
}

sentry_crash_ipc_t *
sentry__crash_ipc_init_daemon(pid_t app_pid, int notify_eventfd, int ready_eventfd)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    memset(ipc, 0, sizeof(sentry_crash_ipc_t));
    ipc->is_daemon = true;

    // Open existing shared memory created by app
    snprintf(
        ipc->shm_name, sizeof(ipc->shm_name), "/sentry-crash-%d", (int)app_pid);

    ipc->shm_fd = shm_open(ipc->shm_name, O_RDWR, 0600);
    if (ipc->shm_fd < 0) {
        SENTRY_WARNF(
            "daemon: failed to open shared memory: %s", strerror(errno));
        sentry_free(ipc);
        return NULL;
    }

    // Map shared memory
    ipc->shmem = mmap(NULL, SENTRY_CRASH_SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, ipc->shm_fd, 0);
    if (ipc->shmem == MAP_FAILED) {
        SENTRY_WARNF(
            "daemon: failed to map shared memory: %s", strerror(errno));
        close(ipc->shm_fd);
        sentry_free(ipc);
        return NULL;
    }

    // Validate shared memory
    if (ipc->shmem->magic != SENTRY_CRASH_MAGIC) {
        SENTRY_WARN("daemon: invalid shared memory magic");
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        sentry_free(ipc);
        return NULL;
    }

    // Eventfds are inherited from parent after fork - assign them
    ipc->eventfd = notify_eventfd;
    ipc->ready_eventfd = ready_eventfd;

    SENTRY_DEBUGF("daemon: attached to crash IPC (shm=%s, eventfd=%d, ready_eventfd=%d)",
        ipc->shm_name, notify_eventfd, ready_eventfd);

    return ipc;
}

void
sentry__crash_ipc_notify(sentry_crash_ipc_t *ipc)
{
    if (!ipc || ipc->eventfd < 0) {
        return;
    }

    // Write to eventfd to wake up daemon
    // This is signal-safe
    uint64_t val = 1;
    ssize_t written = write(ipc->eventfd, &val, sizeof(val));
    (void)written; // Ignore errors in signal handler
}

bool
sentry__crash_ipc_wait(sentry_crash_ipc_t *ipc, int timeout_ms)
{
    if (!ipc || ipc->eventfd < 0) {
        return false;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ipc->eventfd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(ipc->eventfd + 1, &readfds, NULL, NULL,
        timeout_ms >= 0 ? &timeout : NULL);

    if (ret > 0 && FD_ISSET(ipc->eventfd, &readfds)) {
        uint64_t val;
        read(ipc->eventfd, &val, sizeof(val));
        return true;
    }

    return false;
}

void
sentry__crash_ipc_free(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }

    if (ipc->shmem && ipc->shmem != MAP_FAILED) {
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
    }

    if (ipc->shm_fd >= 0) {
        close(ipc->shm_fd);
    }

    if (!ipc->is_daemon && ipc->shm_name[0]) {
        shm_unlink(ipc->shm_name);
    }

    if (ipc->eventfd >= 0) {
        close(ipc->eventfd);
    }

    if (ipc->ready_eventfd >= 0) {
        close(ipc->ready_eventfd);
    }

    sentry_free(ipc);
}

#elif defined(SENTRY_PLATFORM_MACOS)

#    include <errno.h>
#    include <fcntl.h>
#    include <sys/file.h>
#    include <unistd.h>

sentry_crash_ipc_t *
sentry__crash_ipc_init_app(sem_t *init_sem)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    memset(ipc, 0, sizeof(sentry_crash_ipc_t));
    ipc->is_daemon = false;
    ipc->init_sem = init_sem; // Use provided semaphore (managed by backend)

    // Create shared memory
    snprintf(ipc->shm_name, sizeof(ipc->shm_name), "/sentry-crash-%d",
        (int)getpid());

    // Acquire semaphore for exclusive access during initialization
    if (ipc->init_sem && sem_wait(ipc->init_sem) < 0) {
        SENTRY_WARNF(
            "failed to acquire initialization semaphore: %s", strerror(errno));
        sentry_free(ipc);
        return NULL;
    }

    // Try to create or open shared memory
    bool shm_exists = false;
    ipc->shm_fd = shm_open(ipc->shm_name, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (ipc->shm_fd < 0 && errno == EEXIST) {
        // Shared memory already exists - reuse it
        shm_exists = true;
        ipc->shm_fd = shm_open(ipc->shm_name, O_RDWR, 0600);
    }

    if (ipc->shm_fd < 0) {
        SENTRY_WARNF("failed to open shared memory: %s", strerror(errno));
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    if (!shm_exists && ftruncate(ipc->shm_fd, SENTRY_CRASH_SHM_SIZE) < 0) {
        SENTRY_WARNF("failed to resize shared memory: %s", strerror(errno));
        close(ipc->shm_fd);
        shm_unlink(ipc->shm_name);
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    ipc->shmem = mmap(NULL, SENTRY_CRASH_SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, ipc->shm_fd, 0);
    if (ipc->shmem == MAP_FAILED) {
        SENTRY_WARNF("failed to map shared memory: %s", strerror(errno));
        close(ipc->shm_fd);
        if (!shm_exists) {
            shm_unlink(ipc->shm_name);
        }
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Create pipe for crash notifications (works across fork)
    if (pipe(ipc->notify_pipe) < 0) {
        SENTRY_WARNF("failed to create notification pipe: %s", strerror(errno));
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        if (!shm_exists) {
            shm_unlink(ipc->shm_name);
        }
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Make write end non-blocking for signal-safe writes
    fcntl(ipc->notify_pipe[1], F_SETFL, O_NONBLOCK);

    // Create pipe for daemon ready signal (works across fork)
    if (pipe(ipc->ready_pipe) < 0) {
        SENTRY_WARNF("failed to create ready pipe: %s", strerror(errno));
        close(ipc->notify_pipe[0]);
        close(ipc->notify_pipe[1]);
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        if (!shm_exists) {
            shm_unlink(ipc->shm_name);
        }
        if (ipc->init_sem) {
            sem_post(ipc->init_sem);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Initialize shared memory only if newly created
    if (!shm_exists) {
        memset(ipc->shmem, 0, SENTRY_CRASH_SHM_SIZE);
        ipc->shmem->magic = SENTRY_CRASH_MAGIC;
        ipc->shmem->version = SENTRY_CRASH_VERSION;
        sentry__atomic_store(&ipc->shmem->state, SENTRY_CRASH_STATE_READY);
        sentry__atomic_store(&ipc->shmem->sequence, 0);
    }

    // Release semaphore after initialization
    if (ipc->init_sem) {
        sem_post(ipc->init_sem);
    }

    SENTRY_DEBUGF("initialized crash IPC (shm=%s, pipe=%d/%d)", ipc->shm_name,
        ipc->notify_pipe[0], ipc->notify_pipe[1]);

    return ipc;
}

sentry_crash_ipc_t *
sentry__crash_ipc_init_daemon(pid_t app_pid, int notify_pipe_read, int ready_pipe_write)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    memset(ipc, 0, sizeof(sentry_crash_ipc_t));
    ipc->is_daemon = true;

    snprintf(
        ipc->shm_name, sizeof(ipc->shm_name), "/sentry-crash-%d", (int)app_pid);

    ipc->shm_fd = shm_open(ipc->shm_name, O_RDWR, 0600);
    if (ipc->shm_fd < 0) {
        SENTRY_WARNF(
            "daemon: failed to open shared memory: %s", strerror(errno));
        sentry_free(ipc);
        return NULL;
    }

    ipc->shmem = mmap(NULL, SENTRY_CRASH_SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, ipc->shm_fd, 0);
    if (ipc->shmem == MAP_FAILED) {
        SENTRY_WARNF(
            "daemon: failed to map shared memory: %s", strerror(errno));
        close(ipc->shm_fd);
        sentry_free(ipc);
        return NULL;
    }

    if (ipc->shmem->magic != SENTRY_CRASH_MAGIC) {
        SENTRY_WARN("daemon: invalid shared memory magic");
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        sentry_free(ipc);
        return NULL;
    }

    // Pipes are inherited from parent after fork - assign the fds
    ipc->notify_pipe[0] = notify_pipe_read;
    ipc->notify_pipe[1] = -1; // Daemon doesn't write to notify pipe
    ipc->ready_pipe[0] = -1;  // Daemon doesn't read from ready pipe
    ipc->ready_pipe[1] = ready_pipe_write;

    SENTRY_DEBUGF("daemon: attached to crash IPC (shm=%s, notify_pipe=%d, ready_pipe=%d)",
        ipc->shm_name, notify_pipe_read, ready_pipe_write);

    return ipc;
}

void
sentry__crash_ipc_notify(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }

    // Write byte to pipe (signal-safe)
    char byte = 1;
    write(ipc->notify_pipe[1], &byte, 1);
}

bool
sentry__crash_ipc_wait(sentry_crash_ipc_t *ipc, int timeout_ms)
{
    if (!ipc) {
        return false;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ipc->notify_pipe[0], &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(ipc->notify_pipe[0] + 1, &readfds, NULL, NULL,
        timeout_ms >= 0 ? &timeout : NULL);

    if (result > 0) {
        // Read and discard the byte
        char byte;
        read(ipc->notify_pipe[0], &byte, 1);
        return true;
    }

    return false;
}

void
sentry__crash_ipc_free(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }

    if (ipc->shmem && ipc->shmem != MAP_FAILED) {
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
    }

    if (ipc->shm_fd >= 0) {
        close(ipc->shm_fd);
    }

    // Close pipes
    if (ipc->notify_pipe[0] >= 0) {
        close(ipc->notify_pipe[0]);
    }
    if (ipc->notify_pipe[1] >= 0) {
        close(ipc->notify_pipe[1]);
    }

    // Close ready pipes
    if (ipc->ready_pipe[0] >= 0) {
        close(ipc->ready_pipe[0]);
    }
    if (ipc->ready_pipe[1] >= 0) {
        close(ipc->ready_pipe[1]);
    }

    if (!ipc->is_daemon && ipc->shm_name[0]) {
        shm_unlink(ipc->shm_name);
    }

    sentry_free(ipc);
}

#elif defined(SENTRY_PLATFORM_WINDOWS)

sentry_crash_ipc_t *
sentry__crash_ipc_init_app(HANDLE init_mutex)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    memset(ipc, 0, sizeof(sentry_crash_ipc_t));
    ipc->is_daemon = false;
    ipc->init_mutex = init_mutex; // Use provided mutex (managed by backend)

    // Create named shared memory
    swprintf(ipc->shm_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrash-%lu", GetCurrentProcessId());

    // Log the shared memory name
    char *shm_name_utf8 = sentry__string_from_wstr(ipc->shm_name);
    if (shm_name_utf8) {
        SENTRY_DEBUGF("APP: Creating shared memory: %s", shm_name_utf8);
        sentry_free(shm_name_utf8);
    }

    // Acquire mutex for exclusive access during initialization
    if (ipc->init_mutex) {
        DWORD result = WaitForSingleObject(ipc->init_mutex, INFINITE);
        if (result != WAIT_OBJECT_0) {
            SENTRY_WARNF(
                "failed to acquire initialization mutex: %lu", GetLastError());
            sentry_free(ipc);
            return NULL;
        }
    }

    // Try to create or open shared memory
    bool shm_exists = false;
    ipc->shm_handle = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, 0, SENTRY_CRASH_SHM_SIZE, ipc->shm_name);
    if (!ipc->shm_handle) {
        SENTRY_WARNF("failed to create shared memory: %lu", GetLastError());
        if (ipc->init_mutex) {
            ReleaseMutex(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Check if shared memory already existed
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        shm_exists = true;
    }

    ipc->shmem = MapViewOfFile(
        ipc->shm_handle, FILE_MAP_ALL_ACCESS, 0, 0, SENTRY_CRASH_SHM_SIZE);
    if (!ipc->shmem) {
        SENTRY_WARNF("failed to map shared memory: %lu", GetLastError());
        CloseHandle(ipc->shm_handle);
        if (ipc->init_mutex) {
            ReleaseMutex(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Create named event for notifications
    swprintf(ipc->event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashEvent-%lu", GetCurrentProcessId());

    // Log the event name
    char *event_name_utf8 = sentry__string_from_wstr(ipc->event_name);
    if (event_name_utf8) {
        SENTRY_DEBUGF("APP: Creating event: %s", event_name_utf8);
        sentry_free(event_name_utf8);
    }

    ipc->event_handle = CreateEventW(NULL, FALSE, FALSE, ipc->event_name);
    if (!ipc->event_handle) {
        SENTRY_WARNF("failed to create event: %lu", GetLastError());
        UnmapViewOfFile(ipc->shmem);
        CloseHandle(ipc->shm_handle);
        if (ipc->init_mutex) {
            ReleaseMutex(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Create ready event for daemon to signal when it's initialized
    swprintf(ipc->ready_event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashReady-%lu", GetCurrentProcessId());
    ipc->ready_event_handle = CreateEventW(
        NULL, TRUE, FALSE, ipc->ready_event_name); // Manual-reset
    if (!ipc->ready_event_handle) {
        SENTRY_WARNF("failed to create ready event: %lu", GetLastError());
        CloseHandle(ipc->event_handle);
        UnmapViewOfFile(ipc->shmem);
        CloseHandle(ipc->shm_handle);
        if (ipc->init_mutex) {
            ReleaseMutex(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Initialize shared memory only if newly created
    if (!shm_exists) {
        memset(ipc->shmem, 0, SENTRY_CRASH_SHM_SIZE);
        ipc->shmem->magic = SENTRY_CRASH_MAGIC;
        ipc->shmem->version = SENTRY_CRASH_VERSION;
        sentry__atomic_store(&ipc->shmem->state, SENTRY_CRASH_STATE_READY);
        sentry__atomic_store(&ipc->shmem->sequence, 0);
    }

    // Release mutex after initialization
    if (ipc->init_mutex) {
        ReleaseMutex(ipc->init_mutex);
    }

    SENTRY_DEBUG("initialized crash IPC");

    return ipc;
}

sentry_crash_ipc_t *
sentry__crash_ipc_init_daemon(pid_t app_pid)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    memset(ipc, 0, sizeof(sentry_crash_ipc_t));
    ipc->is_daemon = true;

    // Open existing shared memory
    swprintf(ipc->shm_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrash-%lu", (unsigned long)app_pid);

    ipc->shm_handle
        = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, ipc->shm_name);
    if (!ipc->shm_handle) {
        SENTRY_WARNF(
            "daemon: failed to open shared memory: %lu", GetLastError());
        sentry_free(ipc);
        return NULL;
    }

    ipc->shmem = MapViewOfFile(
        ipc->shm_handle, FILE_MAP_ALL_ACCESS, 0, 0, SENTRY_CRASH_SHM_SIZE);
    if (!ipc->shmem) {
        SENTRY_WARNF(
            "daemon: failed to map shared memory: %lu", GetLastError());
        CloseHandle(ipc->shm_handle);
        sentry_free(ipc);
        return NULL;
    }

    if (ipc->shmem->magic != SENTRY_CRASH_MAGIC) {
        SENTRY_WARN("daemon: invalid shared memory magic");
        UnmapViewOfFile(ipc->shmem);
        CloseHandle(ipc->shm_handle);
        sentry_free(ipc);
        return NULL;
    }

    // Open existing event
    swprintf(ipc->event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashEvent-%lu", (unsigned long)app_pid);

    ipc->event_handle = OpenEventW(SYNCHRONIZE, FALSE, ipc->event_name);
    if (!ipc->event_handle) {
        SENTRY_WARNF("daemon: failed to open event: %lu", GetLastError());
        UnmapViewOfFile(ipc->shmem);
        CloseHandle(ipc->shm_handle);
        sentry_free(ipc);
        return NULL;
    }

    // Open ready event to signal when daemon is initialized
    swprintf(ipc->ready_event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashReady-%lu", (unsigned long)app_pid);
    ipc->ready_event_handle
        = OpenEventW(EVENT_MODIFY_STATE, FALSE, ipc->ready_event_name);
    if (!ipc->ready_event_handle) {
        SENTRY_WARNF("daemon: failed to open ready event: %lu", GetLastError());
        CloseHandle(ipc->event_handle);
        UnmapViewOfFile(ipc->shmem);
        CloseHandle(ipc->shm_handle);
        sentry_free(ipc);
        return NULL;
    }

    SENTRY_DEBUG("daemon: attached to crash IPC");

    return ipc;
}

void
sentry__crash_ipc_notify(sentry_crash_ipc_t *ipc)
{
    if (!ipc || !ipc->event_handle) {
        SENTRY_WARN("crash_ipc_notify: ipc or event_handle is NULL!");
        return;
    }

    if (!SetEvent(ipc->event_handle)) {
        SENTRY_WARNF("crash_ipc_notify: SetEvent failed: %lu", GetLastError());
    } else {
        // Do nothing
    }
}

bool
sentry__crash_ipc_wait(sentry_crash_ipc_t *ipc, int timeout_ms)
{
    if (!ipc || !ipc->event_handle) {
        SENTRY_WARN("crash_ipc_wait: ipc or event_handle is NULL");
        return false;
    }

    DWORD timeout = (timeout_ms >= 0) ? (DWORD)timeout_ms : INFINITE;
    DWORD result = WaitForSingleObject(ipc->event_handle, timeout);

    if (result == WAIT_OBJECT_0) {
        return true;
    } else if (result == WAIT_TIMEOUT) {
        return false;
    } else {
        SENTRY_WARNF("crash_ipc_wait: unexpected result %lu, error %lu", result,
            GetLastError());
        return false;
    }
}

void
sentry__crash_ipc_free(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }

    if (ipc->shmem) {
        UnmapViewOfFile(ipc->shmem);
    }

    if (ipc->shm_handle) {
        CloseHandle(ipc->shm_handle);
    }

    if (ipc->event_handle) {
        CloseHandle(ipc->event_handle);
    }

    sentry_free(ipc);
}

#endif

// Cross-platform ready signaling functions
void
sentry__crash_ipc_signal_ready(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        SENTRY_WARN("signal_ready: ipc is NULL");
        return;
    }

#if defined(SENTRY_PLATFORM_WINDOWS)
    if (!ipc->ready_event_handle) {
        SENTRY_WARN("signal_ready: ready_event_handle is NULL");
        return;
    }
    if (!SetEvent(ipc->ready_event_handle)) {
        SENTRY_WARNF("daemon: SetEvent failed: %lu", GetLastError());
    } else {
        SENTRY_DEBUG("daemon: Successfully signaled ready to parent");
    }
#elif defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    // Signal via eventfd
    uint64_t val = 1;
    if (write(ipc->ready_eventfd, &val, sizeof(val)) < 0) {
        SENTRY_WARNF("daemon: write to ready_eventfd failed: %s", strerror(errno));
    } else {
        SENTRY_DEBUG("daemon: signaled ready to parent");
    }
#elif defined(SENTRY_PLATFORM_MACOS)
    // Signal via pipe
    char byte = 1;
    if (write(ipc->ready_pipe[1], &byte, 1) < 0) {
        SENTRY_WARNF("daemon: write to ready_pipe failed: %s", strerror(errno));
    } else {
        SENTRY_DEBUG("daemon: signaled ready to parent");
    }
#endif
}

bool
sentry__crash_ipc_wait_for_ready(sentry_crash_ipc_t *ipc, int timeout_ms)
{
    if (!ipc) {
        return false;
    }

#if defined(SENTRY_PLATFORM_WINDOWS)
    if (!ipc->ready_event_handle) {
        SENTRY_WARN("No ready event handle");
        return false;
    }

    DWORD timeout = (timeout_ms >= 0) ? (DWORD)timeout_ms : INFINITE;
    DWORD result = WaitForSingleObject(ipc->ready_event_handle, timeout);

    if (result == WAIT_OBJECT_0) {
        return true;
    } else if (result == WAIT_TIMEOUT) {
        return false;
    } else {
        SENTRY_WARNF(
            "crash_ipc_wait_for_ready: unexpected result %lu, error %lu",
            result, GetLastError());
        return false;
    }
#elif defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    // Wait on ready_eventfd with poll/select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ipc->ready_eventfd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(ipc->ready_eventfd + 1, &readfds, NULL, NULL,
        timeout_ms >= 0 ? &timeout : NULL);

    if (result > 0) {
        // Read the eventfd value
        uint64_t val;
        if (read(ipc->ready_eventfd, &val, sizeof(val)) < 0) {
            SENTRY_WARNF("read from ready_eventfd failed: %s", strerror(errno));
            return false;
        }
        return true;
    } else if (result == 0) {
        return false; // Timeout
    } else {
        SENTRY_WARNF("select on ready_eventfd failed: %s", strerror(errno));
        return false;
    }
#elif defined(SENTRY_PLATFORM_MACOS)
    // Wait on ready_pipe with select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ipc->ready_pipe[0], &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(ipc->ready_pipe[0] + 1, &readfds, NULL, NULL,
        timeout_ms >= 0 ? &timeout : NULL);

    if (result > 0) {
        // Read and discard the byte
        char byte;
        if (read(ipc->ready_pipe[0], &byte, 1) < 0) {
            SENTRY_WARNF("read from ready_pipe failed: %s", strerror(errno));
            return false;
        }
        return true;
    } else if (result == 0) {
        return false; // Timeout
    } else {
        SENTRY_WARNF("select on ready_pipe failed: %s", strerror(errno));
        return false;
    }
#else
    return false;
#endif
}
