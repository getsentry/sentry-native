#include "sentry_thread_stackwalk.h"

#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)

#    include "sentry_app_hang_latch.h" // SENTRY_APP_HANG_MAX_FRAMES
#    include "sentry_logger.h"

#    include <dirent.h>
#    include <errno.h>
#    include <fcntl.h>
#    include <semaphore.h>
#    include <signal.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <sys/syscall.h>
#    include <time.h>
#    include <unistd.h>

#    if defined(SENTRY_WITH_UNWINDER_LIBUNWIND)
#        define UNW_LOCAL_ONLY
#        include <libunwind.h>
#    endif

#    if defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)
#        include "sentry.h"
#    endif

#    define SENTRY_APP_HANG_SIGNAL (SIGRTMIN + 4)

// Each sample uses its own slot. A handler that runs after its sample timed
// out writes only to its own slot, which nobody reads anymore. A slot is reused
// only when no handler runs in it.
#    define SENTRY_APP_HANG_SAMPLE_SLOTS 8

typedef struct {
    sem_t done; // handler -> watchdog: the sample is complete
    // The thread whose handler may claim the sample, or 0 when the sample is
    // claimed or abandoned.
    volatile pid_t waiting_tid;
    volatile sig_atomic_t running; // handlers that run in this slot
    size_t want;
    size_t count;
    void *ips[SENTRY_APP_HANG_MAX_FRAMES];
#    if defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)
    sem_t uctx_ready; // handler -> watchdog: parked, uctx valid
    sem_t unwind_done; // watchdog -> handler: unwind complete, you may return
    volatile sig_atomic_t abort_park;
    ucontext_t *volatile uctx;
#    endif
} sample_slot_t;

static sample_slot_t g_slots[SENTRY_APP_HANG_SAMPLE_SLOTS];
static size_t g_next_slot = 0;

// Claims the slot that waits for the calling thread. One compare-and-swap
// checks the thread and claims the sample, so a late or repeated signal
// cannot claim a sample that belongs to another thread or was already taken.
static sample_slot_t *
claim_slot(void)
{
    const pid_t tid = (pid_t)syscall(SYS_gettid);
    for (size_t i = 0; i < SENTRY_APP_HANG_SAMPLE_SLOTS; i++) {
        sample_slot_t *slot = &g_slots[i];
        __atomic_fetch_add(&slot->running, 1, __ATOMIC_ACQ_REL);
        pid_t expected = tid;
        if (__atomic_compare_exchange_n(&slot->waiting_tid, &expected, 0, false,
                __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
            return slot;
        }
        __atomic_fetch_sub(&slot->running, 1, __ATOMIC_RELEASE);
    }
    return NULL;
}

static void
handler(int sig, siginfo_t *info, void *ucontext)
{
    (void)sig;
    (void)info;
    // The handler interrupts arbitrary code on the target thread; preserve its
    // errno so the calls below (sem_*, unw_*) don't leak a value back to it.
    const int saved_errno = errno;
    sample_slot_t *slot = claim_slot();
    if (!slot) {
        errno = saved_errno;
        return; // stray/late delivery; ignore
    }
    size_t n = 0;
#    if defined(SENTRY_WITH_UNWINDER_LIBUNWIND)
    // This duplicates the unwind loop in sentry__unwind_stack_libunwind rather
    // than calling it, because we are inside a signal handler on the target
    // thread: the shared unwinder is not async-signal-safe (it calls
    // SENTRY_WARN and open("/proc/self/maps") for SP validation). libunwind has
    // no API to local-unwind another thread off-thread, so we must walk in the
    // handler with this trimmed, signal-safe loop. The libunwindstack path
    // below sidesteps this by parking the handler and unwinding on the watchdog
    // thread, which is why it can reuse sentry_unwind_stack_from_ucontext.
    unw_cursor_t cursor;
    if (unw_init_local2(
            &cursor, (unw_context_t *)ucontext, UNW_INIT_SIGNAL_FRAME)
        == 0) {
        while (n < slot->want) {
            unw_word_t ip = 0;
            if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0 || ip == 0) {
                break;
            }
            slot->ips[n++] = (void *)(uintptr_t)ip;
            if (unw_step(&cursor) <= 0) {
                break;
            }
        }
    }
    slot->count = n;
#    elif defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)
    (void)n;
    slot->uctx = (ucontext_t *)ucontext;
    sem_post(&slot->uctx_ready);
    for (;;) {
        if (sem_wait(&slot->unwind_done) == 0) {
            break;
        }
        if (errno != EINTR || slot->abort_park) {
            break;
        }
    }
    slot->uctx = NULL;
#    else
    (void)ucontext;
    slot->count = n;
#    endif
    sem_post(&slot->done);
    __atomic_fetch_sub(&slot->running, 1, __ATOMIC_RELEASE);
    errno = saved_errno;
}

