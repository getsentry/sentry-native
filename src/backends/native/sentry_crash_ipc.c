#include "sentry_crash_ipc.h"

#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_logger.h"
#include "sentry_sync.h"
#include "sentry_value.h"

#include <stdio.h>
#include <string.h>

#if defined(SENTRY_PLATFORM_UNIX)
#    include <poll.h>
#endif

static void
write_u16_le(char *buf, uint16_t value)
{
    buf[0] = (char)(value & 0xffu);
    buf[1] = (char)((value >> 8) & 0xffu);
}

static void
write_u32_le(char *buf, uint32_t value)
{
    buf[0] = (char)(value & 0xffu);
    buf[1] = (char)((value >> 8) & 0xffu);
    buf[2] = (char)((value >> 16) & 0xffu);
    buf[3] = (char)((value >> 24) & 0xffu);
}

static void
write_u64_le(char *buf, uint64_t value)
{
    for (size_t i = 0; i < 8; i++) {
        buf[i] = (char)((value >> (i * 8)) & 0xffu);
    }
}

static uint16_t
read_u16_le(const char *buf)
{
    return (uint16_t)((uint8_t)buf[0] | ((uint16_t)(uint8_t)buf[1] << 8));
}

static uint32_t
read_u32_le(const char *buf)
{
    return (uint32_t)(uint8_t)buf[0] | ((uint32_t)(uint8_t)buf[1] << 8)
        | ((uint32_t)(uint8_t)buf[2] << 16) | ((uint32_t)(uint8_t)buf[3] << 24);
}

static uint64_t
read_u64_le(const char *buf)
{
    uint64_t value = 0;
    for (size_t i = 0; i < 8; i++) {
        value |= (uint64_t)(uint8_t)buf[i] << (i * 8);
    }
    return value;
}

bool
sentry__crash_ipc_message_type_is_known(uint16_t type)
{
    return type >= SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT
        && type <= SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN;
}

sentry_crash_ipc_message_result_t
sentry__crash_ipc_message_encode(uint16_t type, uint16_t flags,
    uint64_t sequence, const char *payload, size_t payload_len, char **out_buf,
    size_t *out_len)
{
    if (!out_buf || !out_len || (payload_len && !payload)
        || !sentry__crash_ipc_message_type_is_known(type)) {
        return !sentry__crash_ipc_message_type_is_known(type)
            ? SENTRY_CRASH_IPC_MESSAGE_UNKNOWN_TYPE
            : SENTRY_CRASH_IPC_MESSAGE_INVALID;
    }

    if (payload_len > SENTRY_CRASH_IPC_MESSAGE_MAX_LEN
                - SENTRY_CRASH_IPC_MESSAGE_MIN_LEN
        || payload_len > SIZE_MAX - SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE) {
        return SENTRY_CRASH_IPC_MESSAGE_OVERSIZED;
    }

    uint32_t message_len
        = (uint32_t)(SENTRY_CRASH_IPC_MESSAGE_MIN_LEN + payload_len);
    size_t total_len
        = SENTRY_CRASH_IPC_MESSAGE_PREFIX_SIZE + (size_t)message_len;
    char *buf = sentry_malloc(total_len);
    if (!buf) {
        return SENTRY_CRASH_IPC_MESSAGE_OOM;
    }

    write_u32_le(buf, message_len);
    write_u16_le(buf + 4, type);
    write_u16_le(buf + 6, flags);
    write_u64_le(buf + 8, sequence);
    if (payload_len) {
        memcpy(
            buf + SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE, payload, payload_len);
    }

    *out_buf = buf;
    *out_len = total_len;
    return SENTRY_CRASH_IPC_MESSAGE_OK;
}

sentry_crash_ipc_message_result_t
sentry__crash_ipc_message_decode(
    const char *buf, size_t buf_len, sentry_crash_ipc_message_t *message)
{
    if (!buf || !message) {
        return SENTRY_CRASH_IPC_MESSAGE_INVALID;
    }
    if (buf_len < SENTRY_CRASH_IPC_MESSAGE_PREFIX_SIZE) {
        return SENTRY_CRASH_IPC_MESSAGE_PARTIAL;
    }

    uint32_t message_len = read_u32_le(buf);
    if (message_len > SENTRY_CRASH_IPC_MESSAGE_MAX_LEN) {
        return SENTRY_CRASH_IPC_MESSAGE_OVERSIZED;
    }
    if (message_len < SENTRY_CRASH_IPC_MESSAGE_MIN_LEN) {
        return SENTRY_CRASH_IPC_MESSAGE_INVALID;
    }
#if SIZE_MAX <= UINT32_MAX
    if (message_len > SIZE_MAX - SENTRY_CRASH_IPC_MESSAGE_PREFIX_SIZE) {
        return SENTRY_CRASH_IPC_MESSAGE_OVERSIZED;
    }
#endif

    size_t total_len
        = SENTRY_CRASH_IPC_MESSAGE_PREFIX_SIZE + (size_t)message_len;
    if (buf_len < total_len) {
        return SENTRY_CRASH_IPC_MESSAGE_PARTIAL;
    }
    if (buf_len > total_len) {
        return SENTRY_CRASH_IPC_MESSAGE_INVALID;
    }

    uint16_t type = read_u16_le(buf + 4);
    if (!sentry__crash_ipc_message_type_is_known(type)) {
        return SENTRY_CRASH_IPC_MESSAGE_UNKNOWN_TYPE;
    }

    message->type = type;
    message->flags = read_u16_le(buf + 6);
    message->sequence = read_u64_le(buf + 8);
    message->payload = buf + SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE;
    message->payload_len = message_len - SENTRY_CRASH_IPC_MESSAGE_MIN_LEN;
    return SENTRY_CRASH_IPC_MESSAGE_OK;
}

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)

