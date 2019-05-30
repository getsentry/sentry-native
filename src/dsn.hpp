#ifndef SENTRY_DSN_HPP_INCLUDED
#define SENTRY_DSN_HPP_INCLUDED
#include <string>
#include "internal.hpp"

namespace sentry {
class Dsn {
   public:
    Dsn() : m_valid(false) {
    }
    Dsn(const char *dsn);

    const char *scheme() const {
        return m_https ? "https" : "http";
    }
    const char *public_key() const {
        return m_public_key.c_str();
    }
    const char *private_key() const {
        return m_private_key.c_str();
    }
    const char *host() const {
        return m_host.c_str();
    }
    int port() const {
        return m_port;
    }
    const char *path() const {
        return m_path.c_str();
    }
    const char *project_id() const {
        return m_project_id.c_str();
    }
    bool valid() const {
        return m_valid;
    }
    const char *raw() const {
        return m_valid ? m_raw.c_str() : nullptr;
    }

    std::string &&get_minidump_url() const;

   private:
    std::string m_raw;
    bool m_https;
    std::string m_public_key;
    std::string m_private_key;
    std::string m_host;
    int m_port;
    std::string m_path;
    std::string m_project_id;
    bool m_valid;
};
}  // namespace sentry

#endif
