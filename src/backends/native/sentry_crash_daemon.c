#include "sentry_crash_daemon.h"

#include "minidump/sentry_minidump_writer.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_crash_ipc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_json.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_process.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_uuid.h"
#include "sentry_value.h"
#include "transports/sentry_disk_transport.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Buffer size for file I/O operations
#define SENTRY_FILE_COPY_BUFFER_SIZE (8 * 1024) // 8KB

// Path buffer size for constructing file paths
// Use system PATH_MAX where available, fallback to 4096
#ifdef PATH_MAX
#    define SENTRY_PATH_BUFFER_SIZE PATH_MAX
#else
#    define SENTRY_PATH_BUFFER_SIZE 4096
#endif

/**
 * Helper to write a file as an attachment to an envelope
 * Returns true on success, false on failure
 */
static bool
write_attachment_to_envelope(int fd, const char *file_path,
    const char *filename, const char *content_type)
{
    int attach_fd = open(file_path, O_RDONLY);
    if (attach_fd < 0) {
        SENTRY_WARNF("Failed to open attachment file: %s", file_path);
        return false;
    }

    struct stat st;
    if (fstat(attach_fd, &st) != 0) {
        SENTRY_WARNF("Failed to stat attachment file: %s", file_path);
        close(attach_fd);
        return false;
    }

    // Write attachment item header
    int header_written;
    if (content_type) {
        header_written = dprintf(fd,
            "{\"type\":\"attachment\",\"length\":%lld,"
            "\"attachment_type\":\"event.attachment\","
            "\"content_type\":\"%s\","
            "\"filename\":\"%s\"}\n",
            (long long)st.st_size, content_type,
            filename ? filename : "attachment");
    } else {
        header_written = dprintf(fd,
            "{\"type\":\"attachment\",\"length\":%lld,"
            "\"attachment_type\":\"event.attachment\","
            "\"filename\":\"%s\"}\n",
            (long long)st.st_size, filename ? filename : "attachment");
    }

    if (header_written < 0) {
        SENTRY_WARN("Failed to write attachment header");
        close(attach_fd);
        return false;
    }

    // Copy attachment content
    char buf[SENTRY_FILE_COPY_BUFFER_SIZE];
    ssize_t n;
    while ((n = read(attach_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = write(fd, buf, n);
        if (written != n) {
            SENTRY_WARNF("Failed to write attachment content for: %s", file_path);
            close(attach_fd);
            return false;
        }
    }

    if (n < 0) {
        SENTRY_WARNF("Failed to read attachment file: %s", file_path);
        close(attach_fd);
        return false;
    }

    write(fd, "\n", 1);
    close(attach_fd);
    return true;
}

/**
 * Manually write a Sentry envelope with event, minidump, and attachments.
 * Format matches what Crashpad's Envelope class does.
 */
static bool
write_envelope_with_minidump(const sentry_options_t *options,
    const char *envelope_path, const char *event_msgpack_path,
    const char *minidump_path, sentry_path_t *run_folder)
{
    // Open envelope file for writing
    int fd = open(envelope_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        SENTRY_WARN("Failed to open envelope file for writing");
        return false;
    }

    // Write envelope headers (just DSN if available)
    const char *dsn
        = options && options->dsn ? sentry_options_get_dsn(options) : NULL;
    if (dsn) {
        dprintf(fd, "{\"dsn\":\"%s\"}\n", dsn);
    } else {
        dprintf(fd, "{}\n");
    }

    // Read event JSON data
    sentry_path_t *ev_path = sentry__path_from_str(event_msgpack_path);
    if (ev_path) {
        size_t event_size = 0;
        char *event_json = sentry__path_read_to_buffer(ev_path, &event_size);
        sentry__path_free(ev_path);

        if (event_json && event_size > 0) {
            // Write event item header
            dprintf(fd, "{\"type\":\"event\",\"length\":%zu}\n", event_size);
            // Write JSON event payload
            write(fd, event_json, event_size);
            write(fd, "\n", 1);
            sentry_free(event_json);
        }
    }

    // Add minidump as attachment
    int minidump_fd = open(minidump_path, O_RDONLY);
    if (minidump_fd >= 0) {
        struct stat st;
        if (fstat(minidump_fd, &st) == 0) {
            // Write minidump item header
            dprintf(fd,
                "{\"type\":\"attachment\",\"length\":%lld,"
                "\"attachment_type\":\"event.minidump\","
                "\"filename\":\"minidump.dmp\"}\n",
                (long long)st.st_size);

            // Copy minidump content
            char buf[8192];
            ssize_t n;
            while ((n = read(minidump_fd, buf, sizeof(buf))) > 0) {
                write(fd, buf, n);
            }
            write(fd, "\n", 1);
        }
        close(minidump_fd);
    }

    // Add scope attachments using metadata file
    if (run_folder) {
        sentry_path_t *attach_list_path
            = sentry__path_join_str(run_folder, "__sentry-attachments");
        if (attach_list_path) {
            size_t attach_json_len = 0;
            char *attach_json
                = sentry__path_read_to_buffer(attach_list_path, &attach_json_len);
            sentry__path_free(attach_list_path);

            if (attach_json && attach_json_len > 0) {
                // Parse attachment list JSON
                sentry_value_t attach_list
                    = sentry__value_from_json(attach_json, attach_json_len);
                sentry_free(attach_json);

                if (!sentry_value_is_null(attach_list)) {
                    size_t len = sentry_value_get_length(attach_list);
                    for (size_t i = 0; i < len; i++) {
                        sentry_value_t attach_info
                            = sentry_value_get_by_index(attach_list, i);
                        sentry_value_t path_val
                            = sentry_value_get_by_key(attach_info, "path");
                        sentry_value_t filename_val
                            = sentry_value_get_by_key(attach_info, "filename");
                        sentry_value_t content_type_val
                            = sentry_value_get_by_key(attach_info, "content_type");

                        const char *path = sentry_value_as_string(path_val);
                        const char *filename
                            = sentry_value_as_string(filename_val);
                        const char *content_type
                            = sentry_value_as_string(content_type_val);

                        if (path && filename) {
                            write_attachment_to_envelope(
                                fd, path, filename, content_type);
                        }
                    }
                    sentry_value_decref(attach_list);
                }
            }
        }
    }

    close(fd);
    SENTRY_INFO("Envelope written successfully");
    return true;
}

/**
 * Process crash and generate minidump
 * Uses Sentry's API to reuse all existing functionality
 *
 * Called by the crash daemon (out-of-process on Linux/macOS).
 */
void
sentry__process_crash(const sentry_options_t *options, sentry_crash_ipc_t *ipc)
{
    SENTRY_DEBUG("Processing crash");

    sentry_crash_context_t *ctx = ipc->shmem;

    // Mark as processing
    atomic_store(&ctx->state, SENTRY_CRASH_STATE_PROCESSING);

    // Generate minidump path in database directory
    char minidump_path[SENTRY_PATH_BUFFER_SIZE];
    const char *db_dir = ctx->database_path;
    int path_len = snprintf(minidump_path, sizeof(minidump_path),
        "%s/sentry-minidump-%d-%d.dmp", db_dir, ctx->crashed_pid,
        ctx->crashed_tid);

    if (path_len < 0 || path_len >= (int)sizeof(minidump_path)) {
        SENTRY_WARN("Minidump path truncated or invalid");
        goto done;
    }

    SENTRY_DEBUG("Writing minidump");

    // Write minidump
    if (sentry__write_minidump(ctx, minidump_path) == 0) {
        SENTRY_INFO("Minidump written successfully");

        // Copy minidump path back to shared memory
        strncpy(
            ctx->minidump_path, minidump_path, sizeof(ctx->minidump_path) - 1);
        ctx->minidump_path[sizeof(ctx->minidump_path) - 1] = '\0';

        // Get event file path from context
        const char *event_path = ctx->event_path[0] ? ctx->event_path : NULL;
        if (!event_path) {
            SENTRY_WARN("No event file from parent");
            goto done;
        }

        // Extract run folder path from event path (event is at
        // run_folder/__sentry-event)
        sentry_path_t *ev_path = sentry__path_from_str(event_path);
        sentry_path_t *run_folder = ev_path ? sentry__path_dir(ev_path) : NULL;
        if (ev_path)
            sentry__path_free(ev_path);

        // Create envelope file in database directory
        char envelope_path[SENTRY_PATH_BUFFER_SIZE];
        path_len = snprintf(envelope_path, sizeof(envelope_path),
            "%s/sentry-envelope-%d.env", db_dir, ctx->crashed_pid);

        if (path_len < 0 || path_len >= (int)sizeof(envelope_path)) {
            SENTRY_WARN("Envelope path truncated or invalid");
            if (run_folder) {
                sentry__path_free(run_folder);
            }
            goto done;
        }

        // Write envelope manually with all attachments from run folder
        // (avoids mutex-locked SDK functions)
        if (!write_envelope_with_minidump(
                options, envelope_path, event_path, minidump_path, run_folder)) {
            SENTRY_WARN("Failed to write envelope");
            if (run_folder) {
                sentry__path_free(run_folder);
            }
            goto done;
        }

        // Read envelope and send via transport
        sentry_path_t *env_path = sentry__path_from_str(envelope_path);
        if (!env_path) {
            SENTRY_WARN("Failed to create envelope path");
            goto cleanup;
        }

        sentry_envelope_t *envelope = sentry__envelope_from_path(env_path);
        sentry__path_free(env_path);

        if (!envelope) {
            SENTRY_WARN("Failed to read envelope file");
            goto cleanup;
        }

        SENTRY_INFO("Sending crash envelope via transport");

        // Send directly via transport
        if (options && options->transport) {
            sentry__transport_send_envelope(options->transport, envelope);
            SENTRY_INFO("Crash envelope sent successfully");
        } else {
            SENTRY_WARN("No transport available for sending envelope");
            sentry_envelope_free(envelope);
        }

        // Clean up temporary envelope file (keep minidump for inspection/debugging)
        unlink(envelope_path);
        // Note: minidump file is kept in database for debugging/inspection

    cleanup:
        // Send all other envelopes from run folder (logs, etc.) before cleanup
        if (run_folder && options && options->transport) {
            SENTRY_DEBUG("Sending additional envelopes from run folder");
            sentry_pathiter_t *piter = sentry__path_iter_directory(run_folder);
            if (piter) {
                const sentry_path_t *file_path;
                while ((file_path = sentry__pathiter_next(piter)) != NULL) {
                    // Check if this is an envelope file (ends with .envelope)
                    const char *path_str = file_path->path;
                    size_t len = strlen(path_str);
                    if (len > 9
                        && strcmp(path_str + len - 9, ".envelope") == 0) {
                        SENTRY_DEBUGF(
                            "Sending envelope from run folder: %s", path_str);
                        sentry_envelope_t *run_envelope
                            = sentry__envelope_from_path(file_path);
                        if (run_envelope) {
                            sentry__transport_send_envelope(
                                options->transport, run_envelope);
                        }
                    }
                }
                sentry__pathiter_free(piter);
            }
        }

        // Clean up the entire run folder (contains breadcrumbs, etc.)
        if (run_folder) {
            sentry__path_remove_all(run_folder);

            // Also delete the lock file (run_folder.lock)
            sentry_path_t *lock_path
                = sentry__path_append_str(run_folder, ".lock");
            if (lock_path) {
                sentry__path_remove(lock_path);
                sentry__path_free(lock_path);
            }

            sentry__path_free(run_folder);
            SENTRY_DEBUG("Cleaned up crash run folder and lock file");
        }

        SENTRY_DEBUG("Cleaned up crash files");
    } else {
        SENTRY_WARN("Failed to write minidump");
    }

done:
    // Mark as done
    atomic_store(&ctx->state, SENTRY_CRASH_STATE_DONE);
    SENTRY_DEBUG("Crash processing complete");
}

/**
 * Check if parent process is still alive
 */
static bool
is_parent_alive(pid_t parent_pid)
{
    // Send signal 0 to check if process exists
    return kill(parent_pid, 0) == 0 || errno != ESRCH;
}

int
sentry__crash_daemon_main(pid_t app_pid, int eventfd_handle)
{
    // Close standard streams to avoid interfering with parent
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Open /dev/null for std streams
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) {
            close(devnull);
        }
    }

    // Initialize IPC (attach to shared memory created by parent)
    sentry_crash_ipc_t *ipc = sentry__crash_ipc_init_daemon(app_pid);
    if (!ipc) {
        return 1;
    }

    // Initialize Sentry options for daemon (reuses all SDK infrastructure)
    // Options are passed explicitly to all functions, no global state
    sentry_options_t *options = sentry_options_new();
    if (options) {
        // Set DSN if configured
        if (ipc->shmem->dsn[0] != '\0') {
            sentry_options_set_dsn(options, ipc->shmem->dsn);
        }

        // Create run with database path
        sentry_path_t *db_path
            = sentry__path_from_str(ipc->shmem->database_path);
        if (db_path) {
            options->run = sentry__run_new(db_path);
            sentry__path_free(db_path);
        }

        // Set external crash reporter if configured
        if (ipc->shmem->external_reporter_path[0] != '\0') {
            sentry_path_t *reporter
                = sentry__path_from_str(ipc->shmem->external_reporter_path);
            if (reporter) {
                options->external_crash_reporter = reporter;
            }
        }

        // Initialize transport for sending envelopes
        options->transport = sentry__transport_new_default();
        if (options->transport) {
            sentry__transport_startup(options->transport, options);
        }

        SENTRY_DEBUG("Daemon options initialized");
    }

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    // Use the inherited eventfd from parent
    ipc->eventfd = eventfd_handle;
#else
    // On other platforms, notification mechanism is set up by init_daemon
    (void)eventfd_handle;
#endif

    SENTRY_DEBUG("Entering main loop");

    // Daemon main loop
    bool crash_processed = false;
    while (true) {
        // Wait for crash notification (with timeout to check parent health)
        if (sentry__crash_ipc_wait(ipc, 5000)) { // 5 second timeout
            // Crash occurred!
            uint32_t state = atomic_load(&ipc->shmem->state);
            if (state == SENTRY_CRASH_STATE_CRASHED && !crash_processed) {
                SENTRY_INFO("Crash notification received");
                sentry__process_crash(options, ipc);
                crash_processed = true;

                // After processing crash, exit regardless of parent state
                // (parent has likely already exited after re-raising signal)
                SENTRY_DEBUG("Crash processed, daemon exiting");
                break;
            }
            // If crash already processed, just ignore spurious notifications
        }

        // Check if parent is still alive (only if no crash processed yet)
        if (!crash_processed && !is_parent_alive(app_pid)) {
            SENTRY_DEBUG("Parent process exited without crash");
            break;
        }
    }

    SENTRY_DEBUG("Daemon exiting");

    // Cleanup
    if (options) {
        if (options->transport) {
            // Wait up to 2 seconds for transport to send pending envelopes
            // (crash envelope + logs envelope, etc.)
            sentry__transport_shutdown(options->transport, 2000);
        }
        sentry_options_free(options);
    }
    sentry__crash_ipc_free(ipc);

    return 0;
}

pid_t
sentry__crash_daemon_start(pid_t app_pid, int eventfd_handle)
{
    pid_t daemon_pid = fork();

    if (daemon_pid < 0) {
        // Fork failed
        return -1;
    } else if (daemon_pid == 0) {
        // Child process - become daemon
        // Create new session
        setsid();

        // Run daemon main loop
        int exit_code = sentry__crash_daemon_main(app_pid, eventfd_handle);
        _exit(exit_code);
    }

    // Parent process - return daemon PID
    return daemon_pid;
}