#    include <errno.h>
#    include <fcntl.h>
#    include <pthread.h>
#    include <sys/file.h>
#    include <sys/socket.h>
#    include <sys/stat.h>
#    include <unistd.h>

sentry_crash_ipc_t *
sentry__crash_ipc_init_app(sem_t *init_sem)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    ipc->is_daemon = false;
    ipc->init_sem = init_sem; // Use provided semaphore (managed by backend)
    ipc->message_fd[0] = -1;
    ipc->message_fd[1] = -1;

    // Create shared memory with unique name based on PID and thread ID
    // macOS has a 31-character limit for POSIX shared memory names (PSEMNAMLEN)
    // Format: /s-{8_hex_chars} = 11 chars total (well under 31 limit)
    // We mix PID and TID to create a unique 32-bit identifier, allowing
    // multiple sentry_init() calls from different threads in the same process.
    uint64_t tid = (uint64_t)pthread_self();
    uint32_t id = (uint32_t)((getpid() ^ (tid & 0xFFFFFFFF)) & 0xFFFFFFFF);
    snprintf(ipc->shm_name, sizeof(ipc->shm_name), "/s-%08x", id);

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

    // Verify and resize shared memory (both new and existing)
    if (shm_exists) {
        // Check if existing shared memory has correct size
        struct stat st;
        if (fstat(ipc->shm_fd, &st) < 0) {
            SENTRY_WARNF("failed to stat shared memory: %s", strerror(errno));
            close(ipc->shm_fd);
            if (ipc->init_sem) {
                sem_post(ipc->init_sem);
            }
            sentry_free(ipc);
            return NULL;
        }
        if (st.st_size != SENTRY_CRASH_SHM_SIZE) {
            // Existing shm has wrong size, resize it
            if (ftruncate(ipc->shm_fd, SENTRY_CRASH_SHM_SIZE) < 0) {
                SENTRY_WARNF("failed to resize existing shared memory: %s",
                    strerror(errno));
                close(ipc->shm_fd);
                if (ipc->init_sem) {
                    sem_post(ipc->init_sem);
                }
                sentry_free(ipc);
                return NULL;
            }
        }
    } else {
        // New shared memory, set size
        if (ftruncate(ipc->shm_fd, SENTRY_CRASH_SHM_SIZE) < 0) {
            SENTRY_WARNF("failed to resize shared memory: %s", strerror(errno));
            close(ipc->shm_fd);
            shm_unlink(ipc->shm_name);
            if (ipc->init_sem) {
                sem_post(ipc->init_sem);
            }
            sentry_free(ipc);
            return NULL;
        }
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
    ipc->notify_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (ipc->notify_fd < 0) {
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
    ipc->ready_fd = eventfd(0, EFD_CLOEXEC);
    if (ipc->ready_fd < 0) {
        SENTRY_WARNF("failed to create ready eventfd: %s", strerror(errno));
        close(ipc->notify_fd);
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

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, ipc->message_fd)
        < 0) {
        SENTRY_WARNF(
            "failed to create message socketpair: %s", strerror(errno));
        close(ipc->ready_fd);
        close(ipc->notify_fd);
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

    struct timeval send_timeout = { 1, 0 };
    setsockopt(ipc->message_fd[0], SOL_SOCKET, SO_SNDTIMEO, &send_timeout,
        sizeof(send_timeout));

    // Initialize shared memory only if newly created
    if (!shm_exists) {
        sentry__crash_context_init(ipc->shmem);
    }

    // Release semaphore after initialization
    if (ipc->init_sem) {
        sem_post(ipc->init_sem);
    }

    SENTRY_DEBUGF("initialized crash IPC (shm=%s, notify_fd=%d)", ipc->shm_name,
        ipc->notify_fd);

    return ipc;
}

sentry_crash_ipc_t *
sentry__crash_ipc_init_daemon(pid_t app_pid, uint64_t app_tid,
    int notify_eventfd, int ready_eventfd, int message_fd)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    ipc->is_daemon = true;
    ipc->message_fd[0] = -1;
    ipc->message_fd[1] = message_fd;
    fcntl(message_fd, F_SETFD, FD_CLOEXEC);

    // Open existing shared memory created by app (using PID and thread ID)
    // Must match the format in sentry__crash_ipc_init_app
    uint32_t id = (uint32_t)((app_pid ^ (app_tid & 0xFFFFFFFF)) & 0xFFFFFFFF);
    snprintf(ipc->shm_name, sizeof(ipc->shm_name), "/s-%08x", id);

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
    ipc->notify_fd = notify_eventfd;
    ipc->ready_fd = ready_eventfd;

    SENTRY_DEBUGF("daemon: attached to crash IPC (shm=%s, notify_fd=%d, "
                  "ready_notify_fd=%d)",
        ipc->shm_name, notify_eventfd, ready_eventfd);

    return ipc;
}

