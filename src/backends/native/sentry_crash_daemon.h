#ifndef SENTRY_CRASH_DAEMON_H_INCLUDED
#define SENTRY_CRASH_DAEMON_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_crash_ipc.h"

#if defined(SENTRY_PLATFORM_UNIX)
#include <sys/types.h>
#elif defined(SENTRY_PLATFORM_WINDOWS)
#include <windows.h>
#endif

// Forward declaration
struct sentry_options_s;

/**
 * Start crash daemon for monitoring app process
 * This forks a child process (Unix) or creates a new process (Windows) that waits for crashes
 *
 * @param app_pid Parent application process ID
 * @param eventfd_handle Event notification handle (Unix) or HANDLE (Windows)
 * @return Daemon PID on success, -1 on failure
 */
#if defined(SENTRY_PLATFORM_UNIX)
pid_t sentry__crash_daemon_start(pid_t app_pid, int eventfd_handle);
#elif defined(SENTRY_PLATFORM_WINDOWS)
pid_t sentry__crash_daemon_start(pid_t app_pid, HANDLE event_handle);
#endif

/**
 * Daemon main loop (runs in forked child on Unix, or separate process on Windows)
 */
#if defined(SENTRY_PLATFORM_UNIX)
int sentry__crash_daemon_main(pid_t app_pid, int eventfd_handle);
#elif defined(SENTRY_PLATFORM_WINDOWS)
int sentry__crash_daemon_main(pid_t app_pid, HANDLE event_handle);
#endif

/**
 * Process crash and generate minidump with envelope
 *
 * Called by the crash daemon (out-of-process on Linux/macOS).
 *
 * It writes the minidump, creates an envelope with all attachments,
 * and sends it via transport. Signal-safe, avoids SDK mutexes.
 *
 * @param options Sentry options (DSN, transport, etc.)
 * @param ipc Crash IPC with crash context in shared memory
 */
void sentry__process_crash(
    const struct sentry_options_s *options, sentry_crash_ipc_t *ipc);

#endif
