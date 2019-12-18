#ifndef SENTRY_OPTIONS_HPP_INCLUDED
#define SENTRY_OPTIONS_HPP_INCLUDED

#include <functional>
#include <string>
#include <vector>

#include "attachment.hpp"
#include "backends/base_backend.hpp"
#include "dsn.hpp"
#include "internal.hpp"
#include "path.hpp"
#include "transports/base_transport.hpp"

struct sentry_options_s {
    sentry_options_s();

    sentry::Dsn dsn;
    std::string release;
    std::string environment;
    std::string dist;
    std::string http_proxy;
    std::string ca_certs;
    bool debug;
    std::vector<sentry::Attachment> attachments;
    sentry::Path handler_path;
    sentry::Path database_path;
    bool system_crash_reporter_enabled;
    bool require_user_consent;

    std::function<sentry::Value(sentry::Value, void *hint)> before_send;
    sentry::transports::Transport *transport;
    sentry::backends::Backend *backend;

    // internal options
    std::string run_id;
    sentry::Path runs_folder;
    sentry_user_consent_t user_consent;

    bool should_upload() const {
        return !require_user_consent ||
               user_consent == SENTRY_USER_CONSENT_GIVEN;
    }
};

#endif
