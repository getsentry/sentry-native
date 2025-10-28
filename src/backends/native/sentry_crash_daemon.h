#ifndef SENTRY_CRASH_DAEMON_H_INCLUDED
#define SENTRY_CRASH_DAEMON_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_crash_ipc.h"

#include <sys/types.h>

// Forward declaration
struct sentry_options_s;

/**
 * Start crash daemon for monitoring app process
 * This forks a child process that waits for crashes
 *
 * @param app_pid Parent application process ID
 * @param eventfd_handle Event notification handle (inherited from parent)
 * @return Daemon PID on success, -1 on failure
 */
pid_t sentry__crash_daemon_start(pid_t app_pid, int eventfd_handle);

/**
 * Daemon main loop (runs in forked child)
 */
int sentry__crash_daemon_main(pid_t app_pid, int eventfd_handle);

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
