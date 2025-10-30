#include "sentry_crash_handler.h"

#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_logger.h"
#include "sentry_sync.h"

#include <string.h>
#include <time.h>

#if defined(SENTRY_PLATFORM_UNIX)
#    include <errno.h>
#    include <fcntl.h>
#    include <limits.h>
#    include <pthread.h>
#    include <signal.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>
#elif defined(SENTRY_PLATFORM_WINDOWS)
#    include <tlhelp32.h>
#    include <windows.h>
#    include <winnt.h>
#endif

#if defined(SENTRY_PLATFORM_UNIX)

#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#        include <dirent.h>
#        include <stdlib.h>
#        include <sys/syscall.h>
#    endif

#    if defined(SENTRY_PLATFORM_MACOS)
#        include <mach-o/dyld.h>
#        include <mach-o/loader.h>
#        include <mach/mach.h>
#        include <mach/mach_vm.h>
#    endif

// Signals to handle
static const int g_crash_signals[] = {
    SIGABRT,
    SIGBUS,
    SIGFPE,
    SIGILL,
    SIGSEGV,
    SIGSYS,
    SIGTRAP,
};
static const size_t g_crash_signal_count
    = sizeof(g_crash_signals) / sizeof(g_crash_signals[0]);

// Global state (signal-safe)
static sentry_crash_ipc_t *g_crash_ipc = NULL;
static struct sigaction g_previous_handlers[16];
static stack_t g_signal_stack = { 0 };

/**
 * Get current thread ID (signal-safe)
 */
static pid_t
get_tid(void)
{
#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    return (pid_t)syscall(SYS_gettid);
#    elif defined(SENTRY_PLATFORM_MACOS)
    // Use mach_thread_self() which is signal-safe on macOS
    return (pid_t)mach_thread_self();
#    else
    return getpid();
#    endif
}

#    if defined(SENTRY_PLATFORM_MACOS)
/**
 * Safe string copy (signal-safe, only used on macOS)
 */
static void
safe_strncpy(char *dest, const char *src, size_t n)
{
    if (!dest || !src || n == 0) {
        return;
    }

    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}
#    endif // SENTRY_PLATFORM_MACOS

/**
 * Signal handler (signal-safe)
 */
