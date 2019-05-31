#include <string>
#include <vector>
#include "attachment.hpp"
#include "dsn.hpp"
#include "internal.hpp"
#include "path.hpp"

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

    // internal options
    std::string run_id;
    sentry::Path runs_folder;
};
