#ifndef SENTRY_OPTIONS_HPP_INCLUDED
#define SENTRY_OPTIONS_HPP_INCLUDED

#include <string>
#include <vector>
#include "attachment.hpp"
#include "dsn.hpp"
#include "internal.hpp"
#include "path.hpp"
#include "transports/base.hpp"

struct sentry_options_s {
    sentry_options_s();

    sentry::Dsn dsn;
    std::string release;
    std::string environment;
    std::string dist;
    bool debug;
    std::vector<sentry::Attachment> attachments;
    sentry::Path handler_path;
    sentry::Path database_path;

    sentry::transports::Transport *transport = nullptr;

    // internal options
    std::string run_id;
    sentry::Path runs_folder;
};

#endif
