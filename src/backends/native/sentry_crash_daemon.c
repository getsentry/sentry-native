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
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_uuid.h"
#include "sentry_value.h"
#include "transports/sentry_disk_transport.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(SENTRY_PLATFORM_UNIX)
#    include <errno.h>
#    include <fcntl.h>
#    include <signal.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <unistd.h>
#elif defined(SENTRY_PLATFORM_WINDOWS)
#    include <fcntl.h>
#    include <io.h>
#    include <sys/stat.h>
#    include <windows.h>
#endif

/**
 * Helper to write a file as an attachment to an envelope
 * Returns true on success, false on failure
 */
static bool
write_attachment_to_envelope(int fd, const char *file_path,
    const char *filename, const char *content_type)
{
#if defined(SENTRY_PLATFORM_UNIX)
    int attach_fd = open(file_path, O_RDONLY);
#elif defined(SENTRY_PLATFORM_WINDOWS)
    int attach_fd = _open(file_path, _O_RDONLY | _O_BINARY);
#endif
    if (attach_fd < 0) {
        SENTRY_WARNF("Failed to open attachment file: %s", file_path);
        return false;
    }

#if defined(SENTRY_PLATFORM_UNIX)
    struct stat st;
    if (fstat(attach_fd, &st) != 0) {
        SENTRY_WARNF("Failed to stat attachment file: %s", file_path);
        close(attach_fd);
        return false;
    }
    long long file_size = (long long)st.st_size;
#elif defined(SENTRY_PLATFORM_WINDOWS)
    struct __stat64 st;
    if (_fstat64(attach_fd, &st) != 0) {
        SENTRY_WARNF("Failed to stat attachment file: %s", file_path);
        _close(attach_fd);
        return false;
    }
    long long file_size = (long long)st.st_size;
#endif

    // Write attachment item header
    char header[SENTRY_CRASH_ENVELOPE_HEADER_SIZE];
    int header_written;
    if (content_type) {
        header_written = snprintf(header, sizeof(header),
            "{\"type\":\"attachment\",\"length\":%lld,"
            "\"attachment_type\":\"event.attachment\","
            "\"content_type\":\"%s\","
            "\"filename\":\"%s\"}\n",
            file_size, content_type, filename ? filename : "attachment");
    } else {
        header_written = snprintf(header, sizeof(header),
            "{\"type\":\"attachment\",\"length\":%lld,"
            "\"attachment_type\":\"event.attachment\","
            "\"filename\":\"%s\"}\n",
            file_size, filename ? filename : "attachment");
    }

    if (header_written < 0 || header_written >= (int)sizeof(header)) {
        SENTRY_WARN("Failed to write attachment header");
#if defined(SENTRY_PLATFORM_UNIX)
        close(attach_fd);
#elif defined(SENTRY_PLATFORM_WINDOWS)
        _close(attach_fd);
#endif
        return false;
    }

#if defined(SENTRY_PLATFORM_UNIX)
    if (write(fd, header, header_written) != (ssize_t)header_written) {
        SENTRY_WARN("Failed to write attachment header to envelope");
    }
#elif defined(SENTRY_PLATFORM_WINDOWS)
    _write(fd, header, header_written);
#endif

    // Copy attachment content
    char buf[SENTRY_CRASH_FILE_BUFFER_SIZE];
#if defined(SENTRY_PLATFORM_UNIX)
    ssize_t n;
    while ((n = read(attach_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = write(fd, buf, n);
        if (written != n) {
            SENTRY_WARNF(
                "Failed to write attachment content for: %s", file_path);
            close(attach_fd);
            return false;
        }
    }

    if (n < 0) {
        SENTRY_WARNF("Failed to read attachment file: %s", file_path);
        close(attach_fd);
        return false;
    }

    if (write(fd, "\n", 1) != 1) {
        SENTRY_WARN("Failed to write newline to envelope");
    }
    close(attach_fd);
#elif defined(SENTRY_PLATFORM_WINDOWS)
    int n;
    while ((n = _read(attach_fd, buf, sizeof(buf))) > 0) {
        int written = _write(fd, buf, n);
        if (written != n) {
            SENTRY_WARNF(
                "Failed to write attachment content for: %s", file_path);
            _close(attach_fd);
            return false;
        }
    }

    if (n < 0) {
        SENTRY_WARNF("Failed to read attachment file: %s", file_path);
        _close(attach_fd);
        return false;
    }

    _write(fd, "\n", 1);
    _close(attach_fd);
#endif
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
#if defined(SENTRY_PLATFORM_UNIX)
    int fd = open(envelope_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
#elif defined(SENTRY_PLATFORM_WINDOWS)
    int fd = _open(envelope_path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
        _S_IREAD | _S_IWRITE);
#endif
    if (fd < 0) {
        SENTRY_WARN("Failed to open envelope file for writing");
        return false;
    }

    // Write envelope headers (just DSN if available)
    const char *dsn
        = options && options->dsn ? sentry_options_get_dsn(options) : NULL;
    char header_buf[SENTRY_CRASH_ENVELOPE_HEADER_SIZE];
    int header_len;
    if (dsn) {
        header_len = snprintf(
            header_buf, sizeof(header_buf), "{\"dsn\":\"%s\"}\n", dsn);
    } else {
        header_len = snprintf(header_buf, sizeof(header_buf), "{}\n");
    }
    if (header_len > 0 && header_len < (int)sizeof(header_buf)) {
#if defined(SENTRY_PLATFORM_UNIX)
        if (write(fd, header_buf, header_len) != header_len) {
            SENTRY_WARN("Failed to write envelope header");
        }
#elif defined(SENTRY_PLATFORM_WINDOWS)
        _write(fd, header_buf, header_len);
#endif
    }

    // Read event JSON data
    sentry_path_t *ev_path = sentry__path_from_str(event_msgpack_path);
    if (ev_path) {
        size_t event_size = 0;
        char *event_json = sentry__path_read_to_buffer(ev_path, &event_size);
        sentry__path_free(ev_path);

        if (event_json && event_size > 0) {
            // Write event item header
            char event_header[SENTRY_CRASH_ITEM_HEADER_SIZE];
            int ev_header_len = snprintf(event_header, sizeof(event_header),
                "{\"type\":\"event\",\"length\":%zu}\n", event_size);
            if (ev_header_len > 0
                && ev_header_len < (int)sizeof(event_header)) {
#if defined(SENTRY_PLATFORM_UNIX)
                if (write(fd, event_header, ev_header_len) != ev_header_len) {
                    SENTRY_WARN("Failed to write event header to envelope");
                }
                if (write(fd, event_json, event_size) != (ssize_t)event_size) {
                    SENTRY_WARN("Failed to write event data to envelope");
                }
                if (write(fd, "\n", 1) != 1) {
                    SENTRY_WARN("Failed to write event newline to envelope");
                }
#elif defined(SENTRY_PLATFORM_WINDOWS)
                _write(fd, event_header, ev_header_len);
                _write(fd, event_json, (unsigned int)event_size);
                _write(fd, "\n", 1);
#endif
            }
            sentry_free(event_json);
        }
    }

    // Add minidump as attachment
#if defined(SENTRY_PLATFORM_UNIX)
    int minidump_fd = open(minidump_path, O_RDONLY);
#elif defined(SENTRY_PLATFORM_WINDOWS)
    int minidump_fd = _open(minidump_path, _O_RDONLY | _O_BINARY);
#endif
    if (minidump_fd >= 0) {
#if defined(SENTRY_PLATFORM_UNIX)
        struct stat st;
        if (fstat(minidump_fd, &st) == 0) {
            long long minidump_size = (long long)st.st_size;
#elif defined(SENTRY_PLATFORM_WINDOWS)
        struct __stat64 st;
        if (_fstat64(minidump_fd, &st) == 0) {
            long long minidump_size = (long long)st.st_size;
#endif
            // Write minidump item header
            char minidump_header[SENTRY_CRASH_ITEM_HEADER_SIZE];
            int md_header_len
                = snprintf(minidump_header, sizeof(minidump_header),
                    "{\"type\":\"attachment\",\"length\":%lld,"
                    "\"attachment_type\":\"event.minidump\","
                    "\"filename\":\"minidump.dmp\"}\n",
                    minidump_size);

            if (md_header_len > 0
                && md_header_len < (int)sizeof(minidump_header)) {
#if defined(SENTRY_PLATFORM_UNIX)
                if (write(fd, minidump_header, md_header_len)
                    != md_header_len) {
                    SENTRY_WARN("Failed to write minidump header to envelope");
                }
#elif defined(SENTRY_PLATFORM_WINDOWS)
                _write(fd, minidump_header, md_header_len);
#endif
            }

            // Copy minidump content
            char buf[SENTRY_CRASH_READ_BUFFER_SIZE];
#if defined(SENTRY_PLATFORM_UNIX)
            ssize_t n;
            while ((n = read(minidump_fd, buf, sizeof(buf))) > 0) {
                if (write(fd, buf, n) != n) {
                    SENTRY_WARN("Failed to write minidump data to envelope");
                    break;
                }
            }
            if (write(fd, "\n", 1) != 1) {
                SENTRY_WARN("Failed to write minidump newline to envelope");
            }
#elif defined(SENTRY_PLATFORM_WINDOWS)
            int n;
            while ((n = _read(minidump_fd, buf, sizeof(buf))) > 0) {
                _write(fd, buf, n);
            }
            _write(fd, "\n", 1);
#endif
        }
#if defined(SENTRY_PLATFORM_UNIX)
        close(minidump_fd);
#elif defined(SENTRY_PLATFORM_WINDOWS)
        _close(minidump_fd);
#endif
    }

    // Add scope attachments using metadata file
    if (run_folder) {
        sentry_path_t *attach_list_path
            = sentry__path_join_str(run_folder, "__sentry-attachments");
        if (attach_list_path) {
            size_t attach_json_len = 0;
            char *attach_json = sentry__path_read_to_buffer(
                attach_list_path, &attach_json_len);
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
                            = sentry_value_get_by_key(
                                attach_info, "content_type");

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

#if defined(SENTRY_PLATFORM_UNIX)
    close(fd);
#elif defined(SENTRY_PLATFORM_WINDOWS)
    _close(fd);
#endif
    SENTRY_DEBUG("Envelope written successfully");
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
    SENTRY_DEBUG("Processing crash - START");

    sentry_crash_context_t *ctx = ipc->shmem;

    // Mark as processing
    sentry__atomic_store(&ctx->state, SENTRY_CRASH_STATE_PROCESSING);
    SENTRY_DEBUG("Marked state as PROCESSING");

    // Generate minidump path in database directory
    char minidump_path[SENTRY_CRASH_MAX_PATH];
    const char *db_dir = ctx->database_path;
    int path_len = snprintf(minidump_path, sizeof(minidump_path),
        "%s/sentry-minidump-%lu-%lu.dmp", db_dir,
        (unsigned long)ctx->crashed_pid, (unsigned long)ctx->crashed_tid);

    if (path_len < 0 || path_len >= (int)sizeof(minidump_path)) {
        SENTRY_WARN("Minidump path truncated or invalid");
        goto done;
    }

    SENTRY_DEBUGF("Writing minidump to: %s", minidump_path);
    SENTRY_DEBUGF(
        "About to call sentry__write_minidump, ctx=%p, crashed_pid=%d",
        (void *)ctx, ctx->crashed_pid);

    // Write minidump
    int minidump_result = sentry__write_minidump(ctx, minidump_path);
    SENTRY_DEBUGF("sentry__write_minidump returned: %d", minidump_result);

    if (minidump_result == 0) {
        SENTRY_DEBUG("Minidump written successfully");

        // Copy minidump path back to shared memory
#ifdef _WIN32
        strncpy_s(ctx->minidump_path, sizeof(ctx->minidump_path), minidump_path,
            _TRUNCATE);
#else
        strncpy(
            ctx->minidump_path, minidump_path, sizeof(ctx->minidump_path) - 1);
        ctx->minidump_path[sizeof(ctx->minidump_path) - 1] = '\0';
#endif

        // Get event file path from context
        const char *event_path = ctx->event_path[0] ? ctx->event_path : NULL;
        SENTRY_DEBUGF(
            "Event path from context: %s", event_path ? event_path : "(null)");
        if (!event_path) {
            SENTRY_WARN("No event file from parent");
            goto done;
        }

        // Extract run folder path from event path (event is at
        // run_folder/__sentry-event)
        SENTRY_DEBUG("Extracting run folder from event path");
        sentry_path_t *ev_path = sentry__path_from_str(event_path);
        sentry_path_t *run_folder = ev_path ? sentry__path_dir(ev_path) : NULL;
        if (ev_path)
            sentry__path_free(ev_path);

        // Create envelope file in database directory
        char envelope_path[SENTRY_CRASH_MAX_PATH];
        path_len = snprintf(envelope_path, sizeof(envelope_path),
            "%s/sentry-envelope-%lu.env", db_dir,
            (unsigned long)ctx->crashed_pid);

        if (path_len < 0 || path_len >= (int)sizeof(envelope_path)) {
            SENTRY_WARN("Envelope path truncated or invalid");
            if (run_folder) {
                sentry__path_free(run_folder);
            }
            goto done;
        }

        SENTRY_DEBUGF("Creating envelope at: %s", envelope_path);

        // Write envelope manually with all attachments from run folder
        // (avoids mutex-locked SDK functions)
        SENTRY_DEBUG("Writing envelope with minidump");
        if (!write_envelope_with_minidump(options, envelope_path, event_path,
                minidump_path, run_folder)) {
            SENTRY_WARN("Failed to write envelope");
            if (run_folder) {
                sentry__path_free(run_folder);
            }
            goto done;
        }
        SENTRY_DEBUG("Envelope written successfully");

        // Read envelope and send via transport
        SENTRY_DEBUG("Reading envelope file back");
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

        SENTRY_DEBUG("Envelope loaded, sending via transport");

        // Send directly via transport
        if (options && options->transport) {
            SENTRY_DEBUG("Calling transport send_envelope");
            sentry__transport_send_envelope(options->transport, envelope);
            SENTRY_DEBUG("Crash envelope sent to transport (queued)");
        } else {
            SENTRY_WARN("No transport available for sending envelope");
            sentry_envelope_free(envelope);
        }

        // Clean up temporary envelope file (keep minidump for
        // inspection/debugging)
#if defined(SENTRY_PLATFORM_UNIX)
        unlink(envelope_path);
#elif defined(SENTRY_PLATFORM_WINDOWS)
        _unlink(envelope_path);
#endif

    cleanup:
        // Send all other envelopes from run folder (logs, etc.) before cleanup
        if (run_folder && options && options->transport) {
            SENTRY_DEBUG("Checking for additional envelopes in run folder");
            sentry_pathiter_t *piter = sentry__path_iter_directory(run_folder);
            if (piter) {
                SENTRY_DEBUG("Iterating run folder for envelope files");
                const sentry_path_t *file_path;
                int envelope_count = 0;
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
                            envelope_count++;
                        } else {
                            SENTRY_WARNF(
                                "Failed to load envelope: %s", path_str);
                        }
                    }
                }
                SENTRY_DEBUGF("Sent %d additional envelopes from run folder",
                    envelope_count);
                sentry__pathiter_free(piter);
            } else {
                SENTRY_DEBUG("Could not iterate run folder");
            }
        } else {
            SENTRY_DEBUG("No run folder or transport for additional envelopes");
        }

        // Clean up the entire run folder (contains breadcrumbs, etc.)
        if (run_folder) {
            SENTRY_DEBUG("Cleaning up run folder");
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

        SENTRY_DEBUG("Crash processing completed successfully");
    } else {
        SENTRY_WARN("Failed to write minidump");
    }

done:
    // Mark as done
    SENTRY_DEBUG("Marking crash state as DONE");
    sentry__atomic_store(&ctx->state, SENTRY_CRASH_STATE_DONE);
    SENTRY_DEBUG("Processing crash - END");
    SENTRY_DEBUG("Crash processing complete");
}

/**
 * Check if parent process is still alive
 */
static bool
is_parent_alive(pid_t parent_pid)
{
#if defined(SENTRY_PLATFORM_UNIX)
    // Send signal 0 to check if process exists
    return kill(parent_pid, 0) == 0 || errno != ESRCH;
#elif defined(SENTRY_PLATFORM_WINDOWS)
    // Open handle to process with minimum rights
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, parent_pid);
    if (!hProcess) {
        return false; // Process doesn't exist or can't be accessed
    }
    // Check if process has exited
    DWORD exit_code;
    bool alive
        = GetExitCodeProcess(hProcess, &exit_code) && exit_code == STILL_ACTIVE;
    CloseHandle(hProcess);
    return alive;
#endif
}

/**
 * Custom logger function that writes to a file
 * Used by the daemon to log its activity
 */
static void
daemon_file_logger(
    sentry_level_t level, const char *message, va_list args, void *userdata)
{
    FILE *log_file = (FILE *)userdata;
    if (!log_file) {
        return;
    }

    // Get current timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[SENTRY_CRASH_TIMESTAMP_SIZE];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Get level description from Sentry's formatter
    const char *level_str = sentry__logger_describe(level);

    // Write log entry
    fprintf(log_file, "[%s] %s", timestamp, level_str);
    vfprintf(log_file, message, args);
    fprintf(log_file, "\n");
    fflush(log_file); // Flush immediately to ensure logs are written
}

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
int
sentry__crash_daemon_main(
    pid_t app_pid, uint64_t app_tid, int notify_eventfd, int ready_eventfd)
#elif defined(SENTRY_PLATFORM_MACOS)
int
sentry__crash_daemon_main(
    pid_t app_pid, uint64_t app_tid, int notify_pipe_read, int ready_pipe_write)
#elif defined(SENTRY_PLATFORM_WINDOWS)
int
sentry__crash_daemon_main(pid_t app_pid, uint64_t app_tid, HANDLE event_handle,
    HANDLE ready_event_handle)
#endif
{
    // Initialize IPC first (attach to shared memory created by parent)
    // We need this to get the database path for logging
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    sentry_crash_ipc_t *ipc = sentry__crash_ipc_init_daemon(
        app_pid, app_tid, notify_eventfd, ready_eventfd);
#elif defined(SENTRY_PLATFORM_MACOS)
    sentry_crash_ipc_t *ipc = sentry__crash_ipc_init_daemon(
        app_pid, app_tid, notify_pipe_read, ready_pipe_write);
#elif defined(SENTRY_PLATFORM_WINDOWS)
    sentry_crash_ipc_t *ipc = sentry__crash_ipc_init_daemon(
        app_pid, app_tid, event_handle, ready_event_handle);
#endif
    if (!ipc) {
        return 1;
    }

    // Set up logging to file for daemon BEFORE redirecting streams
    char log_path[SENTRY_CRASH_MAX_PATH];
    FILE *log_file = NULL;
    int log_path_len
        = snprintf(log_path, sizeof(log_path), "%s/sentry-daemon-%lu.log",
            ipc->shmem->database_path, (unsigned long)app_pid);

    if (log_path_len > 0 && log_path_len < (int)sizeof(log_path)) {
        log_file = fopen(log_path, "w");
        if (log_file) {
            // Disable buffering for immediate writes
            setvbuf(log_file, NULL, _IONBF, 0);

            // Set up Sentry logger to write to file
            // Use log level from parent's debug setting
            sentry_level_t log_level = ipc->shmem->debug_enabled
                ? SENTRY_LEVEL_DEBUG
                : SENTRY_LEVEL_INFO;
            sentry_logger_t file_logger = { .logger_func = daemon_file_logger,
                .logger_data = log_file,
                .logger_level = log_level };
            sentry__logger_set_global(file_logger);
            sentry__logger_enable();

            SENTRY_DEBUG("=== Daemon starting ===");
            SENTRY_DEBUGF("App PID: %lu", (unsigned long)app_pid);
            SENTRY_DEBUGF("Database path: %s", ipc->shmem->database_path);
        }
    }

#if defined(SENTRY_PLATFORM_UNIX)
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
#elif defined(SENTRY_PLATFORM_WINDOWS)
    // On Windows, redirect stdin/stdout to NUL
    // But redirect stderr to the log file so fprintf(stderr) appears in the log
    (void)freopen("NUL", "r", stdin);
    (void)freopen("NUL", "w", stdout);

    (void)freopen("NUL", "w", stderr);
#endif

    // Initialize Sentry options for daemon (reuses all SDK infrastructure)
    // Options are passed explicitly to all functions, no global state
    sentry_options_t *options = sentry_options_new();
    if (!options) {
        SENTRY_ERROR("sentry_options_new() failed");
        if (log_file) {
            fclose(log_file);
        }
        return 1;
    }

    // Use debug logging setting from parent process
    sentry_options_set_debug(options, ipc->shmem->debug_enabled);

    // Set custom logger that writes to file
    if (log_file) {
        sentry_options_set_logger(options, daemon_file_logger, log_file);
    }

    // Set DSN if configured
    if (ipc->shmem->dsn[0] != '\0') {
        SENTRY_DEBUGF("Setting DSN: %s", ipc->shmem->dsn);
        sentry_options_set_dsn(options, ipc->shmem->dsn);
    } else {
        SENTRY_DEBUG("No DSN configured");
    }

    // Create run with database path
    SENTRY_DEBUG("Creating run with database path");
    sentry_path_t *db_path = sentry__path_from_str(ipc->shmem->database_path);
    if (db_path) {
        options->run = sentry__run_new(db_path);
        sentry__path_free(db_path);
    }

    // Set external crash reporter if configured
    if (ipc->shmem->external_reporter_path[0] != '\0') {
        SENTRY_DEBUGF("Setting external reporter: %s",
            ipc->shmem->external_reporter_path);
        sentry_path_t *reporter
            = sentry__path_from_str(ipc->shmem->external_reporter_path);
        if (reporter) {
            options->external_crash_reporter = reporter;
        }
    }

    // Transport is already initialized by sentry_options_new(), just start it
    if (options->transport) {
        SENTRY_DEBUG("Starting transport");
        sentry__transport_startup(options->transport, options);
    } else {
        SENTRY_WARN("No transport available");
    }

    SENTRY_DEBUG("Daemon options fully initialized");

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    // Use the inherited eventfd from parent
    ipc->notify_fd = notify_eventfd;
    ipc->ready_fd = ready_eventfd;
#elif defined(SENTRY_PLATFORM_WINDOWS)
    // On Windows, event handle is already opened by name in init_daemon
    // Don't overwrite it with the parent's handle (handles are per-process)
    (void)event_handle;
    (void)ready_event_handle;
#endif

    // Signal to parent that daemon is ready
    SENTRY_DEBUG("Signaling ready to parent");
    sentry__crash_ipc_signal_ready(ipc);

    SENTRY_DEBUG("Entering main loop");

    // Daemon main loop
    bool crash_processed = false;
    while (true) {
        // Wait for crash notification (with timeout to check parent health)
        bool wait_result
            = sentry__crash_ipc_wait(ipc, SENTRY_CRASH_DAEMON_WAIT_TIMEOUT_MS);
        if (wait_result) {
            // Crash occurred!
            SENTRY_DEBUG("Event signaled, checking crash state");

            // Retry reading state with delays to handle CPU cache coherency
            // issues Between processes, cache lines may take time to
            // invalidate/sync
            long state = sentry__atomic_fetch(&ipc->shmem->state);
            if (state == SENTRY_CRASH_STATE_CRASHED && !crash_processed) {
                SENTRY_DEBUG("Crash notification received, processing");
                sentry__process_crash(options, ipc);
                crash_processed = true;

                // After processing crash, exit regardless of parent state
                // (parent has likely already exited after re-raising signal)
                SENTRY_DEBUG("Crash processed, daemon exiting");
                break;
            }
            // If crash already processed, just ignore spurious notifications
            SENTRY_DEBUG("Spurious notification or already processed");
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
            sentry__transport_shutdown(
                options->transport, SENTRY_CRASH_TRANSPORT_SHUTDOWN_TIMEOUT_MS);
        }
        sentry_options_free(options);
    }
    sentry__crash_ipc_free(ipc);

    // Close log file
    if (log_file) {
        fclose(log_file);
    }

    return 0;
}

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
pid_t
sentry__crash_daemon_start(
    pid_t app_pid, uint64_t app_tid, int notify_eventfd, int ready_eventfd)
#elif defined(SENTRY_PLATFORM_MACOS)
pid_t
sentry__crash_daemon_start(
    pid_t app_pid, uint64_t app_tid, int notify_pipe_read, int ready_pipe_write)
#elif defined(SENTRY_PLATFORM_WINDOWS)
pid_t
sentry__crash_daemon_start(pid_t app_pid, uint64_t app_tid, HANDLE event_handle,
    HANDLE ready_event_handle)
#endif
{
#if defined(SENTRY_PLATFORM_UNIX)
    // On Unix, fork and call daemon main directly (no exec)
    pid_t daemon_pid = fork();

    if (daemon_pid < 0) {
        // Fork failed
        SENTRY_WARN("Failed to fork daemon process");
        return -1;
    } else if (daemon_pid == 0) {
        // Child process - become daemon and call main directly
        setsid();

        // Call daemon main with inherited fds
#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
        exit(sentry__crash_daemon_main(
            app_pid, app_tid, notify_eventfd, ready_eventfd));
#    elif defined(SENTRY_PLATFORM_MACOS)
        exit(sentry__crash_daemon_main(
            app_pid, app_tid, notify_pipe_read, ready_pipe_write));
#    endif
    }

    // Parent process - return daemon PID
    return daemon_pid;

#elif defined(SENTRY_PLATFORM_WINDOWS)
    // On Windows, create a separate daemon process using CreateProcess
    // Spawn the sentry-crash.exe executable

    // Try to find sentry-crash.exe in the same directory as the current
    // executable
    wchar_t exe_dir[SENTRY_CRASH_MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, exe_dir, SENTRY_CRASH_MAX_PATH);
    if (len == 0 || len >= SENTRY_CRASH_MAX_PATH) {
        SENTRY_WARN("Failed to get current executable path");
        return (pid_t)-1;
    }

    // Remove filename to get directory
    wchar_t *last_slash = wcsrchr(exe_dir, L'\\');
    if (last_slash) {
        *(last_slash + 1) = L'\0'; // Keep the trailing backslash
    }

    // Build full path to sentry-crash.exe
    wchar_t daemon_path[SENTRY_CRASH_MAX_PATH];
    int path_len = _snwprintf(
        daemon_path, SENTRY_CRASH_MAX_PATH, L"%ssentry-crash.exe", exe_dir);
    if (path_len < 0 || path_len >= SENTRY_CRASH_MAX_PATH) {
        SENTRY_WARN("Daemon path too long");
        return (pid_t)-1;
    }

    // Log the daemon path we're trying to launch for debugging
    char *daemon_path_utf8 = sentry__string_from_wstr(daemon_path);
    if (daemon_path_utf8) {
        SENTRY_DEBUGF("Attempting to launch daemon: %s", daemon_path_utf8);
        sentry_free(daemon_path_utf8);
    }

    // Build command line: sentry-crash.exe <app_pid> <app_tid> <event_handle>
    // <ready_event_handle>
    wchar_t cmd_line[SENTRY_CRASH_MAX_PATH + 128];
    int cmd_len = _snwprintf(cmd_line, sizeof(cmd_line) / sizeof(wchar_t),
        L"\"%s\" %lu %llx %llu %llu", daemon_path, (unsigned long)app_pid,
        (unsigned long long)app_tid,
        (unsigned long long)(uintptr_t)event_handle,
        (unsigned long long)(uintptr_t)ready_event_handle);

    if (cmd_len < 0 || cmd_len >= (int)(sizeof(cmd_line) / sizeof(wchar_t))) {
        SENTRY_WARN("Command line too long for daemon spawn");
        return (pid_t)-1;
    }

    // Prepare process creation structures
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    // Hide console window for daemon
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    // Create the daemon process
    if (!CreateProcessW(NULL, // Application name (use command line)
            cmd_line, // Command line
            NULL, // Process security attributes
            NULL, // Thread security attributes
            TRUE, // Inherit handles (for event_handle)
            CREATE_NO_WINDOW | DETACHED_PROCESS, // Creation flags
            NULL, // Environment
            NULL, // Current directory
            &si, // Startup info
            &pi)) { // Process information
        DWORD error = GetLastError();
        char *daemon_path_err = sentry__string_from_wstr(daemon_path);
        if (daemon_path_err) {
            SENTRY_WARNF("Failed to create daemon process at '%s': Error %lu%s",
                daemon_path_err, error,
                error == 2       ? " (File not found)"
                    : error == 3 ? " (Path not found)"
                                 : "");
            sentry_free(daemon_path_err);
        } else {
            SENTRY_WARNF("Failed to create daemon process: %lu", error);
        }
        return (pid_t)-1;
    }

    // Close thread handle (we don't need it)
    CloseHandle(pi.hThread);

    // Close process handle (daemon is independent)
    CloseHandle(pi.hProcess);

    // Return daemon process ID
    return pi.dwProcessId;
#endif
}

// When built as standalone executable, provide main entry point
#ifdef SENTRY_CRASH_DAEMON_STANDALONE

int
main(int argc, char **argv)
{
    // Expected arguments: <app_pid> <app_tid> <notify_handle> <ready_handle>
    if (argc < 5) {
        fprintf(stderr,
            "Usage: sentry-crash <app_pid> <app_tid> <notify_handle> "
            "<ready_handle>\n");
        return 1;
    }

    // Parse arguments
    pid_t app_pid = (pid_t)strtoul(argv[1], NULL, 10);
    uint64_t app_tid = strtoull(argv[2], NULL, 16);

#    if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    int notify_eventfd = atoi(argv[3]);
    int ready_eventfd = atoi(argv[4]);
    return sentry__crash_daemon_main(
        app_pid, app_tid, notify_eventfd, ready_eventfd);
#    elif defined(SENTRY_PLATFORM_MACOS)
    int notify_pipe_read = atoi(argv[3]);
    int ready_pipe_write = atoi(argv[4]);
    return sentry__crash_daemon_main(
        app_pid, app_tid, notify_pipe_read, ready_pipe_write);
#    elif defined(SENTRY_PLATFORM_WINDOWS)
    unsigned long long event_handle_val = strtoull(argv[3], NULL, 10);
    unsigned long long ready_event_val = strtoull(argv[4], NULL, 10);
    HANDLE event_handle = (HANDLE)(uintptr_t)event_handle_val;
    HANDLE ready_event_handle = (HANDLE)(uintptr_t)ready_event_val;
    return sentry__crash_daemon_main(
        app_pid, app_tid, event_handle, ready_event_handle);
#    else
    fprintf(stderr, "Platform not supported\n");
    return 1;
#    endif
}

#endif // SENTRY_CRASH_DAEMON_STANDALONE