static void
crash_signal_handler(int signum, siginfo_t *info, void *context)
{
    // Only handle crash once - check if already processing
    static volatile long handling_crash = 0;
    if (!sentry__atomic_compare_swap(&handling_crash, 0, 1)) {
        // Already handling a crash, just exit immediately
        _exit(1);
    }

    // Re-enable signal to prevent loops
    signal(signum, SIG_DFL);

    sentry_crash_ipc_t *ipc = g_crash_ipc;
    if (!ipc || !ipc->shmem) {
        // No IPC available, just re-raise
        raise(signum);
        return;
    }

    sentry_crash_context_t *ctx = ipc->shmem;
    ucontext_t *uctx = (ucontext_t *)context;

    // Fill crash context
    ctx->crashed_pid = getpid();
    ctx->crashed_tid = get_tid();

#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    ctx->platform.signum = signum;
    ctx->platform.siginfo = *info;
    ctx->platform.context = *uctx;

    // Capture all threads on Linux
    ctx->platform.num_threads = 0;

    // Open /proc/self/task directory to enumerate threads
    DIR *task_dir = opendir("/proc/self/task");
    if (task_dir) {
        struct dirent *entry;
        while ((entry = readdir(task_dir)) != NULL
            && ctx->platform.num_threads < SENTRY_CRASH_MAX_THREADS) {

            // Skip "." and ".."
            if (entry->d_name[0] == '.') {
                continue;
            }

            pid_t tid = (pid_t)atoi(entry->d_name);
            if (tid == 0) {
                continue;
            }

            // Store thread ID
            ctx->platform.threads[ctx->platform.num_threads].tid = tid;

            // For the crashing thread, we already have the context from signal
            // handler
            if (tid == ctx->crashed_tid) {
                ctx->platform.threads[ctx->platform.num_threads].context
                    = *uctx;
                ctx->platform.num_threads++;
                continue;
            }

            // For other threads, try to read their context from
            // /proc/[pid]/task/[tid]/ Note: This is not always possible from
            // signal handler context We'll just store the TID and let the
            // daemon read the state if possible
            memset(&ctx->platform.threads[ctx->platform.num_threads].context, 0,
                sizeof(ucontext_t));
            ctx->platform.num_threads++;
        }
        closedir(task_dir);
    }

    // If we couldn't enumerate threads, at least store the crashing thread
    if (ctx->platform.num_threads == 0) {
        ctx->platform.threads[0].tid = ctx->crashed_tid;
        ctx->platform.threads[0].context = *uctx;
        ctx->platform.num_threads = 1;
    }
#    elif defined(SENTRY_PLATFORM_MACOS)
    ctx->platform.signum = signum;
    ctx->platform.siginfo = *info;
    // Copy mcontext data (ucontext_t.uc_mcontext is just a pointer)
    ctx->platform.mcontext = *uctx->uc_mcontext;

    // Capture all threads (signal-safe on macOS)
    ctx->platform.num_threads = 0;
    task_t task = mach_task_self();
    thread_act_array_t threads = NULL;
    mach_msg_type_number_t thread_count = 0;

    // Get the crashing thread
    thread_t crashing_thread = mach_thread_self();

    kern_return_t kr = task_threads(task, &threads, &thread_count);
    if (kr == KERN_SUCCESS) {
        // Limit to available space
        if (thread_count > SENTRY_CRASH_MAX_THREADS) {
            thread_count = SENTRY_CRASH_MAX_THREADS;
        }

        for (mach_msg_type_number_t i = 0; i < thread_count; i++) {
            ctx->platform.threads[i].thread = threads[i];

            // Get thread ID (portable across processes)
            thread_identifier_info_data_t identifier_info;
            mach_msg_type_number_t identifier_info_count
                = THREAD_IDENTIFIER_INFO_COUNT;
            if (thread_info(threads[i], THREAD_IDENTIFIER_INFO,
                    (thread_info_t)&identifier_info, &identifier_info_count)
                == KERN_SUCCESS) {
                ctx->platform.threads[i].tid = identifier_info.thread_id;
            } else {
                ctx->platform.threads[i].tid = 0;
            }

            // For the crashing thread, use the context from the signal handler
            // For other threads, use thread_get_state()
            bool is_crashing_thread = (threads[i] == crashing_thread);

            if (is_crashing_thread) {
                // Use register state from signal handler context
#        if defined(__x86_64__)
                ctx->platform.threads[i].state.__ss.__rax
                    = uctx->uc_mcontext->__ss.__rax;
                ctx->platform.threads[i].state.__ss.__rbx
                    = uctx->uc_mcontext->__ss.__rbx;
                ctx->platform.threads[i].state.__ss.__rcx
                    = uctx->uc_mcontext->__ss.__rcx;
                ctx->platform.threads[i].state.__ss.__rdx
                    = uctx->uc_mcontext->__ss.__rdx;
                ctx->platform.threads[i].state.__ss.__rdi
                    = uctx->uc_mcontext->__ss.__rdi;
                ctx->platform.threads[i].state.__ss.__rsi
                    = uctx->uc_mcontext->__ss.__rsi;
                ctx->platform.threads[i].state.__ss.__rbp
                    = uctx->uc_mcontext->__ss.__rbp;
                ctx->platform.threads[i].state.__ss.__rsp
                    = uctx->uc_mcontext->__ss.__rsp;
                ctx->platform.threads[i].state.__ss.__r8
                    = uctx->uc_mcontext->__ss.__r8;
                ctx->platform.threads[i].state.__ss.__r9
                    = uctx->uc_mcontext->__ss.__r9;
                ctx->platform.threads[i].state.__ss.__r10
                    = uctx->uc_mcontext->__ss.__r10;
                ctx->platform.threads[i].state.__ss.__r11
                    = uctx->uc_mcontext->__ss.__r11;
                ctx->platform.threads[i].state.__ss.__r12
                    = uctx->uc_mcontext->__ss.__r12;
                ctx->platform.threads[i].state.__ss.__r13
                    = uctx->uc_mcontext->__ss.__r13;
                ctx->platform.threads[i].state.__ss.__r14
                    = uctx->uc_mcontext->__ss.__r14;
                ctx->platform.threads[i].state.__ss.__r15
                    = uctx->uc_mcontext->__ss.__r15;
                ctx->platform.threads[i].state.__ss.__rip
                    = uctx->uc_mcontext->__ss.__rip;
                ctx->platform.threads[i].state.__ss.__rflags
                    = uctx->uc_mcontext->__ss.__rflags;
                ctx->platform.threads[i].state.__ss.__cs
                    = uctx->uc_mcontext->__ss.__cs;
                ctx->platform.threads[i].state.__ss.__fs
                    = uctx->uc_mcontext->__ss.__fs;
                ctx->platform.threads[i].state.__ss.__gs
                    = uctx->uc_mcontext->__ss.__gs;
#        elif defined(__aarch64__)
                // Copy all registers from signal handler context
                for (int j = 0; j < 29; j++) {
                    ctx->platform.threads[i].state.__ss.__x[j]
                        = uctx->uc_mcontext->__ss.__x[j];
                }
                ctx->platform.threads[i].state.__ss.__fp
                    = uctx->uc_mcontext->__ss.__fp;
                ctx->platform.threads[i].state.__ss.__lr
                    = uctx->uc_mcontext->__ss.__lr;
                ctx->platform.threads[i].state.__ss.__sp
                    = uctx->uc_mcontext->__ss.__sp;
                ctx->platform.threads[i].state.__ss.__pc
                    = uctx->uc_mcontext->__ss.__pc;
                ctx->platform.threads[i].state.__ss.__cpsr
                    = uctx->uc_mcontext->__ss.__cpsr;
#        endif
            } else {
                // Capture thread state from thread_get_state for other threads
                mach_msg_type_number_t state_count = MACHINE_THREAD_STATE_COUNT;
                kern_return_t state_kr
                    = thread_get_state(threads[i], MACHINE_THREAD_STATE,
                        (thread_state_t)&ctx->platform.threads[i].state,
                        &state_count);
                if (state_kr != KERN_SUCCESS) {
                    // Failed to get state, but continue with other threads
                    memset(&ctx->platform.threads[i].state, 0,
                        sizeof(ctx->platform.threads[i].state));
                    ctx->platform.threads[i].stack_path[0] = '\0';
                    ctx->platform.threads[i].stack_size = 0;
                    continue;
                }
            }

            // Capture stack memory for this thread
            uint64_t sp;
#        if defined(__x86_64__)
            sp = ctx->platform.threads[i].state.__ss.__rsp;
#        elif defined(__aarch64__)
            sp = ctx->platform.threads[i].state.__ss.__sp;
#        else
            sp = 0;
#        endif

            if (sp > 0) {
                // Query stack bounds using vm_region (signal-safe)
                mach_vm_address_t address = sp;
                mach_vm_size_t region_size = 0;
                vm_region_basic_info_data_64_t info;
                mach_msg_type_number_t info_count
                    = VM_REGION_BASIC_INFO_COUNT_64;
                mach_port_t object_name;

                kern_return_t kr = mach_vm_region(task, &address, &region_size,
                    VM_REGION_BASIC_INFO_64, (vm_region_info_t)&info,
                    &info_count, &object_name);

                size_t actual_stack_size = 0;
                if (kr == KERN_SUCCESS) {
                    // Stack region found - capture from SP to end of region
                    uint64_t region_end = address + region_size;
                    if (sp >= address && sp < region_end) {
                        actual_stack_size = region_end - sp;
                    }
                }

                // Fallback: if vm_region failed or returned unreasonable size,
                // use a safe maximum (e.g., 512KB is typical stack size)
                if (actual_stack_size == 0
                    || actual_stack_size > SENTRY_CRASH_MAX_REGION_SIZE / 8) {
                    actual_stack_size = SENTRY_CRASH_MAX_STACK_CAPTURE;
                }

                if (actual_stack_size > 0) {
                    // Create stack file path in database directory
                    char stack_path[SENTRY_CRASH_MAX_PATH];
                    int len = snprintf(stack_path, sizeof(stack_path),
                        "%s/__sentry-stack%u", ctx->database_path, i);

                    // Check for truncation (signal-safe check)
                    if (len < 0 || len >= (int)sizeof(stack_path)) {
                        continue; // Skip this thread if path too long
                    }

                    // Open and write stack memory (signal-safe)
                    int stack_fd
                        = open(stack_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                    if (stack_fd >= 0) {
                        // Write stack memory from SP upwards
                        ssize_t written
                            = write(stack_fd, (void *)sp, actual_stack_size);
                        close(stack_fd);

                        if (written > 0) {
                            // Successfully saved stack (even if partial)
                            safe_strncpy(ctx->platform.threads[i].stack_path,
                                stack_path,
                                sizeof(ctx->platform.threads[i].stack_path));
                            ctx->platform.threads[i].stack_size
                                = (size_t)written;
                        } else {
                            ctx->platform.threads[i].stack_path[0] = '\0';
                            ctx->platform.threads[i].stack_size = 0;
                        }
                    } else {
                        ctx->platform.threads[i].stack_path[0] = '\0';
                        ctx->platform.threads[i].stack_size = 0;
                    }
                } else {
                    ctx->platform.threads[i].stack_path[0] = '\0';
                    ctx->platform.threads[i].stack_size = 0;
                }
            } else {
                ctx->platform.threads[i].stack_path[0] = '\0';
                ctx->platform.threads[i].stack_size = 0;
            }
        }
        ctx->platform.num_threads = thread_count;

        // Don't deallocate threads array here - will be done by daemon
        // The thread ports remain valid across processes
    } else {
        // task_threads failed - this might happen from signal handler
        // Fall back to just capturing the crashing thread
        ctx->platform.num_threads = 0;
    }

    // Capture module information from dyld (signal-safe on macOS)
    ctx->module_count = 0;
    uint32_t image_count = _dyld_image_count();
    if (image_count > SENTRY_CRASH_MAX_MODULES) {
        image_count = SENTRY_CRASH_MAX_MODULES;
    }

    for (uint32_t i = 0;
        i < image_count && ctx->module_count < SENTRY_CRASH_MAX_MODULES; i++) {
        const struct mach_header *header = _dyld_get_image_header(i);
        const char *name = _dyld_get_image_name(i);
        intptr_t slide = _dyld_get_image_vmaddr_slide(i);

        if (!header || !name) {
            continue;
        }

        sentry_module_info_t *module = &ctx->modules[ctx->module_count++];
        module->base_address = (uint64_t)header + slide;

        // Calculate module size and extract UUID (signal-safe)
        uint32_t size = 0;
        memset(module->uuid, 0, sizeof(module->uuid)); // Zero UUID by default

        if (header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
            const struct mach_header_64 *header64
                = (const struct mach_header_64 *)header;
            const uint8_t *cmds = (const uint8_t *)(header64 + 1);

            for (uint32_t j = 0; j < header64->ncmds && j < 256; j++) {
                const struct load_command *cmd
                    = (const struct load_command *)cmds;

                if (cmd->cmd == LC_SEGMENT_64) {
                    const struct segment_command_64 *seg
                        = (const struct segment_command_64 *)cmd;
                    uint32_t seg_end = seg->vmaddr + seg->vmsize;
                    if (seg_end > size) {
                        size = seg_end;
                    }
                } else if (cmd->cmd == LC_UUID) {
                    // Extract UUID for symbolication
                    const struct uuid_command *uuid_cmd
                        = (const struct uuid_command *)cmd;
                    memcpy(module->uuid, uuid_cmd->uuid, 16);
                }

                cmds += cmd->cmdsize;
                if (cmd->cmdsize == 0)
                    break; // Prevent infinite loop
            }
        }
        module->size = size;

        // Copy module name (signal-safe)
        safe_strncpy(module->name, name, sizeof(module->name));
    }
#    endif

    // Call Sentry's exception handler to invoke on_crash/before_send hooks
    // This must happen BEFORE notifying the daemon
    sentry_ucontext_t sentry_uctx;
    sentry_uctx.signum = signum;
    sentry_uctx.siginfo = info;
    sentry_uctx.user_context = uctx;
    sentry_handle_exception(&sentry_uctx);

    // Try to notify daemon
    if (sentry__atomic_compare_swap(&ctx->state, SENTRY_CRASH_STATE_READY,
            SENTRY_CRASH_STATE_CRASHED)) {

        // Successfully claimed crash slot, notify daemon
        sentry__crash_ipc_notify(ipc);

        // Wait for daemon to finish processing (keep process alive for
        // minidump)
        bool processing_started = false;
        int elapsed_ms = 0;
        while (elapsed_ms < SENTRY_CRASH_HANDLER_WAIT_TIMEOUT_MS) {
            long state = sentry__atomic_fetch(&ctx->state);
            if (state == SENTRY_CRASH_STATE_PROCESSING && !processing_started) {
                // Daemon started processing (no logging - signal-safe)
                processing_started = true;
            } else if (state == SENTRY_CRASH_STATE_DONE) {
                // Daemon finished processing (no logging - signal-safe)
                goto daemon_handling;
            }

            // Sleep using poll interval (signal-safe)
            struct timespec ts = { .tv_sec = 0,
                .tv_nsec = SENTRY_CRASH_HANDLER_POLL_INTERVAL_MS * 1000000LL };
            nanosleep(&ts, NULL);
            elapsed_ms += SENTRY_CRASH_HANDLER_POLL_INTERVAL_MS;
        }

        // Timeout (no logging - signal-safe)
    }

daemon_handling:
    // Re-raise signal to let system handle it
    SENTRY_DEBUG("Wait complete, allowing process to terminate");
    raise(signum);
}

int
sentry__crash_handler_init(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return -1;
    }

    g_crash_ipc = ipc;

    // Set up signal stack
    g_signal_stack.ss_sp = sentry_malloc(SENTRY_CRASH_SIGNAL_STACK_SIZE);
    if (!g_signal_stack.ss_sp) {
        SENTRY_WARN("failed to allocate signal stack");
        return -1;
    }

    g_signal_stack.ss_size = SENTRY_CRASH_SIGNAL_STACK_SIZE;
    g_signal_stack.ss_flags = 0;

    if (sigaltstack(&g_signal_stack, NULL) < 0) {
        SENTRY_WARNF("failed to set signal stack: %s", strerror(errno));
        sentry_free(g_signal_stack.ss_sp);
        g_signal_stack.ss_sp = NULL;
        return -1;
    }

    // Install signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = crash_signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

    for (size_t i = 0; i < g_crash_signal_count; i++) {
        int sig = g_crash_signals[i];
        if (sigaction(sig, &sa, &g_previous_handlers[i]) < 0) {
            SENTRY_WARNF("failed to install handler for signal %d: %s", sig,
                strerror(errno));
        }
    }

    SENTRY_DEBUG("crash handler initialized");
    return 0;
}

