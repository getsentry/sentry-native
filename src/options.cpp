#include "options.hpp"
#include <cstdlib>
#include <ctime>
#include <random>
#include <sstream>
#include "transports/function.hpp"
#include "transports/libcurl.hpp"

static const char *getenv_or_empty(const char *key) {
    const char *rv = getenv(key);
    return rv ? rv : "";
}

static const char *empty_str_null(const char *s) {
    if (!s && !*s) {
        return nullptr;
    } else {
        return s;
    }
}

sentry_options_s::sentry_options_s()
    : debug(false),
      database_path("./.sentrypad"),
      dsn(getenv_or_empty("SENTRY_DSN")),
      environment(getenv_or_empty("SENTRY_ENVIRONMENT")),
      release(getenv_or_empty("SENTRY_RELEASE")),
      transport(new sentry::transports::LibcurlTransport()),
      before_send(nullptr) {
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

void sentry_options_set_transport(sentry_options_t *opts,
                                  sentry_transport_function_t func,
                                  void *data) {
    delete opts->transport;
    opts->transport = new sentry::transports::FunctionTransport(
        [func, data](sentry::Value value) { func(value.lower(), data); });
}

void sentry_options_set_before_send(sentry_options_t *opts,
                                    sentry_event_function_t func) {
    opts->before_send = func;
}

void sentry_options_free(sentry_options_t *opts) {
    if (opts) {
        delete opts;
    }
}

void sentry_options_set_dsn(sentry_options_t *opts, const char *dsn) {
    opts->dsn = dsn;
}

const char *sentry_options_get_dsn(const sentry_options_t *opts) {
    return opts->dsn.raw();
}

void sentry_options_set_release(sentry_options_t *opts, const char *release) {
    opts->release = release;
}

const char *sentry_options_get_release(const sentry_options_t *opts) {
    return empty_str_null(opts->release.c_str());
}

void sentry_options_set_environment(sentry_options_t *opts,
                                    const char *environment) {
    opts->environment = environment;
}

const char *sentry_options_get_environment(const sentry_options_t *opts) {
    return empty_str_null(opts->environment.c_str());
}

void sentry_options_set_dist(sentry_options_t *opts, const char *dist) {
    opts->dist = dist;
}

const char *sentry_options_get_dist(const sentry_options_t *opts) {
    return empty_str_null(opts->dist.c_str());
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
    opts->attachments.emplace_back(sentry::Attachment(name, path));
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
    opts->attachments.emplace_back(sentry::Attachment(name, path));
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