static bool g_installed = false;
static size_t g_slots_initialized = 0;

static bool
init_slot(sample_slot_t *slot)
{
    if (sem_init(&slot->done, 0, 0) != 0) {
        return false;
    }
#    if defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)
    if (sem_init(&slot->uctx_ready, 0, 0) != 0) {
        sem_destroy(&slot->done);
        return false;
    }
    if (sem_init(&slot->unwind_done, 0, 0) != 0) {
        sem_destroy(&slot->done);
        sem_destroy(&slot->uctx_ready);
        return false;
    }
#    endif
    return true;
}

static bool
ensure_installed(void)
{
    if (g_installed) {
        return true;
    }
    // Semaphores are process-lifetime; intentionally never sem_destroy'd.
    for (; g_slots_initialized < SENTRY_APP_HANG_SAMPLE_SLOTS;
        g_slots_initialized++) {
        if (!init_slot(&g_slots[g_slots_initialized])) {
            SENTRY_DEBUG("app-hang: sem_init failed");
            return false;
        }
    }
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SENTRY_APP_HANG_SIGNAL, &sa, NULL) != 0) {
        SENTRY_DEBUG("app-hang: sigaction failed");
        return false;
    }
#    if defined(SENTRY_WITH_UNWINDER_LIBUNWIND)
    // Prime the unwinder cache so the in-handler unwind never triggers
    // dl_iterate_phdr for the first time inside the signal handler.
    unw_context_t uc;
    unw_cursor_t cur;
#        ifdef __clang__
// This pragma is required to build with Werror on ARM64 Ubuntu
#            pragma clang diagnostic push
#            pragma clang diagnostic ignored                                   \
                "-Wgnu-statement-expression-from-macro-expansion"
#        endif
    int got_context = unw_getcontext(&uc);
#        ifdef __clang__
#            pragma clang diagnostic pop
#        endif
    if (got_context == 0 && unw_init_local(&cur, &uc) == 0) {
        for (int i = 0; i < 5 && unw_step(&cur) > 0; i++) { }
    }
#    endif
    g_installed = true;
    return true;
}

static struct timespec
deadline_after(uint64_t timeout_ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += (time_t)(timeout_ms / 1000);
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return ts;
}

