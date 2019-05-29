#include "options.hpp"
#include <ctime>
#include <random>
#include <sstream>

sentry_options_s::sentry_options_s()
    : debug(false), database_path("./.sentrypad") {
    std::random_device seed;
    std::default_random_engine engine(seed());
    std::uniform_int_distribution<int> uniform_dist(0, INT32_MAX);
    time_t result = std::time(nullptr);
    std::stringstream ss;
    ss << result << "-" << uniform_dist(engine);
    run_id = ss.str();
}

sentry_options_t *sentry_options_new(void) {
    return new sentry_options_t();
}

void sentry_options_free(sentry_options_t *opts) {
    if (opts) {
        delete opts;
    }
}

void sentry_options_set_dsn(sentry_options_t *opts, const char *dsn) {
    opts->dsn = dsn;
}

void sentry_options_set_release(sentry_options_t *opts, const char *release) {
    opts->release = release;
}

void sentry_options_set_environment(sentry_options_t *opts,
                                    const char *environment) {
    opts->environment = environment;
}

void sentry_options_set_dist(sentry_options_t *opts, const char *dist) {
    opts->dist = dist;
}

void sentry_options_set_debug(sentry_options_t *opts, int debug) {
    opts->debug = !!debug;
}

int sentry_options_get_debug(const sentry_options_t *opts) {
    return opts->debug;
}

void sentry_options_add_attachment(sentry_options_t *opts,
                                   const char *name,
                                   const char *path) {
    opts->attachments.push_back(sentry::Attachment(name, path));
}

void sentry_options_set_handler_path(sentry_options_t *opts, const char *path) {
    opts->handler_path = path;
}

void sentry_options_set_database_path(sentry_options_t *opts,
                                      const char *path) {
    opts->database_path = path;
}

#ifdef _WIN32
void sentry_options_add_attachmentw(sentry_options_t *opts,
                                    const char *name,
                                    const wchar_t *path) {
    opts->attachments.push_back(sentry::Attachment(name, path));
}

void sentry_options_set_handler_pathw(sentry_options_t *opts,
                                      const wchar_t *path) {
    opts->handler_path = path;
}

void sentry_options_set_database_pathw(sentry_options_t *opts,
                                       const wchar_t *path) {
    opts->database_path = path;
}
#endif
