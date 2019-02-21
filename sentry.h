#ifndef SENTRY_H
#define SENTRY_H

#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct sentry_options_s
    {
        // Unified API
        const char *dsn;
        const char *release;
        const char *environment;
        const char *dist;
        int debug;
        // Crashpad
        const char *handler_path;
        const char *database_path;
        // TODO hook/callback to crashpad configuration object.
        // Breakpad
        // TODO: whatever breakpad needs
    } sentry_options_t;

    // typedef struct sentry_breadcrumb_s
    // {

    // } sentry_breadcrumb_t;

    typedef struct sentry_user_s
    {
        const char *username;
        const char *email;
        const char *id;
        const char *ip_address;
    } sentry_user_t;

    enum sentry_level_t
    {
        Debug,
        Info,
        Warning,
        Error
    };

    // // int sentrypad_shutdown(void);
    // int sentrypad_set_tag(const char *key, const char *value);
    // int sentrypad_set_extra(const char *key, const char *value);
    // int sentrypad_set_release(const char *release);
    // // int sentrypad_remove_tag(const char *key);
    // int sentrypad_attach_file_by_path(const char *path);
    // int sentrypad_attach_file_with_contents(const char *filename, const char *buf, size_t len);
    // // int sentrypad_capture_stacktrace(void);

    // Unified API
    int sentry_init(const sentry_options_t *options);
    // int sentry_add_breadcrumb(sentry_breadcrumb_t *breadcrumb);
    int sentry_push_scope();
    int sentry_pop_scope();
    int sentry_set_user(sentry_user_t *user);
    int sentry_remove_user();
    int sentry_set_tag(const char *key, const char *value);
    int sentry_remove_tag(const char *key);
    int sentry_set_extra(const char *key, const char *value);
    int sentry_remove_extra(const char *key);
    int sentry_set_release(const char *release);
    int sentry_remove_release();
    int sentry_set_fingerprint(const char **fingerprint, size_t len);
    int sentry_remove_fingerprint();
    int sentry_set_transaction(const char *transaction);
    int sentry_remove_transaction();
    int sentry_set_level(enum sentry_level_t level);

    // Sentrypad custom API
    int sentry_attach_file_by_path(const char *path);
    int sentry_attach_file_with_contents(const char *filename, const char *buf, size_t len);
    int sentry_capture_minidump(const char *optional_message);
    void sentry_options_init(sentry_options_t *options);

#ifdef __cplusplus
}
#endif
#endif
