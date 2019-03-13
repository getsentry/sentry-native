#include <map>
#include <string>
#include "sentry.h"

namespace sentry {
namespace crashpad {
int init(const sentry_options_t *options,
         const char *minidump_url,
         std::map<std::string, std::string> attachments);
int set_annotation(const char *key, const char *value);
int remove_annotation(const char *key);
} /* namespace crashpad */
} /* namespace sentry */
