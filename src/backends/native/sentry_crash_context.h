#ifndef SENTRY_CRASH_CONTEXT_H_INCLUDED
#define SENTRY_CRASH_CONTEXT_H_INCLUDED

#include "sentry_boot.h"
#include "sentry.h" // For sentry_minidump_mode_t

#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>

#if defined(SENTRY_PLATFORM_UNIX)
// Define _XOPEN_SOURCE for ucontext.h on macOS
#    ifndef _XOPEN_SOURCE
#        define _XOPEN_SOURCE 700
#    endif
#    include <signal.h>
#    include <sys/types.h>
#    include <ucontext.h>
#    include <unistd.h>
#elif defined(SENTRY_PLATFORM_WINDOWS)
#    include <windows.h>
#endif

#define SENTRY_CRASH_MAGIC 0x53454E54 // "SENT"
#define SENTRY_CRASH_VERSION 1

// Limits for crash context (used in shared memory and minidump writers)
#define SENTRY_CRASH_MAX_THREADS 256
#define SENTRY_CRASH_MAX_MODULES 512
#define SENTRY_CRASH_MAX_MAPPINGS 4096

// Max path length in crash context
// Use system PATH_MAX where available (typically 4096 on Linux/macOS, 260 on
// Windows) Fall back to 1024 for safety on systems without PATH_MAX
#ifdef PATH_MAX
#    define SENTRY_CRASH_MAX_PATH PATH_MAX
#else
#    define SENTRY_CRASH_MAX_PATH 4096
#endif

// Note: SENTRY_CRASH_SHM_SIZE is defined after sentry_crash_context_t
// so we can calculate it using sizeof()

/**
 * Crash state machine for atomic coordination between app and daemon
 */
typedef enum {
    SENTRY_CRASH_STATE_READY = 0,
    SENTRY_CRASH_STATE_CRASHED = 1,
    SENTRY_CRASH_STATE_PROCESSING = 2,
    SENTRY_CRASH_STATE_DONE = 3
} sentry_crash_state_t;

/**
 * Module info for minidump (captured in signal handler)
 */
typedef struct {
    uint64_t base_address;
    uint64_t size;
    char name[SENTRY_CRASH_MAX_PATH];
    uint8_t uuid[16]; // Module UUID for symbolication
} sentry_module_info_t;

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)

/**
 * Linux/Android thread context
 */
typedef struct {
    pid_t tid;
    ucontext_t context;
} sentry_thread_context_linux_t;

/**
 * Linux/Android specific crash context
 */
typedef struct {
    int signum;
    siginfo_t siginfo;
    ucontext_t context;

    // Additional thread contexts (for multi-thread dumps)
    size_t num_threads;
    sentry_thread_context_linux_t threads[SENTRY_CRASH_MAX_THREADS];
} sentry_crash_platform_linux_t;

#elif defined(SENTRY_PLATFORM_MACOS)

#    include <mach/mach.h>

/**
 * macOS thread context
 */
typedef struct {
    thread_t thread; // Mach thread port (only valid in crashed process)
    uint64_t tid; // Thread ID (portable across processes)
    _STRUCT_MCONTEXT state;
    char stack_path[SENTRY_CRASH_MAX_PATH]; // Path to saved stack memory file
    uint64_t stack_size; // Size of captured stack
} sentry_thread_context_darwin_t;

/**
 * macOS specific crash context
 */
typedef struct {
    int signum;
    siginfo_t siginfo;
    // Store mcontext directly (ucontext_t.uc_mcontext is just a pointer)
    _STRUCT_MCONTEXT mcontext;

    // Mach thread state
    thread_t mach_thread;

    // Additional thread contexts
    size_t num_threads;
    sentry_thread_context_darwin_t threads[SENTRY_CRASH_MAX_THREADS];
} sentry_crash_platform_darwin_t;

#elif defined(SENTRY_PLATFORM_WINDOWS)

/**
 * Windows thread context
 */
typedef struct {
    DWORD thread_id;
    CONTEXT context;
} sentry_thread_context_windows_t;

/**
 * Windows specific crash context
 */
typedef struct {
    DWORD exception_code;
    EXCEPTION_RECORD exception_record;
    CONTEXT context;

    // Additional thread contexts
    DWORD num_threads;
    sentry_thread_context_windows_t threads[SENTRY_CRASH_MAX_THREADS];
} sentry_crash_platform_windows_t;

#endif

/**
 * Shared memory structure for crash communication.
 * This MUST be safe to write from signal handlers (no allocations, no locks).
 */
typedef struct {
    // Header with magic + version for validation
    uint32_t magic;
    uint32_t version;

    // Atomic state machine
    atomic_uint_fast32_t state;
    atomic_uint_fast32_t sequence;

    // Process info
    pid_t crashed_pid;
    pid_t crashed_tid;

    // Configuration (set by app during init)
    sentry_minidump_mode_t minidump_mode;

    // Platform-specific crash context
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    sentry_crash_platform_linux_t platform;
#elif defined(SENTRY_PLATFORM_MACOS)
    sentry_crash_platform_darwin_t platform;
#elif defined(SENTRY_PLATFORM_WINDOWS)
    sentry_crash_platform_windows_t platform;
#endif

    // Sentry-specific metadata paths
    char database_path[SENTRY_CRASH_MAX_PATH]; // Database directory for all
                                               // files
    char event_path[SENTRY_CRASH_MAX_PATH];
    char breadcrumb1_path[SENTRY_CRASH_MAX_PATH];
    char breadcrumb2_path[SENTRY_CRASH_MAX_PATH];
    char envelope_path[SENTRY_CRASH_MAX_PATH];
    char external_reporter_path[SENTRY_CRASH_MAX_PATH];
    char dsn[SENTRY_CRASH_MAX_PATH]; // Sentry DSN for uploading crashes

    // Minidump output path (filled by daemon)
    char minidump_path[SENTRY_CRASH_MAX_PATH];

    // Module information (captured in signal handler from dyld)
    uint32_t module_count;
    sentry_module_info_t modules[SENTRY_CRASH_MAX_MODULES];

} sentry_crash_context_t;

// Shared memory size: calculated at compile-time based on actual struct size
// Add 8KB padding for safety and future additions
#define SENTRY_CRASH_SHM_SIZE \
    (sizeof(sentry_crash_context_t) + (8 * 1024))

#endif
