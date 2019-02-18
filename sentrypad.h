#ifndef SENTRY_PAD_H
#define SENTRY_PAD_H

#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

    int sentrypad_init(void);
    // int sentrypad_shutdown(void);
    // int sentrypad_set_tag(const char *key, const char *value);
    // int sentrypad_remove_tag(const char *key);
    // int sentrypad_attach_file_by_path(const char *path);
    // int sentrypad_attach_file_with_contents(const char *filename, const char *buf, size_t len);
    // int sentrypad_capture_stacktrace(void);

#ifdef __cplusplus
}
#endif
#endif