static void
notify_fallback(sentry_crash_ipc_t *ipc)
{
    if (!ipc || ipc->notify_fd < 0) {
        return;
    }

    // Write to eventfd to wake up daemon
    // This is signal-safe
    uint64_t val = 1;
    ssize_t written = write(ipc->notify_fd, &val, sizeof(val));
    (void)written; // Ignore errors in signal handler
}

bool
sentry__crash_ipc_wait(sentry_crash_ipc_t *ipc, int timeout_ms)
{
    if (!ipc || ipc->notify_fd < 0) {
        return false;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ipc->notify_fd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(ipc->notify_fd + 1, &readfds, NULL, NULL,
        timeout_ms >= 0 ? &timeout : NULL);

    if (ret > 0 && FD_ISSET(ipc->notify_fd, &readfds)) {
        uint64_t val;
        ssize_t result = read(ipc->notify_fd, &val, sizeof(val));
        if (result < 0) {
            SENTRY_WARN("Failed to read from notify_fd");
        }
        return true;
    }

    return false;
}

void
sentry__crash_ipc_unlink(sentry_crash_ipc_t *ipc)
{
    if (ipc && ipc->shm_name[0]) {
        shm_unlink(ipc->shm_name);
    }
}

void
sentry__crash_ipc_free(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }

    sentry_free(ipc->message_buf);
    sentry__crash_scope_free(&ipc->scope);

    if (ipc->shmem && ipc->shmem != MAP_FAILED) {
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
    }

    if (ipc->shm_fd >= 0) {
        close(ipc->shm_fd);
    }

    if (!ipc->is_daemon) {
        sentry__crash_ipc_unlink(ipc);
    }

    if (ipc->notify_fd >= 0) {
        close(ipc->notify_fd);
    }

    if (ipc->ready_fd >= 0) {
        close(ipc->ready_fd);
    }

    if (ipc->message_fd[0] >= 0) {
        close(ipc->message_fd[0]);
    }
    if (ipc->message_fd[1] >= 0) {
        close(ipc->message_fd[1]);
    }

    sentry_free(ipc);
}

#elif defined(SENTRY_PLATFORM_MACOS)

#    include <errno.h>
#    include <fcntl.h>
#    include <pthread.h>
#    include <stdlib.h>
#    include <sys/file.h>
#    include <sys/socket.h>
#    include <sys/stat.h>
#    include <unistd.h>

