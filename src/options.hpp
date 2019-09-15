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

    std::function<sentry::Value(sentry::Value, void *hint)> before_send;
    sentry::transports::Transport *transport;
    sentry::backends::Backend *backend;

    // internal options
    std::string run_id;
    sentry::Path runs_folder;
};

#endif
