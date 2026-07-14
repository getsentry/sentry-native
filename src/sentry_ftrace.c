#include "sentry_ftrace.h"

#ifdef SENTRY_FTRACE

#    include <errno.h>
#    include <fcntl.h>
#    include <inttypes.h>
#    include <stdarg.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <sys/types.h>
#    include <unistd.h>

#    define SENTRY_FTRACE_NAME_MAX 192
#    define SENTRY_FTRACE_EVENT_MAX 256

static int g_trace_marker_fd = -2;

static int
trace_marker_fd(void)
{
    if (g_trace_marker_fd != -2) {
        return g_trace_marker_fd;
    }

    const char *marker = getenv("SENTRY_FTRACE_MARKER");
    int fd;
    if (marker && marker[0]) {
        fd = open(marker, O_WRONLY | O_CLOEXEC);
    } else {
        fd = open("/sys/kernel/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
        if (fd < 0) {
            fd = open(
                "/sys/kernel/debug/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
        }
    }
    g_trace_marker_fd = fd >= 0 ? fd : -1;
    return g_trace_marker_fd;
}

static void
write_marker(const char *event)
{
    int fd = trace_marker_fd();
    if (fd < 0 || !event) {
        return;
    }

    size_t len = strlen(event);
    while (len > 0) {
        ssize_t written = write(fd, event, len);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        event += written;
        len -= (size_t)written;
    }
}

static void
format_name(char *buf, size_t buf_len, const char *format, va_list args)
{
    if (!buf_len) {
        return;
    }
    if (!format) {
        buf[0] = '\0';
        return;
    }

    vsnprintf(buf, buf_len, format, args);
    buf[buf_len - 1] = '\0';
}

void
sentry__ftrace_begin(const char *format, ...)
{
    char name[SENTRY_FTRACE_NAME_MAX];
    va_list args;
    va_start(args, format);
    format_name(name, sizeof(name), format, args);
    va_end(args);

    char event[SENTRY_FTRACE_EVENT_MAX];
    snprintf(event, sizeof(event), "B|%ld|%s", (long)getpid(), name);
    write_marker(event);
}

void
sentry__ftrace_end(void)
{
    write_marker("E");
}

void
sentry__ftrace_instant(const char *format, ...)
{
    char name[SENTRY_FTRACE_NAME_MAX];
    va_list args;
    va_start(args, format);
    format_name(name, sizeof(name), format, args);
    va_end(args);

    char event[SENTRY_FTRACE_EVENT_MAX];
    snprintf(event, sizeof(event), "B|%ld|%s", (long)getpid(), name);
    write_marker(event);
    write_marker("E");
}

void
sentry__ftrace_counter(int64_t value, const char *format, ...)
{
    char name[SENTRY_FTRACE_NAME_MAX];
    va_list args;
    va_start(args, format);
    format_name(name, sizeof(name), format, args);
    va_end(args);

    char event[SENTRY_FTRACE_EVENT_MAX];
    snprintf(
        event, sizeof(event), "C|%ld|%s|%" PRId64, (long)getpid(), name, value);
    write_marker(event);
}

#endif