sentry_crash_ipc_t *
sentry__crash_ipc_init_app(sentry_mutex_t *init_mutex)
{
    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    ipc->is_daemon = false;
    ipc->init_mutex = init_mutex;
    ipc->message_fd[0] = -1;
    ipc->message_fd[1] = -1;

    // Build a file path for shared memory using the system temp directory.
    // Unlike shm_open(), regular files in $TMPDIR work inside App Sandbox.
    // $TMPDIR is per-app in sandbox (e.g. .../Containers/.../T/).
    uint64_t tid = (uint64_t)pthread_self();
    uint32_t id = (uint32_t)((getpid() ^ (tid & 0xFFFFFFFF)) & 0xFFFFFFFF);
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir) {
        tmpdir = "/tmp";
    }
    snprintf(ipc->shm_path, sizeof(ipc->shm_path), "%s/.sentry-shm-%08x",
        tmpdir, id);

    // Acquire mutex for exclusive access during initialization
    if (ipc->init_mutex) {
        sentry__mutex_lock(ipc->init_mutex);
    }

    // Use file-backed shared memory (sandbox-safe, no shm_open)
    bool shm_exists = false;
    ipc->shm_fd = open(ipc->shm_path, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (ipc->shm_fd < 0 && errno == EEXIST) {
        shm_exists = true;
        ipc->shm_fd = open(ipc->shm_path, O_RDWR, 0600);
    }

    if (ipc->shm_fd < 0) {
        SENTRY_WARNF("failed to open shared memory file: %s", strerror(errno));
        if (ipc->init_mutex) {
            sentry__mutex_unlock(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Verify and resize shared memory (both new and existing)
    if (shm_exists) {
        struct stat st;
        if (fstat(ipc->shm_fd, &st) < 0) {
            SENTRY_WARNF(
                "failed to stat shared memory file: %s", strerror(errno));
            close(ipc->shm_fd);
            if (ipc->init_mutex) {
                sentry__mutex_unlock(ipc->init_mutex);
            }
            sentry_free(ipc);
            return NULL;
        }
        if (st.st_size != SENTRY_CRASH_SHM_SIZE) {
            if (ftruncate(ipc->shm_fd, SENTRY_CRASH_SHM_SIZE) < 0) {
                SENTRY_WARNF(
                    "failed to resize shared memory file: %s", strerror(errno));
                close(ipc->shm_fd);
                if (ipc->init_mutex) {
                    sentry__mutex_unlock(ipc->init_mutex);
                }
                sentry_free(ipc);
                return NULL;
            }
        }
    } else {
        if (ftruncate(ipc->shm_fd, SENTRY_CRASH_SHM_SIZE) < 0) {
            SENTRY_WARNF(
                "failed to resize shared memory file: %s", strerror(errno));
            close(ipc->shm_fd);
            unlink(ipc->shm_path);
            if (ipc->init_mutex) {
                sentry__mutex_unlock(ipc->init_mutex);
            }
            sentry_free(ipc);
            return NULL;
        }
    }

    ipc->shmem = mmap(NULL, SENTRY_CRASH_SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, ipc->shm_fd, 0);
    if (ipc->shmem == MAP_FAILED) {
        SENTRY_WARNF("failed to map shared memory: %s", strerror(errno));
        close(ipc->shm_fd);
        if (!shm_exists) {
            unlink(ipc->shm_path);
        }
        if (ipc->init_mutex) {
            sentry__mutex_unlock(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Create pipe for crash notifications (works across fork/posix_spawn)
    if (pipe(ipc->notify_pipe) < 0) {
        SENTRY_WARNF("failed to create notification pipe: %s", strerror(errno));
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        if (!shm_exists) {
            unlink(ipc->shm_path);
        }
        if (ipc->init_mutex) {
            sentry__mutex_unlock(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    // Make write end non-blocking for signal-safe writes
    fcntl(ipc->notify_pipe[1], F_SETFL, O_NONBLOCK);

    // Create pipe for daemon ready signal
    if (pipe(ipc->ready_pipe) < 0) {
        SENTRY_WARNF("failed to create ready pipe: %s", strerror(errno));
        close(ipc->notify_pipe[0]);
        close(ipc->notify_pipe[1]);
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        if (!shm_exists) {
            unlink(ipc->shm_path);
        }
        if (ipc->init_mutex) {
            sentry__mutex_unlock(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipc->message_fd) < 0) {
        SENTRY_WARNF(
            "failed to create message socketpair: %s", strerror(errno));
        close(ipc->ready_pipe[0]);
        close(ipc->ready_pipe[1]);
        close(ipc->notify_pipe[0]);
        close(ipc->notify_pipe[1]);
        munmap(ipc->shmem, SENTRY_CRASH_SHM_SIZE);
        close(ipc->shm_fd);
        if (!shm_exists) {
            unlink(ipc->shm_path);
        }
        if (ipc->init_mutex) {
            sentry__mutex_unlock(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }
    int no_sigpipe = 1;
    setsockopt(ipc->message_fd[0], SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
        sizeof(no_sigpipe));
    struct timeval send_timeout = { 1, 0 };
    setsockopt(ipc->message_fd[0], SOL_SOCKET, SO_SNDTIMEO, &send_timeout,
        sizeof(send_timeout));
    fcntl(ipc->message_fd[0], F_SETFD, FD_CLOEXEC);
    fcntl(ipc->message_fd[1], F_SETFD, FD_CLOEXEC);

    if (!shm_exists) {
        sentry__crash_context_init(ipc->shmem);
    }

    if (ipc->init_mutex) {
        sentry__mutex_unlock(ipc->init_mutex);
    }

    SENTRY_DEBUGF("initialized crash IPC (shm=%s, pipe=%d/%d)", ipc->shm_path,
        ipc->notify_pipe[0], ipc->notify_pipe[1]);

    return ipc;
}

sentry_crash_ipc_t *
sentry__crash_ipc_init_daemon(pid_t app_pid, uint64_t app_tid,
    int notify_pipe_read, int ready_pipe_write, int shm_fd, int message_fd)
{
    (void)app_pid;
    (void)app_tid;

    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    ipc->is_daemon = true;
    ipc->message_fd[0] = -1;
    ipc->message_fd[1] = message_fd;
    fcntl(message_fd, F_SETFD, FD_CLOEXEC);

    // Use the inherited shm_fd directly (no shm_open needed, sandbox-safe)
    ipc->shm_fd = shm_fd;
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

    // Pipes and shm_fd are inherited from parent via posix_spawn
    ipc->notify_pipe[0] = notify_pipe_read;
    ipc->notify_pipe[1] = -1;
    ipc->ready_pipe[0] = -1;
    ipc->ready_pipe[1] = ready_pipe_write;

    SENTRY_DEBUGF("daemon: attached to crash IPC (shm_fd=%d, notify_pipe=%d, "
                  "ready_pipe=%d)",
        shm_fd, notify_pipe_read, ready_pipe_write);

    return ipc;
}

static void
notify_fallback(sentry_crash_ipc_t *ipc)
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
sentry__crash_ipc_unlink(sentry_crash_ipc_t *ipc)
{
    if (ipc && ipc->shm_path[0]) {
        unlink(ipc->shm_path);
    }
}

void
sentry__crash_ipc_free(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }

    sentry_free(ipc->message_buf);
    sentry__crash_scope_free(&ipc->scope);

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

    if (ipc->message_fd[0] >= 0) {
        close(ipc->message_fd[0]);
    }
    if (ipc->message_fd[1] >= 0) {
        close(ipc->message_fd[1]);
    }

    if (!ipc->is_daemon) {
        sentry__crash_ipc_unlink(ipc);
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
    ipc->is_daemon = false;
    ipc->init_mutex = init_mutex; // Use provided mutex (managed by backend)

    // Create named shared memory with unique name based on PID and thread ID
    uint64_t tid = (uint64_t)GetCurrentThreadId();
    swprintf(ipc->shm_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrash-%lu-%llx", GetCurrentProcessId(), tid);

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

    // Create named event for notifications (using PID and thread ID)
    swprintf(ipc->event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashEvent-%lu-%llx", GetCurrentProcessId(), tid);

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

    // Create ready event for daemon to signal when it's initialized (using PID
    // and thread ID)
    swprintf(ipc->ready_event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashReady-%lu-%llx", GetCurrentProcessId(), tid);
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

    SECURITY_ATTRIBUTES pipe_attrs;
    ZeroMemory(&pipe_attrs, sizeof(pipe_attrs));
    pipe_attrs.nLength = sizeof(pipe_attrs);
    pipe_attrs.bInheritHandle = TRUE;
    if (!CreatePipe(&ipc->message_read_handle, &ipc->message_write_handle,
            &pipe_attrs, 0)) {
        SENTRY_WARNF("failed to create message pipe: %lu", GetLastError());
        CloseHandle(ipc->ready_event_handle);
        CloseHandle(ipc->event_handle);
        UnmapViewOfFile(ipc->shmem);
        CloseHandle(ipc->shm_handle);
        if (ipc->init_mutex) {
            ReleaseMutex(ipc->init_mutex);
        }
        sentry_free(ipc);
        return NULL;
    }
    SetHandleInformation(ipc->message_write_handle, HANDLE_FLAG_INHERIT, 0);

    // Initialize shared memory only if newly created
    if (!shm_exists) {
        sentry__crash_context_init(ipc->shmem);
    }

    // Release mutex after initialization
    if (ipc->init_mutex) {
        ReleaseMutex(ipc->init_mutex);
    }

    SENTRY_DEBUG("initialized crash IPC");

    return ipc;
}

sentry_crash_ipc_t *
sentry__crash_ipc_init_daemon(pid_t app_pid, uint64_t app_tid,
    HANDLE event_handle, HANDLE ready_event_handle, HANDLE message_read_handle)
{
    // On Windows, we open events by name, so handles from parent are not used
    // (handles are per-process and cannot be directly inherited)
    (void)event_handle;
    (void)ready_event_handle;

    sentry_crash_ipc_t *ipc = SENTRY_MAKE(sentry_crash_ipc_t);
    if (!ipc) {
        return NULL;
    }
    ipc->is_daemon = true;
    ipc->message_read_handle = message_read_handle;
    SetHandleInformation(message_read_handle, HANDLE_FLAG_INHERIT, 0);

    // Open existing shared memory (using PID and thread ID)
    swprintf(ipc->shm_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrash-%lu-%llx", (unsigned long)app_pid, app_tid);

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

    // Open existing event (using PID and thread ID)
    swprintf(ipc->event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashEvent-%lu-%llx", (unsigned long)app_pid, app_tid);

    ipc->event_handle = OpenEventW(SYNCHRONIZE, FALSE, ipc->event_name);
    if (!ipc->event_handle) {
        SENTRY_WARNF("daemon: failed to open event: %lu", GetLastError());
        UnmapViewOfFile(ipc->shmem);
        CloseHandle(ipc->shm_handle);
        sentry_free(ipc);
        return NULL;
    }

    // Open ready event to signal when daemon is initialized (using PID and
    // thread ID)
    swprintf(ipc->ready_event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashReady-%lu-%llx", (unsigned long)app_pid, app_tid);
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

static void
notify_fallback(sentry_crash_ipc_t *ipc)
{
    if (!ipc || !ipc->event_handle) {
        // No logging - called from signal handler/exception filter
        return;
    }

    // SetEvent is safe to call from exception filter
    // Ignore errors silently - we're crashing anyway
    SetEvent(ipc->event_handle);
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
sentry__crash_ipc_unlink(sentry_crash_ipc_t *ipc)
{
    (void)ipc;
}

void
sentry__crash_ipc_free(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }

    sentry_free(ipc->message_buf);
    sentry__crash_scope_free(&ipc->scope);

    if (ipc->shmem) {
        UnmapViewOfFile(ipc->shmem);
    }

    if (ipc->shm_handle) {
        CloseHandle(ipc->shm_handle);
    }

    if (ipc->event_handle) {
        CloseHandle(ipc->event_handle);
    }

    if (ipc->ready_event_handle) {
        CloseHandle(ipc->ready_event_handle);
    }

    if (ipc->message_read_handle) {
        CloseHandle(ipc->message_read_handle);
    }
    if (ipc->message_write_handle) {
        CloseHandle(ipc->message_write_handle);
    }

    if (ipc->parent_handle) {
        CloseHandle(ipc->parent_handle);
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
    if (write(ipc->ready_fd, &val, sizeof(val)) < 0) {
        SENTRY_WARNF(
            "daemon: write to ready_eventfd failed: %s", strerror(errno));
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
    FD_SET(ipc->ready_fd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(ipc->ready_fd + 1, &readfds, NULL, NULL,
        timeout_ms >= 0 ? &timeout : NULL);

    if (result > 0) {
        // Read the eventfd value
        uint64_t val;
        if (read(ipc->ready_fd, &val, sizeof(val)) < 0) {
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

bool
sentry__crash_scope_init(sentry_crash_scope_t *scope, size_t max_breadcrumbs)
{
    memset(scope, 0, sizeof(*scope));
    scope->event = sentry_value_new_null();
    scope->attachments = sentry_value_new_list();
    scope->breadcrumbs = sentry__ringbuffer_new(max_breadcrumbs);
    return scope->breadcrumbs && !sentry_value_is_null(scope->attachments);
}

void
sentry__crash_scope_free(sentry_crash_scope_t *scope)
{
    sentry_value_decref(scope->event);
    sentry_value_decref(scope->attachments);
    sentry__ringbuffer_free(scope->breadcrumbs);
}

bool
sentry__crash_scope_apply(
    sentry_crash_scope_t *scope, const sentry_crash_ipc_message_t *message)
{
    if (scope->stopped || message->flags
        || message->sequence != scope->sequence + 1
        || (!scope->initialized
            && message->type != SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT)) {
        return false;
    }
    if (message->type == SENTRY_CRASH_IPC_MESSAGE_CRASH
        || message->type == SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN) {
        if (message->payload_len) {
            return false;
        }
        scope->sequence = message->sequence;
        scope->stopped = true;
        return true;
    }

    sentry_value_t value
        = sentry__value_from_msgpack(message->payload, message->payload_len);
    sentry_value_type_t type = sentry_value_get_type(value);
    if (sentry_value_is_null(value)
        && (message->payload_len != 1
            || (unsigned char)message->payload[0] != 0xc0)) {
        return false;
    }
    bool valid = false;
    const char *field = NULL;
    switch (message->type) {
    case SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT: {
        sentry_value_t event = sentry_value_get_by_key(value, "event");
        sentry_value_t attachments
            = sentry_value_get_by_key(value, "attachments");
        if (sentry_value_get_type(event) != SENTRY_VALUE_TYPE_OBJECT
            || sentry_value_get_type(attachments) != SENTRY_VALUE_TYPE_LIST) {
            break;
        }
        sentry_ringbuffer_t *breadcrumbs
            = sentry__ringbuffer_new(scope->breadcrumbs->max_size);
        if (!breadcrumbs) {
            break;
        }
        sentry_value_t list = sentry_value_get_by_key(event, "breadcrumbs");
        for (size_t i = 0; i < sentry_value_get_length(list); i++) {
            sentry__ringbuffer_append(breadcrumbs,
                sentry_value_incref(sentry_value_get_by_index(list, i)));
        }
        sentry_value_remove_by_key(event, "breadcrumbs");
        sentry__value_replace(&scope->event, sentry_value_incref(event));
        sentry__value_replace(
            &scope->attachments, sentry_value_incref(attachments));
        sentry__ringbuffer_free(scope->breadcrumbs);
        scope->breadcrumbs = breadcrumbs;
        scope->initialized = true;
        valid = true;
        break;
    }
    case SENTRY_CRASH_IPC_MESSAGE_SET_RELEASE:
        field = "release";
        break;
    case SENTRY_CRASH_IPC_MESSAGE_SET_ENVIRONMENT:
        field = "environment";
        break;
    case SENTRY_CRASH_IPC_MESSAGE_SET_TRANSACTION:
        field = "transaction";
        break;
    case SENTRY_CRASH_IPC_MESSAGE_SET_FINGERPRINT:
        field = "fingerprint";
        break;
    case SENTRY_CRASH_IPC_MESSAGE_SET_LEVEL:
        field = "level";
        break;
    case SENTRY_CRASH_IPC_MESSAGE_SET_USER:
        field = "user";
        break;
    case SENTRY_CRASH_IPC_MESSAGE_SET_TAG:
    case SENTRY_CRASH_IPC_MESSAGE_SET_EXTRA:
    case SENTRY_CRASH_IPC_MESSAGE_SET_CONTEXT: {
        const char *key
            = sentry_value_as_string(sentry_value_get_by_index(value, 0));
        if (type != SENTRY_VALUE_TYPE_LIST
            || sentry_value_get_length(value) != 2
            || sentry_value_get_type(sentry_value_get_by_index(value, 0))
                != SENTRY_VALUE_TYPE_STRING) {
            break;
        }
        const char *name = message->type == SENTRY_CRASH_IPC_MESSAGE_SET_TAG
            ? "tags"
            : message->type == SENTRY_CRASH_IPC_MESSAGE_SET_EXTRA ? "extra"
                                                                  : "contexts";
        sentry_value_t object = sentry_value_get_by_key(scope->event, name);
        if (sentry_value_is_null(object)) {
            object = sentry_value_new_object();
            sentry_value_set_by_key(scope->event, name, object);
        }
        valid
            = sentry_value_set_by_key_n(object, key,
                  sentry_value_get_length(sentry_value_get_by_index(value, 0)),
                  sentry_value_incref(sentry_value_get_by_index(value, 1)))
            == 0;
        break;
    }
    case SENTRY_CRASH_IPC_MESSAGE_REMOVE_TAG:
    case SENTRY_CRASH_IPC_MESSAGE_REMOVE_EXTRA:
    case SENTRY_CRASH_IPC_MESSAGE_REMOVE_CONTEXT: {
        if (type != SENTRY_VALUE_TYPE_STRING) {
            break;
        }
        const char *name = message->type == SENTRY_CRASH_IPC_MESSAGE_REMOVE_TAG
            ? "tags"
            : message->type == SENTRY_CRASH_IPC_MESSAGE_REMOVE_EXTRA
            ? "extra"
            : "contexts";
        sentry_value_remove_by_key_n(
            sentry_value_get_by_key(scope->event, name),
            sentry_value_as_string(value), sentry_value_get_length(value));
        valid = true;
        break;
    }
    case SENTRY_CRASH_IPC_MESSAGE_ADD_BREADCRUMB:
        if (type == SENTRY_VALUE_TYPE_OBJECT) {
            valid = !scope->breadcrumbs->max_size
                || sentry__ringbuffer_append(
                       scope->breadcrumbs, sentry_value_incref(value))
                    == 0;
        }
        break;
    case SENTRY_CRASH_IPC_MESSAGE_SET_ATTACHMENT_LIST:
        if (type == SENTRY_VALUE_TYPE_LIST) {
            sentry__value_replace(
                &scope->attachments, sentry_value_incref(value));
            valid = true;
        }
        break;
    default:
        break;
    }
    if (field) {
        sentry_value_type_t expected
            = message->type == SENTRY_CRASH_IPC_MESSAGE_SET_USER
            ? SENTRY_VALUE_TYPE_OBJECT
            : message->type == SENTRY_CRASH_IPC_MESSAGE_SET_FINGERPRINT
            ? SENTRY_VALUE_TYPE_LIST
            : SENTRY_VALUE_TYPE_STRING;
        if (type == expected || sentry_value_is_null(value)) {
            if (sentry_value_is_null(value)
                || (type == SENTRY_VALUE_TYPE_STRING
                    && !sentry_value_get_length(value))) {
                sentry_value_remove_by_key(scope->event, field);
                valid = true;
            } else {
                valid = sentry_value_set_by_key(
                            scope->event, field, sentry_value_incref(value))
                    == 0;
            }
        }
    }
    sentry_value_decref(value);
    if (valid) {
        scope->sequence = message->sequence;
    }
    return valid;
}

sentry_value_t
sentry__crash_scope_event(const sentry_crash_scope_t *scope)
{
    sentry_value_t event = scope->initialized
        ? sentry__value_clone(scope->event)
        : sentry_value_new_event();
    sentry_value_set_by_key(
        event, "breadcrumbs", sentry__ringbuffer_to_list(scope->breadcrumbs));
    sentry_value_set_by_key(
        event, "level", sentry__value_new_level(SENTRY_LEVEL_FATAL));
    sentry__ensure_event_id(event, NULL);
    return event;
}

static bool
write_message(
    sentry_crash_ipc_t *ipc, const char *buf, size_t len, bool nonblocking)
{
    while (len) {
#if defined(SENTRY_PLATFORM_WINDOWS)
        (void)nonblocking;
        DWORD written = 0;
        if (!WriteFile(
                ipc->message_write_handle, buf, (DWORD)len, &written, NULL)
            || !written) {
            return false;
        }
#else
        int flags = nonblocking ? MSG_DONTWAIT : 0;
#    if defined(MSG_NOSIGNAL)
        flags |= MSG_NOSIGNAL;
#    endif
        ssize_t written = send(ipc->message_fd[0], buf, len, flags);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
#endif
        buf += written;
        len -= (size_t)written;
    }
    return true;
}

bool
sentry__crash_ipc_send(
    sentry_crash_ipc_t *ipc, uint16_t type, sentry_value_t value)
{
    if (!ipc || sentry__atomic_fetch(&ipc->stopped)
        || sentry__atomic_fetch(&ipc->message_failed)
        || !sentry__atomic_compare_swap(&ipc->writing, 0, 1)) {
        return false;
    }
    size_t payload_len = 0;
    char *payload = type == SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN
        ? NULL
        : sentry_value_to_msgpack(value, &payload_len);
    char *buf = NULL;
    size_t len = 0;
    bool sent = (payload || type == SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN)
        && sentry__crash_ipc_message_encode(
               type, 0, ipc->sequence + 1, payload, payload_len, &buf, &len)
            == SENTRY_CRASH_IPC_MESSAGE_OK;
    if (sent) {
        sent = write_message(ipc, buf, len, false);
        if (sent) {
            ipc->sequence++;
        } else {
            // a partial write makes this stream unusable for further frames
            sentry__atomic_store(&ipc->message_failed, 1);
        }
    }
    sentry_free(buf);
    sentry_free(payload);
    sentry__atomic_store(&ipc->writing, 0);
    return sent;
}

void
sentry__crash_ipc_notify(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return;
    }
    sentry__atomic_store(&ipc->stopped, 1);
#if !defined(SENTRY_PLATFORM_WINDOWS)
    if (!sentry__atomic_fetch(&ipc->message_failed)
        && sentry__atomic_compare_swap(&ipc->writing, 0, 1)) {
        char frame[SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE] = { 0 };
        write_u32_le(frame, SENTRY_CRASH_IPC_MESSAGE_MIN_LEN);
        write_u16_le(frame + 4, SENTRY_CRASH_IPC_MESSAGE_CRASH);
        write_u64_le(frame + 8, ipc->sequence + 1);
        bool sent = write_message(ipc, frame, sizeof(frame), true);
        sentry__atomic_store(&ipc->writing, 0);
        if (sent) {
            return;
        }
    }
#endif
    // also used by WER; the daemon drains complete frames before freezing
    notify_fallback(ipc);
}

static void
close_message_stream(sentry_crash_ipc_t *ipc)
{
    ipc->message_closed = true;
#if defined(SENTRY_PLATFORM_WINDOWS)
    CloseHandle(ipc->message_read_handle);
    ipc->message_read_handle = NULL;
#else
    close(ipc->message_fd[1]);
    ipc->message_fd[1] = -1;
#endif
}

bool
sentry__crash_ipc_receive(sentry_crash_ipc_t *ipc, int timeout_ms)
{
    // a bounded poll also checks legacy notifications without blocking a writer
    int remaining = timeout_ms;
    do {
        ipc->notify_pending
            = ipc->notify_pending || sentry__crash_ipc_wait(ipc, 0);
        bool available = false;
        if (!ipc->message_closed) {
#if defined(SENTRY_PLATFORM_WINDOWS)
            DWORD bytes = 0;
            if (!PeekNamedPipe(
                    ipc->message_read_handle, NULL, 0, NULL, &bytes, NULL)) {
                close_message_stream(ipc);
            }
            available = bytes > 0;
#else
            struct pollfd fd = { ipc->message_fd[1], POLLIN, 0 };
            available = poll(&fd, 1, 0) > 0;
#endif
        }
        if (!available) {
            if (ipc->notify_pending) {
                ipc->scope.stopped = true;
                return true;
            }
            if (ipc->message_closed || remaining <= 0) {
                return false;
            }
#if defined(SENTRY_PLATFORM_WINDOWS)
            int delay = remaining < 10 ? remaining : 10;
            ipc->notify_pending = sentry__crash_ipc_wait(ipc, delay);
            remaining -= delay;
#else
#    if defined(SENTRY_PLATFORM_MACOS)
            int notify_fd = ipc->notify_pipe[0];
#    else
            int notify_fd = ipc->notify_fd;
#    endif
            struct pollfd fds[] = {
                { ipc->message_fd[1], POLLIN, 0 },
                { notify_fd, POLLIN, 0 },
            };
            int rv = poll(fds, 2, remaining);
            if (!rv || (rv < 0 && errno != EINTR)) {
                return false;
            }
#endif
            continue;
        }
        if (!ipc->message_buf) {
            ipc->message_size = SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE;
            ipc->message_buf = sentry_malloc(ipc->message_size);
            if (!ipc->message_buf) {
                close_message_stream(ipc);
                continue;
            }
        }
        size_t len = ipc->message_size - ipc->message_len;
#if defined(SENTRY_PLATFORM_WINDOWS)
        DWORD bytes = 0, received = 0;
        if (!PeekNamedPipe(
                ipc->message_read_handle, NULL, 0, NULL, &bytes, NULL)
            || !ReadFile(ipc->message_read_handle,
                ipc->message_buf + ipc->message_len,
                (DWORD)(len < bytes ? len : bytes), &received, NULL)
            || !received) {
            close_message_stream(ipc);
            continue;
        }
#else
        ssize_t received = recv(ipc->message_fd[1],
            ipc->message_buf + ipc->message_len, len, MSG_DONTWAIT);
        if (received < 0 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        }
        if (received <= 0) {
            close_message_stream(ipc);
            continue;
        }
#endif
        ipc->message_len += (size_t)received;
        if (ipc->message_len < ipc->message_size) {
            continue;
        }
        sentry_crash_ipc_message_t message;
        sentry_crash_ipc_message_result_t result
            = sentry__crash_ipc_message_decode(
                ipc->message_buf, ipc->message_len, &message);
        if (result == SENTRY_CRASH_IPC_MESSAGE_PARTIAL) {
            if (!sentry__crash_ipc_message_type_is_known(
                    read_u16_le(ipc->message_buf + 4))
                || read_u16_le(ipc->message_buf + 6)
                || read_u64_le(ipc->message_buf + 8)
                    != ipc->scope.sequence + 1) {
                close_message_stream(ipc);
                continue;
            }
            ipc->message_size = (size_t)read_u32_le(ipc->message_buf)
                + SENTRY_CRASH_IPC_MESSAGE_PREFIX_SIZE;
            char *buf = sentry_malloc(ipc->message_size);
            if (!buf) {
                close_message_stream(ipc);
                continue;
            }
            memcpy(buf, ipc->message_buf, ipc->message_len);
            sentry_free(ipc->message_buf);
            ipc->message_buf = buf;
            continue;
        }
        if (result != SENTRY_CRASH_IPC_MESSAGE_OK
            || !sentry__crash_scope_apply(&ipc->scope, &message)) {
            SENTRY_WARN("daemon: invalid scope IPC frame");
            close_message_stream(ipc);
            continue;
        }
        uint16_t type = message.type;
        sentry_free(ipc->message_buf);
        ipc->message_buf = NULL;
        ipc->message_len = 0;
        if (type == SENTRY_CRASH_IPC_MESSAGE_CRASH) {
            return true;
        }
        if (type == SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN) {
            return false;
        }
    } while (true);
}