// A signal from an earlier sample is still pending on a thread that blocks the
// signal or does not run. Waiting for it again would only use up the timeout
// and queue one more signal. `SigBlk` cannot be used instead: tools such as
// valgrind block all signals of every thread in the kernel.
static bool
is_signal_pending(uint64_t tid)
{
    // Reads with open/read and a stack buffer: this runs before every sample,
    // and stdio would allocate.
    char status_path[64];
    snprintf(status_path, sizeof(status_path), "/proc/self/task/%llu/status",
        (unsigned long long)tid);
    int fd = open(status_path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    char status[2048];
    ssize_t len = read(fd, status, sizeof(status) - 1);
    close(fd);
    if (len <= 0) {
        return false;
    }
    status[len] = '\0';
    const char *sig_pnd = strstr(status, "SigPnd:");
    if (!sig_pnd) {
        return false;
    }
    unsigned long long pending
        = strtoull(sig_pnd + sizeof("SigPnd:") - 1, NULL, 16);
    return pending & (1ULL << (SENTRY_APP_HANG_SIGNAL - 1));
}

// Returns a slot that no handler runs in, after clearing what a timed-out
// sample left in it. Only the watchdog thread calls this.
static sample_slot_t *
acquire_slot(void)
{
    for (size_t i = 0; i < SENTRY_APP_HANG_SAMPLE_SLOTS; i++) {
        size_t index = (g_next_slot + i) % SENTRY_APP_HANG_SAMPLE_SLOTS;
        sample_slot_t *slot = &g_slots[index];
        if (__atomic_load_n(&slot->running, __ATOMIC_ACQUIRE) != 0) {
            continue;
        }
        g_next_slot = (index + 1) % SENTRY_APP_HANG_SAMPLE_SLOTS;
        while (sem_trywait(&slot->done) == 0) { }
#    if defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)
        while (sem_trywait(&slot->uctx_ready) == 0) { }
        while (sem_trywait(&slot->unwind_done) == 0) { }
        slot->abort_park = 0;
        slot->uctx = NULL;
#    endif
        return slot;
    }
    return NULL;
}

static bool
wait_for(sem_t *sem, const struct timespec *deadline)
{
    while (sem_timedwait(sem, deadline) != 0) {
        if (errno != EINTR) {
            return false;
        }
    }
    return true;
}

size_t
sentry__thread_stackwalk(
    uint64_t target_tid, void **ips, size_t max, uint64_t timeout_ms)
{
    if (!ensure_installed() || is_signal_pending(target_tid)) {
        return 0;
    }
    sample_slot_t *slot = acquire_slot();
    if (!slot) {
        SENTRY_DEBUG("app-hang: all sample slots are in use");
        return 0;
    }

    slot->want
        = max < SENTRY_APP_HANG_MAX_FRAMES ? max : SENTRY_APP_HANG_MAX_FRAMES;
    slot->count = 0;
    // Release: publishes the slot fields above to the handler that claims the
    // sample.
    __atomic_store_n(&slot->waiting_tid, (pid_t)target_tid, __ATOMIC_RELEASE);

    if (syscall(SYS_tgkill, getpid(), (pid_t)target_tid, SENTRY_APP_HANG_SIGNAL)
        != 0) {
        const int error = errno;
        __atomic_store_n(&slot->waiting_tid, 0, __ATOMIC_RELEASE);
        SENTRY_DEBUGF("app-hang: tgkill(%d) failed: %s", (int)target_tid,
            strerror(error));
        return 0;
    }

    const struct timespec deadline = deadline_after(timeout_ms);
#    if defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)
    if (!wait_for(&slot->uctx_ready, &deadline)) {
        // Abandon the slot. A handler that parks later is released at once.
        __atomic_store_n(&slot->waiting_tid, 0, __ATOMIC_RELEASE);
        slot->abort_park = 1;
        sem_post(&slot->unwind_done);
        return 0;
    }
    sentry_ucontext_t s;
    memset(&s, 0, sizeof(s));
    s.user_context = slot->uctx;
    size_t n = sentry_unwind_stack_from_ucontext(&s, ips, max);
    sem_post(&slot->unwind_done); // release the parked handler
    wait_for(&slot->done, &deadline);
    return n;
#    else
    if (!wait_for(&slot->done, &deadline)) {
        // Abandon the slot: a late handler writes only into it.
        __atomic_store_n(&slot->waiting_tid, 0, __ATOMIC_RELEASE);
        return 0;
    }
    size_t n = slot->count < max ? slot->count : max;
    memcpy(ips, slot->ips, n * sizeof(void *));
    return n;
#    endif
}

static void
read_thread_name(const char *tid, char *name, size_t name_size)
{
    name[0] = '\0';
    char comm_path[64];
    snprintf(comm_path, sizeof(comm_path), "/proc/self/task/%.16s/comm", tid);
    FILE *comm_file = fopen(comm_path, "r");
    if (!comm_file) {
        return;
    }
    if (fgets(name, (int)name_size, comm_file)) {
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == '\n') {
            name[len - 1] = '\0';
        }
    }
    fclose(comm_file);
}

size_t
sentry__thread_list(sentry_thread_info_t *out, size_t max)
{
    DIR *dir = opendir("/proc/self/task");
    if (!dir) {
        return 0;
    }
    size_t count = 0;
    struct dirent *entry;
    while (count < max && (entry = readdir(dir))) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        out[count].tid = strtoull(entry->d_name, NULL, 10);
        read_thread_name(
            entry->d_name, out[count].name, sizeof(out[count].name));
        count++;
    }
    closedir(dir);
    return count;
}

#endif