void
sentry__crash_handler_shutdown(void)
{
    // Restore previous signal handlers
    for (size_t i = 0; i < g_crash_signal_count; i++) {
        sigaction(g_crash_signals[i], &g_previous_handlers[i], NULL);
    }

    // Clean up signal stack
    if (g_signal_stack.ss_sp) {
        g_signal_stack.ss_flags = SS_DISABLE;
        sigaltstack(&g_signal_stack, NULL);
        sentry_free(g_signal_stack.ss_sp);
        g_signal_stack.ss_sp = NULL;
    }

    g_crash_ipc = NULL;

    SENTRY_DEBUG("crash handler shutdown");
}

#elif defined(SENTRY_PLATFORM_WINDOWS)

// Global state for Windows exception handling
static sentry_crash_ipc_t *g_crash_ipc = NULL;
static LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter = NULL;

/**
 * Windows exception filter (crash handler)
 */
static LONG WINAPI
crash_exception_filter(EXCEPTION_POINTERS *exception_info)
{
    // Only handle crash once
    static volatile long handling_crash = 0;
    if (!sentry__atomic_compare_swap(&handling_crash, 0, 1)) {
        // Already handling a crash (no logging - exception filter context)
        return EXCEPTION_CONTINUE_SEARCH;
    }

    sentry_crash_ipc_t *ipc = g_crash_ipc;
    if (!ipc || !ipc->shmem) {
        // No IPC or shared memory (no logging - exception filter context)
        return EXCEPTION_CONTINUE_SEARCH;
    }

    sentry_crash_context_t *ctx = ipc->shmem;

    // Fill crash context
    ctx->crashed_pid = GetCurrentProcessId();
    ctx->crashed_tid = GetCurrentThreadId();

    // Store exception information
    ctx->platform.exception_code
        = exception_info->ExceptionRecord->ExceptionCode;
    ctx->platform.exception_record = *exception_info->ExceptionRecord;
    ctx->platform.context = *exception_info->ContextRecord;
    // Store original exception pointers for out-of-process minidump writing
    ctx->platform.exception_pointers = exception_info;

    // Capture all threads
    ctx->platform.num_threads = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te = { 0 };
        te.dwSize = sizeof(te);
        DWORD current_pid = GetCurrentProcessId();
        DWORD current_tid = GetCurrentThreadId();

        if (Thread32First(snapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == current_pid
                    && ctx->platform.num_threads < SENTRY_CRASH_MAX_THREADS) {

                    ctx->platform.threads[ctx->platform.num_threads].thread_id
                        = te.th32ThreadID;

                    // For the crashing thread, use the context from exception
                    if (te.th32ThreadID == current_tid) {
                        ctx->platform.threads[ctx->platform.num_threads].context
                            = *exception_info->ContextRecord;
                    } else {
                        // For other threads, try to suspend and get context
                        HANDLE thread = OpenThread(
                            THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        if (thread) {
                            SuspendThread(thread);
                            CONTEXT thread_ctx = { 0 };
                            thread_ctx.ContextFlags = CONTEXT_ALL;
                            if (GetThreadContext(thread, &thread_ctx)) {
                                ctx->platform.threads[ctx->platform.num_threads]
                                    .context
                                    = thread_ctx;
                            }
                            ResumeThread(thread);
                            CloseHandle(thread);
                        }
                    }
                    ctx->platform.num_threads++;
                }
            } while (Thread32Next(snapshot, &te));
        }
        CloseHandle(snapshot);
    }

    // Call Sentry's exception handler
    sentry_ucontext_t sentry_uctx = { 0 };
    sentry_uctx.exception_ptrs = *exception_info;
    sentry_handle_exception(&sentry_uctx);

    bool swap_result = sentry__atomic_compare_swap(
        &ctx->state, SENTRY_CRASH_STATE_READY, SENTRY_CRASH_STATE_CRASHED);

    if (swap_result) {
        // Successfully claimed crash slot, notify daemon
        sentry__crash_ipc_notify(ipc);

        // Wait for daemon to finish processing (keep process alive for
        // minidump)
        bool processing_started = false;
        int elapsed_ms = 0;
        while (elapsed_ms < SENTRY_CRASH_HANDLER_WAIT_TIMEOUT_MS) {
            long state = sentry__atomic_fetch(&ctx->state);
            if (state == SENTRY_CRASH_STATE_PROCESSING && !processing_started) {
                // Daemon started processing (no logging - exception filter context)
                processing_started = true;
            } else if (state == SENTRY_CRASH_STATE_DONE) {
                // Daemon finished processing (no logging - exception filter context)
                break;
            }
            Sleep(SENTRY_CRASH_HANDLER_POLL_INTERVAL_MS);
            elapsed_ms += SENTRY_CRASH_HANDLER_POLL_INTERVAL_MS;
        }

        // Timeout or completion (no logging - exception filter context)
    }

    // Continue to default handler (which will terminate the process)
    return EXCEPTION_CONTINUE_SEARCH;
}

int
sentry__crash_handler_init(sentry_crash_ipc_t *ipc)
{
    if (!ipc) {
        return -1;
    }

    g_crash_ipc = ipc;

    // Install exception filter
    g_previous_filter = SetUnhandledExceptionFilter(crash_exception_filter);

    SENTRY_DEBUG("crash handler initialized (Windows SEH)");
    return 0;
}

void
sentry__crash_handler_shutdown(void)
{
    // Restore previous exception filter
    if (g_previous_filter) {
        SetUnhandledExceptionFilter(g_previous_filter);
        g_previous_filter = NULL;
    }

    g_crash_ipc = NULL;

    SENTRY_DEBUG("crash handler shutdown");
}

#endif // SENTRY_PLATFORM_WINDOWS
